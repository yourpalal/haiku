/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "driver.h"
#include "hda_controller_defs.h"
#include "hda_codec_defs.h"


#define MAKE_RATE(base, multiply, divide) \
	((base == 44100 ? FMT_44_1_BASE_RATE : 0) \
		| ((multiply - 1) << FMT_MULTIPLY_RATE_SHIFT) \
		| ((divide - 1) << FMT_DIVIDE_RATE_SHIFT))

static const struct {
	uint32 multi_rate;
	uint32 hw_rate;
	uint32 rate;
} kRates[] = {
	{B_SR_8000, MAKE_RATE(48000, 1, 6), 8000},
	{B_SR_11025, MAKE_RATE(44100, 1, 4), 11025},
	{B_SR_16000, MAKE_RATE(48000, 1, 3), 16000},
	{B_SR_22050, MAKE_RATE(44100, 1, 2), 22050},
	{B_SR_32000, MAKE_RATE(48000, 2, 3), 32000},
	{B_SR_44100, MAKE_RATE(44100, 1, 1), 44100},
	{B_SR_48000, MAKE_RATE(48000, 1, 1), 48000},
	{B_SR_88200, MAKE_RATE(44100, 2, 1), 88200},
	{B_SR_96000, MAKE_RATE(48000, 2, 1), 96000},
	{B_SR_176400, MAKE_RATE(44100, 4, 1), 176400},
	{B_SR_192000, MAKE_RATE(48000, 4, 1), 192000},
};


static inline rirb_t&
current_rirb(hda_controller *controller)
{
	return controller->rirb[controller->rirb_read_pos];
}


static inline uint32
next_rirb(hda_controller *controller)
{
	return (controller->rirb_read_pos + 1) % controller->rirb_length;
}


static inline uint32
next_corb(hda_controller *controller)
{
	return (controller->corb_write_pos + 1) % controller->corb_length;
}


//! Called with interrupts off
static void
stream_handle_interrupt(hda_controller* controller, hda_stream* stream)
{
	uint8 status;

	if (!stream->running)
		return;

	status = OREG8(controller, stream->off, STS);
	if (status == 0)
		return;

	OREG8(controller, stream->off, STS) = status;

	if ((status & STS_BCIS) != 0) {
		// Buffer Completed Interrupt
		acquire_spinlock(&stream->lock);

		stream->real_time = system_time();
		stream->frames_count += stream->buffer_length;
		stream->buffer_cycle = (stream->buffer_cycle + 1)
			% stream->num_buffers;

		release_spinlock(&stream->lock);

		release_sem_etc(stream->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
	} else
		dprintf("HDA: stream status %x\n", status);
}


static int32
hda_interrupt_handler(hda_controller* controller)
{
	int32 rc = B_HANDLED_INTERRUPT;

	/* Check if this interrupt is ours */
	uint32 intsts = REG32(controller, INTSTS);
	if ((intsts & INTSTS_GIS) == 0)
		return B_UNHANDLED_INTERRUPT;

	/* Controller or stream related? */
	if (intsts & INTSTS_CIS) {
		uint32 stateStatus = REG16(controller, STATESTS);
		uint8 rirbStatus = REG8(controller, RIRBSTS);
		uint8 corbStatus = REG8(controller, CORBSTS);

		if (stateStatus) {
			/* Detected Codec state change */
			REG16(controller, STATESTS) = stateStatus;
			controller->codec_status = stateStatus;
		}

		/* Check for incoming responses */			
		if (rirbStatus) {
			REG8(controller, RIRBSTS) = rirbStatus;

			if (rirbStatus & RIRBSTS_RINTFL) {
				uint16 writePos = (REG16(controller, RIRBWP) + 1)
					% controller->rirb_length;

				for (; controller->rirb_read_pos != writePos;
						controller->rirb_read_pos = next_rirb(controller)) {
					uint32 response = current_rirb(controller).response;
					uint32 responseFlags = current_rirb(controller).flags;
					uint32 cad = responseFlags & RESPONSE_FLAGS_CODEC_MASK;
					hda_codec* codec = controller->codecs[cad];

					if ((responseFlags & RESPONSE_FLAGS_UNSOLICITED) != 0) {
						dprintf("hda: Unsolicited response: %08lx/%08lx\n",
							response, responseFlags);
						continue;
					}
					if (codec == NULL) {
						dprintf("hda: Response for unknown codec %ld: "
							"%08lx/%08lx\n", cad, response, responseFlags);
						continue;
					}
					if (codec->response_count >= MAX_CODEC_RESPONSES) {
						dprintf("hda: too many responses received for codec %ld"
							": %08lx/%08lx!\n", cad, response, responseFlags);
						continue;
					}

					/* Store response in codec */
					codec->responses[codec->response_count++] = response;
					release_sem_etc(codec->response_sem, 1, B_DO_NOT_RESCHEDULE);
					rc = B_INVOKE_SCHEDULER;
				}			
			}

			if (rirbStatus & RIRBSTS_OIS)
				dprintf("hda: RIRB Overflow\n");
		}

		/* Check for sending errors */
		if (corbStatus) {
			REG8(controller, CORBSTS) = corbStatus;

			if (corbStatus & CORBSTS_MEI)
				dprintf("hda: CORB Memory Error!\n");
		}
	}

	if (intsts & ~(INTSTS_CIS | INTSTS_GIS)) {
		for (uint32 index = 0; index < HDA_MAX_STREAMS; index++) {
			if ((intsts & (1 << index)) != 0) {
				if (controller->streams[index]) {
					stream_handle_interrupt(controller,
						controller->streams[index]);
				} else {
					dprintf("hda: Stream interrupt for unconfigured stream "
						"%ld!\n", index);
				}
			}
		}
	}

	/* NOTE: See HDA001 => CIS/GIS cannot be cleared! */

	return rc;
}


static status_t
hda_hw_start(hda_controller* controller)
{
	int timeout = 10;

	// TODO: reset controller first? (we currently reset on uninit only)

	/* Put controller out of reset mode */
	REG32(controller, GCTL) |= GCTL_CRST;

	do {
		snooze(100);
	} while (--timeout && !(REG32(controller, GCTL) & GCTL_CRST));	

	return timeout ? B_OK : B_TIMED_OUT;
}


/*! Allocates and initializes the Command Output Ring Buffer (CORB), and
	Response Input Ring Buffer (RIRB) to the maximum supported size, and also
	the DMA position buffer.

	Programs the controller hardware to make use of these buffers (the DMA
	positioning is actually enabled in hda_stream_setup_buffers()).
*/
static status_t
init_corb_rirb_pos(hda_controller* controller)
{
	uint32 memSize, rirbOffset, posOffset;
	uint8 corbSize, rirbSize, posSize;
	status_t rc = B_OK;
	physical_entry pe;

	/* Determine and set size of CORB */
	corbSize = REG8(controller, CORBSIZE);
	if (corbSize & CORBSIZE_CAP_256E) {
		controller->corb_length = 256;
		REG8(controller, CORBSIZE) = CORBSIZE_SZ_256E;
	} else if (corbSize & CORBSIZE_CAP_16E) {
		controller->corb_length = 16;
		REG8(controller, CORBSIZE) = CORBSIZE_SZ_16E;
	} else if (corbSize & CORBSIZE_CAP_2E) {
		controller->corb_length = 2;
		REG8(controller, CORBSIZE) = CORBSIZE_SZ_2E;
	}

	/* Determine and set size of RIRB */
	rirbSize = REG8(controller, RIRBSIZE);
	if (rirbSize & RIRBSIZE_CAP_256E) {
		controller->rirb_length = 256;
		REG8(controller, RIRBSIZE) = RIRBSIZE_SZ_256E;
	} else if (rirbSize & RIRBSIZE_CAP_16E) {
		controller->rirb_length = 16;
		REG8(controller, RIRBSIZE) = RIRBSIZE_SZ_16E;
	} else if (rirbSize & RIRBSIZE_CAP_2E) {
		controller->rirb_length = 2;
		REG8(controller, RIRBSIZE) = RIRBSIZE_SZ_2E;
	}

	/* Determine rirb offset in memory and total size of corb+alignment+rirb */
	rirbOffset = (controller->corb_length * sizeof(corb_t) + 0x7f) & ~0x7f;
	posOffset = (rirbOffset + controller->rirb_length * sizeof(rirb_t) + 0x7f)
		& 0x7f;
	posSize = 8 * (controller->num_input_streams
		+ controller->num_output_streams + controller->num_bidir_streams);

	memSize = (posOffset + posSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	/* Allocate memory area */
	controller->corb_rirb_pos_area = create_area("hda corb/rirb/pos",
		(void**)&controller->corb, B_ANY_KERNEL_ADDRESS, memSize,
		B_CONTIGUOUS, 0);
	if (controller->corb_rirb_pos_area < 0)
		return controller->corb_rirb_pos_area;

	/* Rirb is after corb+aligment */
	controller->rirb = (rirb_t*)(((uint8*)controller->corb) + rirbOffset);

	if ((rc = get_memory_map(controller->corb, memSize, &pe, 1)) != B_OK) {
		delete_area(controller->corb_rirb_pos_area);
		return rc;
	}

	/* Program CORB/RIRB for these locations */
	REG32(controller, CORBLBASE) = (uint32)pe.address;
	REG32(controller, CORBUBASE) = 0;
	REG32(controller, RIRBLBASE) = (uint32)pe.address + rirbOffset;
	REG32(controller, RIRBUBASE) = 0;

	/* Program DMA position update */
	REG32(controller, DMA_POSITION_BASE_LOWER) = (uint32)pe.address + posOffset;
	REG32(controller, DMA_POSITION_BASE_UPPER) = 0;

	controller->stream_positions = (uint32*)
		((uint8*)controller->corb + posOffset);

	/* Reset CORB read pointer */
	/* NOTE: See HDA011 for corrected procedure! */
	REG16(controller, CORBRP) = CORBRP_RST;
	do {
		spin(10);
	} while ( !(REG16(controller, CORBRP) & CORBRP_RST) );
	REG16(controller, CORBRP) = 0;

	/* Reset RIRB write pointer */
	REG16(controller, RIRBWP) = RIRBWP_RST;

	/* Generate interrupt for every response */
	REG16(controller, RINTCNT) = 1;

	/* Setup cached read/write indices */
	controller->rirb_read_pos = 1;
	controller->corb_write_pos = 0;

	/* Gentlemen, start your engines... */
	REG8(controller, CORBCTL) = CORBCTL_RUN | CORBCTL_MEIE;
	REG8(controller, RIRBCTL) = RIRBCTL_DMAEN | RIRBCTL_OIC | RIRBCTL_RINTCTL;

	return B_OK;
}


//	#pragma mark - public stream functions


void
hda_stream_delete(hda_stream* stream)
{
	if (stream->buffer_ready_sem >= B_OK)
		delete_sem(stream->buffer_ready_sem);

	if (stream->buffer_area >= B_OK)
		delete_area(stream->buffer_area);

	if (stream->buffer_descriptors_area >= B_OK)
		delete_area(stream->buffer_descriptors_area);

	free(stream);
}


hda_stream*
hda_stream_new(hda_audio_group* audioGroup, int type)
{
	hda_controller* controller = audioGroup->codec->controller;

	hda_stream* stream = (hda_stream*)calloc(1, sizeof(hda_stream));
	if (stream == NULL)
		return NULL;

	stream->buffer_ready_sem = B_ERROR;
	stream->buffer_area = B_ERROR;
	stream->buffer_descriptors_area = B_ERROR;
	stream->type = type;

	switch (type) {
		case STREAM_PLAYBACK:
			stream->id = 1;
			stream->off = controller->num_input_streams * HDAC_SDSIZE;
			controller->streams[controller->num_input_streams] = stream;
			break;

		case STREAM_RECORD:
			stream->id = 2;
			stream->off = 0;
			controller->streams[0] = stream;
			break;

		default:
			dprintf("%s: Unknown stream type %d!\n", __func__, type);
			free(stream);
			stream = NULL;
			break;
	}

	// find I/O and Pin widgets for this stream

	if (hda_audio_group_get_widgets(audioGroup, stream) == B_OK)
		return stream;

	free(stream);
	return NULL;
}


/*!	Starts a stream's DMA engine, and enables generating and receiving
	interrupts for this stream.
*/
status_t
hda_stream_start(hda_controller* controller, hda_stream* stream)
{
	stream->buffer_ready_sem = create_sem(0,
		stream->type == STREAM_PLAYBACK ? "hda_playback_sem" : "hda_record_sem");
	if (stream->buffer_ready_sem < B_OK)
		return stream->buffer_ready_sem;

	REG32(controller, INTCTL) |= 1 << (stream->off / HDAC_SDSIZE);
	OREG8(controller, stream->off, CTL0) |= CTL0_IOCE | CTL0_FEIE | CTL0_DEIE
		| CTL0_RUN;

#if 0
	while (!(OREG8(controller, stream->off, CTL0) & CTL0_RUN))
		snooze(1);
#endif

	stream->running = true;

	return B_OK;
}


/*!	Stops the stream's DMA engine, and turns off interrupts for this
	stream.
*/
status_t
hda_stream_stop(hda_controller* controller, hda_stream* stream)
{
	OREG8(controller, stream->off, CTL0) &= ~(CTL0_IOCE | CTL0_FEIE | CTL0_DEIE
		| CTL0_RUN);
	REG32(controller, INTCTL) &= ~(1 << (stream->off / HDAC_SDSIZE));

#if 0
	while ((OREG8(controller, stream->off, CTL0) & CTL0_RUN) != 0)
		snooze(1);
#endif

	stream->running = false;
	delete_sem(stream->buffer_ready_sem);
	stream->buffer_ready_sem = -1;

	return B_OK;
}


status_t
hda_stream_setup_buffers(hda_audio_group* audioGroup, hda_stream* stream,
	const char* desc)
{
	uint32 bufferSize, bufferPhysicalAddress, alloc;
	uint32 response[2], index;
	physical_entry pe;
	bdl_entry_t* bufferDescriptors;
	corb_t verb[2];
	uint8* buffer;
	status_t rc;
	uint16 format;

	/* Clear previously allocated memory */
	if (stream->buffer_area >= B_OK) {
		delete_area(stream->buffer_area);
		stream->buffer_area = B_ERROR;
	}

	if (stream->buffer_descriptors_area >= B_OK) {
		delete_area(stream->buffer_descriptors_area);
		stream->buffer_descriptors_area = B_ERROR;
	}

	/* Calculate size of buffer (aligned to 128 bytes) */		
	bufferSize = stream->sample_size * stream->num_channels
		* stream->buffer_length;
	bufferSize = (bufferSize + 127) & (~127);
dprintf("HDA: sample size %ld, num channels %ld, buffer length %ld ****************\n",
	stream->sample_size, stream->num_channels, stream->buffer_length);

	/* Calculate total size of all buffers (aligned to size of B_PAGE_SIZE) */
	alloc = bufferSize * stream->num_buffers;
	alloc = (alloc + B_PAGE_SIZE - 1) & (~(B_PAGE_SIZE -1));

	/* Allocate memory for buffers */
	stream->buffer_area = create_area("hda buffers", (void**)&buffer,
		B_ANY_KERNEL_ADDRESS, alloc, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (stream->buffer_area < B_OK)
		return stream->buffer_area;

	/* Get the physical address of memory */
	rc = get_memory_map(buffer, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(stream->buffer_area);
		return rc;
	}
	
	bufferPhysicalAddress = (uint32)pe.address;

	dprintf("%s(%s): Allocated %lu bytes for %ld buffers\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Store pointers (both virtual/physical) */	
	for (index = 0; index < stream->num_buffers; index++) {
		stream->buffers[index] = buffer + (index * bufferSize);
		stream->physical_buffers[index] = bufferPhysicalAddress
			+ (index * bufferSize);
	}

	/* Now allocate BDL for buffer range */
	alloc = stream->num_buffers * sizeof(bdl_entry_t);
	alloc = (alloc + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	stream->buffer_descriptors_area = create_area("hda buffer descriptors",
		(void**)&bufferDescriptors, B_ANY_KERNEL_ADDRESS, alloc,
		B_CONTIGUOUS, 0);
	if (stream->buffer_descriptors_area < B_OK) {
		delete_area(stream->buffer_area);
		return stream->buffer_descriptors_area;
	}

	/* Get the physical address of memory */
	rc = get_memory_map(bufferDescriptors, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(stream->buffer_area);
		delete_area(stream->buffer_descriptors_area);
		return rc;
	}

	stream->physical_buffer_descriptors = (uint32)pe.address;

	dprintf("%s(%s): Allocated %ld bytes for %ld BDLEs\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Setup buffer descriptor list (BDL) entries */	
	for (index = 0; index < stream->num_buffers; index++, bufferDescriptors++) {
		bufferDescriptors->address = stream->physical_buffers[index];
		bufferDescriptors->length = bufferSize;
		bufferDescriptors->ioc = 1;
			// we want an interrupt after every buffer
	}

	/* Configure stream registers */
	format = stream->num_channels - 1;
	switch (stream->sample_format) {
		case B_FMT_8BIT_S:	format |= FMT_8BIT; stream->bps = 8; break;
		case B_FMT_16BIT:	format |= FMT_16BIT; stream->bps = 16; break;
		case B_FMT_20BIT:	format |= FMT_20BIT; stream->bps = 20; break;
		case B_FMT_24BIT:	format |= FMT_24BIT; stream->bps = 24; break;
		case B_FMT_32BIT:	format |= FMT_32BIT; stream->bps = 32; break;

		default:
			dprintf("%s: Invalid sample format: 0x%lx\n", __func__,
				stream->sample_format);
			break;
	}

	for (index = 0; index < sizeof(kRates) / sizeof(kRates[0]); index++) {
		if (kRates[index].multi_rate == stream->sample_rate) {
			format |= kRates[index].hw_rate;
			stream->rate = kRates[index].rate;
			break;
		}
	}

	dprintf("IRA: %s: setup stream %ld: SR=%ld, SF=%ld\n", __func__, stream->id,
		stream->rate, stream->bps);

	OREG16(audioGroup->codec->controller, stream->off, FMT) = format;
	OREG32(audioGroup->codec->controller, stream->off, BDPL)
		= stream->physical_buffer_descriptors;
	OREG32(audioGroup->codec->controller, stream->off, BDPU) = 0;
	OREG16(audioGroup->codec->controller, stream->off, LVI)
		= stream->num_buffers - 1;
	/* total cyclic buffer size in _bytes_ */
	OREG32(audioGroup->codec->controller, stream->off, CBL)
		= stream->sample_size * stream->num_channels * stream->num_buffers
		* stream->buffer_length;
	OREG8(audioGroup->codec->controller, stream->off, CTL2) = stream->id << 4;

	REG32(audioGroup->codec->controller, DMA_POSITION_BASE_LOWER)
		|= DMA_POSITION_ENABLED;

	for (uint32 i = 0; i < stream->num_io_widgets; i++) {
		verb[0] = MAKE_VERB(audioGroup->codec->addr, stream->io_widgets[i],
			VID_SET_CONVERTER_FORMAT, format);
		verb[1] = MAKE_VERB(audioGroup->codec->addr, stream->io_widgets[i],
			VID_SET_CONVERTER_STREAM_CHANNEL, stream->id << 4);
		hda_send_verbs(audioGroup->codec, verb, response, 2);
	}

	snooze(1000);
	return B_OK;
}


//	#pragma mark - public controller functions


status_t
hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses, uint32 count)
{
	hda_controller *controller = codec->controller;
	uint32 sent = 0;
	status_t rc;

	codec->response_count = 0;

	while (sent < count) {
		uint32 readPos = REG16(controller, CORBRP);
		uint32 queued = 0;

		while (sent < count) {
			uint32 writePos = next_corb(controller);

			if (writePos == readPos) {
				// There is no space left in the ring buffer; execute the
				// queued commands and wait until
				break;
			}

			controller->corb[writePos] = verbs[sent++];
			controller->corb_write_pos = writePos;
			queued++;
		}

		REG16(controller, CORBWP) = controller->corb_write_pos;
		rc = acquire_sem_etc(codec->response_sem, queued, B_RELATIVE_TIMEOUT,
			1000ULL * 50);
		if (rc < B_OK)
			return rc;
	}

	if (responses != NULL)
		memcpy(responses, codec->responses, count * sizeof(uint32));

	return B_OK;
}


/*! Setup hardware for use; detect codecs; etc */
status_t
hda_hw_init(hda_controller* controller)
{
	status_t rc;
	uint16 gcap;
	uint32 index;

	/* Map MMIO registers */
	controller->regs_area = map_physical_memory("hda_hw_regs",
		(void*)controller->pci_info.u.h0.base_registers[0],
		controller->pci_info.u.h0.base_register_sizes[0], B_ANY_KERNEL_ADDRESS,
		0, (void**)&controller->regs);
	if (controller->regs_area < B_OK) {
		rc = controller->regs_area;
		goto error;
	}

	/* Absolute minimum hw is online; we can now install interrupt handler */
	controller->irq = controller->pci_info.u.h0.interrupt_line;
	rc = install_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller, 0);
	if (rc != B_OK)
		goto no_irq;

	/* show some hw features */
	gcap = REG16(controller, GCAP);
	dprintf("HDA: HDA v%d.%d, O:%d/I:%d/B:%d, #SDO:%d, 64bit:%s\n", 
		REG8(controller, VMAJ), REG8(controller, VMIN),
		GCAP_OSS(gcap), GCAP_ISS(gcap), GCAP_BSS(gcap),
		GCAP_NSDO(gcap) ? GCAP_NSDO(gcap) *2 : 1, 
		gcap & GCAP_64OK ? "yes" : "no" );

	controller->num_input_streams = GCAP_OSS(gcap);
	controller->num_output_streams = GCAP_ISS(gcap);
	controller->num_bidir_streams = GCAP_BSS(gcap);

	/* Get controller into valid state */
	rc = hda_hw_start(controller);
	if (rc != B_OK)
		goto reset_failed;

	/* Setup CORB/RIRB/DMA POS */
	rc = init_corb_rirb_pos(controller);
	if (rc != B_OK)
		goto corb_rirb_failed;

	REG16(controller, WAKEEN) = 0x7fff;

	/* Enable controller interrupts */
	REG32(controller, INTCTL) = INTCTL_GIE | INTCTL_CIE;

	/* Wait for codecs to warm up */
	snooze(1000);

	if (!controller->codec_status) {	
		rc = ENODEV;
		goto corb_rirb_failed;
	}

	for (index = 0; index < HDA_MAX_CODECS; index++) {
		if ((controller->codec_status & (1 << index)) != 0)
			hda_codec_new(controller, index);
	}

	for (index = 0; index < HDA_MAX_CODECS; index++) {
		if (controller->codecs[index]
			&& controller->codecs[index]->num_audio_groups > 0) {
			controller->active_codec = controller->codecs[index];
			break;
		}
	}

	if (controller->active_codec != NULL)
		return B_OK;

	rc = ENODEV;

corb_rirb_failed:
	REG32(controller, INTCTL) = 0;

reset_failed:
	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

no_irq:
	delete_area(controller->regs_area);
	controller->regs_area = B_ERROR;
	controller->regs = NULL;

error:
	dprintf("ERROR: %s(%ld)\n", strerror(rc), rc);

	return rc;
}


/*! Stop any activity */
void
hda_hw_stop(hda_controller* controller)
{
	int index;

	/* Stop all audio streams */
	for (index = 0; index < HDA_MAX_STREAMS; index++) {
		if (controller->streams[index] && controller->streams[index]->running)
			hda_stream_stop(controller, controller->streams[index]);
	}
}


/*! Free resources */
void
hda_hw_uninit(hda_controller* controller)
{
	uint32 index;

	if (controller == NULL)
		return;

	/* Stop all audio streams */
	hda_hw_stop(controller);

	/* Stop CORB/RIRB */
	REG8(controller, CORBCTL) = 0;
	REG8(controller, RIRBCTL) = 0;

	REG32(controller, CORBLBASE) = 0;
	REG32(controller, RIRBLBASE) = 0;
	REG32(controller, DMA_POSITION_BASE_LOWER) = 0;

	/* Disable interrupts, reset controller, and remove interrupt handler */
	REG32(controller, INTCTL) = 0;
	REG32(controller, GCTL) &= ~GCTL_CRST;
	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

	/* Delete corb/rirb area */
	if (controller->corb_rirb_pos_area >= 0) {
		delete_area(controller->corb_rirb_pos_area);
		controller->corb_rirb_pos_area = B_ERROR;
		controller->corb = NULL;
		controller->rirb = NULL;
		controller->stream_positions = NULL;
	}

	/* Unmap registers */
	if (controller->regs_area >= 0) {
		delete_area(controller->regs_area);
		controller->regs_area = B_ERROR;
		controller->regs = NULL;
	}

	/* Now delete all codecs */
	for (index = 0; index < HDA_MAX_CODECS; index++) {
		if (controller->codecs[index] != NULL)
			hda_codec_delete(controller->codecs[index]);
	}
}

