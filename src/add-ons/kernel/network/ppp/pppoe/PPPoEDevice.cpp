//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"

#include <core_funcs.h>
#include <cstdlib>

// from libkernelppp
#include <settings_tools.h>
#include <LockerHelper.h>


#if DEBUG
static char sDigits[] = "0123456789ABCDEF";
void
dump_packet(struct mbuf *packet)
{
	if(!packet)
		return;
	
	uint8 *data = mtod(packet, uint8*);
	uint8 buffer[33];
	uint8 bufferIndex = 0;
	
	dprintf("Dumping packet;len=%ld;pkthdr.len=%d\n", packet->m_len,
		packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : -1);
	
	for(uint32 index = 0; index < packet->m_len; index++) {
		buffer[bufferIndex++] = sDigits[data[index] >> 4];
		buffer[bufferIndex++] = sDigits[data[index] & 0x0F];
		if(bufferIndex == 32 || index == packet->m_len - 1) {
			buffer[bufferIndex] = 0;
			dprintf("%s\n", buffer);
			bufferIndex = 0;
		}
	}
}
#endif


PPPoEDevice::PPPoEDevice(KPPPInterface& interface, driver_parameter *settings)
	: KPPPDevice("PPPoE", PPPoE_HEADER_SIZE + ETHER_HDR_LEN, interface, settings),
	fEthernetIfnet(NULL),
	fSessionID(0),
	fHostUniq(NewHostUniq()),
	fACName(NULL),
	fServiceName(NULL),
	fAttempts(0),
	fNextTimeout(0),
	fState(INITIAL)
{
#if DEBUG
	dprintf("PPPoEDevice: Constructor\n");
	if(!settings || !settings->parameters)
		dprintf("PPPoEDevice::ctor: No settings!\n");
#endif
	
	interface.SetPFCOptions(PPP_ALLOW_PFC);
		// we do not want to fail just because the other side requests PFC
	
	memset(fPeer, 0xFF, sizeof(fPeer));
	SetMTU(1494);
		// MTU size does not contain PPP header
	
	// find ethernet device
	const char *interfaceName = get_parameter_value(PPPoE_INTERFACE_KEY, settings);
	if(!interfaceName)
		return;
#if DEBUG
	dprintf("PPPoEDevice::ctor: interfaceName: %s\n", interfaceName);
#endif
	
	ifnet *current = get_interfaces();
	for(; current; current = current->if_next) {
		if(current->if_type == IFT_ETHER && current->if_name
				&& !strcmp(current->if_name, interfaceName)) {
#if DEBUG
			dprintf("PPPoEDevice::ctor: found ethernet interface\n");
#endif
			fEthernetIfnet = current;
			break;
		}
	}
	
#if DEBUG
	if(!fEthernetIfnet)
		dprintf("PPPoEDevice::ctor: could not find ethernet interface\n");
#endif
	
	add_device(this);
}


PPPoEDevice::~PPPoEDevice()
{
#if DEBUG
	dprintf("PPPoEDevice: Destructor\n");
#endif
	
	remove_device(this);
	
	free(fACName);
	free(fServiceName);
}


status_t
PPPoEDevice::InitCheck() const
{
	return EthernetIfnet() && EthernetIfnet()->output
		&& KPPPDevice::InitCheck() == B_OK ? B_OK : B_ERROR;
}


bool
PPPoEDevice::Up()
{
#if DEBUG
	dprintf("PPPoEDevice: Up()\n");
#endif
	
	if(InitCheck() != B_OK)
		return false;
	
	LockerHelper locker(fLock);
	
	if(IsUp())
		return true;
	
	fState = INITIAL;
		// reset state
	
	if(fAttempts > PPPoE_MAX_ATTEMPTS) {
		fAttempts = 0;
		return false;
	}
	
	++fAttempts;
	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));
	
	// create PADI
	DiscoveryPacket discovery(PADI);
	if(fServiceName)
		discovery.AddTag(SERVICE_NAME, strlen(fServiceName), fServiceName);
	else
		discovery.AddTag(SERVICE_NAME, 0, NULL);
	discovery.AddTag(HOST_UNIQ, sizeof(fHostUniq), &fHostUniq);
	discovery.AddTag(END_OF_LIST, 0, NULL);
	
	// set up PPP header
	struct mbuf *packet = discovery.ToMbuf(MTU());
	if(!packet)
		return false;
	
	// create destination
	struct ether_header *ethernetHeader;
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	ethernetHeader = (struct ether_header*) destination.sa_data;
	ethernetHeader->ether_type = ETHERTYPE_PPPOEDISC;
	memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
	
	// check if we are allowed to go up now (user intervention might disallow that)
	if(fAttempts > 0 && !UpStarted()) {
		fAttempts = 0;
		DownEvent();
		return true;
			// there was no error
	}
	
	fState = PADI_SENT;
		// needed before sending, otherwise we might not get all packets
	
	if(EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL) != B_OK) {
		fState = INITIAL;
		fAttempts = 0;
		dprintf("PPPoEDevice::Up(): EthernetIfnet()->output() failed!\n");
		return false;
	}
	
	fNextTimeout = system_time() + PPPoE_TIMEOUT;
	
	return true;
}


bool
PPPoEDevice::Down()
{
#if DEBUG
	dprintf("PPPoEDevice: Down()\n");
#endif
	
	if(InitCheck() != B_OK)
		return false;
	
	LockerHelper locker(fLock);
	
	fState = INITIAL;
	fAttempts = 0;
	fNextTimeout = 0;
		// disable timeouts
	
	if(!IsUp()) {
		DownEvent();
		return true;
	}
	
	DownStarted();
		// this tells StateMachine that DownEvent() does not mean we lost connection
	
	// create PADT
	DiscoveryPacket discovery(PADT, SessionID());
	discovery.AddTag(END_OF_LIST, 0, NULL);
	
	struct mbuf *packet = discovery.ToMbuf(MTU());
	if(!packet) {
		dprintf("PPPoEDevice::Down(): ToMbuf() failed; MTU=%ld\n", MTU());
		DownEvent();
		return false;
	}
	
	// create destination
	struct ether_header *ethernetHeader;
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	ethernetHeader = (struct ether_header*) destination.sa_data;
	ethernetHeader->ether_type = ETHERTYPE_PPPOEDISC;
	memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
	
	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));
	
	EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL);
	DownEvent();
	
	return true;
}


uint32
PPPoEDevice::InputTransferRate() const
{
	return 10000000;
}


uint32
PPPoEDevice::OutputTransferRate() const
{
	return 10000000;
}


uint32
PPPoEDevice::CountOutputBytes() const
{
	// TODO:
	// ?look through ethernet queue for outgoing pppoe packets coming from our device?
	return 0;
}


status_t
PPPoEDevice::Send(struct mbuf *packet, uint16 protocolNumber = 0)
{
	// Send() is only for PPP packets. PPPoE packets are sent directly to ethernet.
	
#if DEBUG
	dprintf("PPPoEDevice: Send()\n");
#endif
	
	if(!packet)
		return B_ERROR;
	else if(InitCheck() != B_OK || protocolNumber != 0) {
		dprintf("PPPoEDevice::Send(): InitCheck() != B_OK!\n");
		m_freem(packet);
		return B_ERROR;
	}
	
	LockerHelper locker(fLock);
	
	if(!IsUp()) {
		dprintf("PPPoEDevice::Send(): no connection!\n");
		m_freem(packet);
		return PPP_NO_CONNECTION;
	}
	
	uint16 length = packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : packet->m_len;
	
	// encapsulate packet into pppoe header
	M_PREPEND(packet, PPPoE_HEADER_SIZE);
	pppoe_header *header = mtod(packet, pppoe_header*);
	header->version = PPPoE_VERSION;
	header->type = PPPoE_TYPE;
	header->code = 0x00;
	header->sessionID = SessionID();
	header->length = htons(length);
	
	// create destination
	struct ether_header *ethernetHeader;
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	ethernetHeader = (struct ether_header*) destination.sa_data;
	ethernetHeader->ether_type = ETHERTYPE_PPPOE;
	memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
	
	locker.UnlockNow();
	
	if(EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL) != B_OK) {
		dprintf("PPPoEDevice::Send(): EthernetIfnet()->output() failed!\n");
		DownEvent();
			// DownEvent() without DownStarted() indicates connection lost
		return PPP_NO_CONNECTION;
	}
	
	return B_OK;
}


status_t
PPPoEDevice::Receive(struct mbuf *packet, uint16 protocolNumber = 0)
{
	if(!packet)
		return B_ERROR;
	else if(InitCheck() != B_OK || IsDown()) {
		m_freem(packet);
		return B_ERROR;
	}
	
	complete_pppoe_header *completeHeader = mtod(packet, complete_pppoe_header*);
	if(!completeHeader) {
		m_freem(packet);
		return B_ERROR;
	}
	
	uint8 ethernetSource[6];
	memcpy(ethernetSource, completeHeader->ethernetHeader.ether_shost, sizeof(fPeer));
	status_t result = B_OK;
	
	if(completeHeader->ethernetHeader.ether_type == ETHERTYPE_PPPOE) {
		m_adj(packet, ETHER_HDR_LEN);
		pppoe_header *header = mtod(packet, pppoe_header*);
		
		if(!IsUp() || header->version != PPPoE_VERSION || header->type != PPPoE_TYPE
				|| header->code != 0x0 || header->sessionID != SessionID()) {
			m_freem(packet);
			return B_ERROR;
		}
		
		m_adj(packet, PPPoE_HEADER_SIZE);
		return Interface().ReceiveFromDevice(packet);
	} else if(completeHeader->ethernetHeader.ether_type == ETHERTYPE_PPPOEDISC) {
		m_adj(packet, ETHER_HDR_LEN);
		pppoe_header *header = mtod(packet, pppoe_header*);
		
		// we do not need to check HOST_UNIQ tag as this is done in pppoe.cpp
		if(header->version != PPPoE_VERSION || header->type != PPPoE_TYPE) {
			m_freem(packet);
			return B_ERROR;
		}
		
		LockerHelper locker(fLock);
		
		if(IsDown()) {
			m_freem(packet);
			return B_ERROR;
		}
		
		DiscoveryPacket discovery(packet);
		switch(discovery.Code()) {
			case PADO: {
				if(fState != PADI_SENT) {
					m_freem(packet);
					return B_OK;
				}
				
				bool hasServiceName = false;
				pppoe_tag *tag;
				DiscoveryPacket reply(PADR);
				for(int32 index = 0; index < discovery.CountTags(); index++) {
					tag = discovery.TagAt(index);
					switch(tag->type) {
						case SERVICE_NAME:
							if(!hasServiceName && (!fServiceName
									|| !memcmp(tag->data, fServiceName, tag->length))) {
								hasServiceName = true;
								reply.AddTag(tag->type, tag->length, tag->data);
							}
						break;
						
						case AC_COOKIE:
						case RELAY_SESSION_ID:
							reply.AddTag(tag->type, tag->length, tag->data);
						break;
						
						case SERVICE_NAME_ERROR:
						case AC_SYSTEM_ERROR:
						case GENERIC_ERROR:
							m_freem(packet);
							return B_ERROR;
						break;
						
						default:
							;
					}
				}
				
				if(!hasServiceName) {
					m_freem(packet);
					return B_ERROR;
				}
				
				reply.AddTag(HOST_UNIQ, sizeof(fHostUniq), &fHostUniq);
				reply.AddTag(END_OF_LIST, 0, NULL);
				struct mbuf *replyPacket = reply.ToMbuf(MTU());
				if(!replyPacket) {
					m_freem(packet);
					return B_ERROR;
				}
				
				memcpy(fPeer, ethernetSource, sizeof(fPeer));
				
				// create destination
				struct ether_header *ethernetHeader;
				struct sockaddr destination;
				memset(&destination, 0, sizeof(destination));
				destination.sa_family = AF_UNSPEC;
					// raw packet with ethernet header
				ethernetHeader = (struct ether_header*) destination.sa_data;
				ethernetHeader->ether_type = ETHERTYPE_PPPOEDISC;
				memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
				
				fState = PADR_SENT;
				
				if(EthernetIfnet()->output(EthernetIfnet(), replyPacket, &destination,
						NULL) != B_OK) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fNextTimeout = system_time() + PPPoE_TIMEOUT;
			} break;
			
			case PADS:
				if(fState != PADR_SENT
						|| memcmp(ethernetSource, fPeer, sizeof(fPeer))) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fSessionID = header->sessionID;
				fState = OPENED;
				fNextTimeout = 0;
				UpEvent();
			break;
			
			case PADT:
				if(!IsUp()
						|| memcmp(ethernetSource, fPeer, sizeof(fPeer))
						|| header->sessionID != SessionID()) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fState = INITIAL;
				fAttempts = 0;
				fSessionID = 0;
				fNextTimeout = 0;
				DownEvent();
			break;
			
			default:
				m_freem(packet);
				return B_ERROR;
		}
	} else
		result = B_ERROR;
	
	m_freem(packet);
	return result;
}


void
PPPoEDevice::Pulse()
{
	// We use Pulse() for timeout of connection establishment.
	
	if(fNextTimeout == 0 || IsUp() || IsDown())
		return;
	
	LockerHelper locker(fLock);
	
	// check if timed out
	if(system_time() >= fNextTimeout) {
		if(!Up())
			UpFailedEvent();
	}
}
