/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <validate_display_mode.h>

#include <string.h>


static uint16
round(uint16 value, uint16 resolution)
{
	return value / resolution * resolution;
}


static void
sanitize_timing(uint16& display, uint16& syncStart, uint16& syncEnd,
	uint16& total, const timing_constraints& constraints)
{
	if (syncStart < display + constraints.min_before_sync)
		syncStart = display + constraints.min_before_sync;
	else if (syncStart > constraints.max_sync_start)
		syncStart = constraints.max_sync_start;

	uint32 syncLength = syncEnd - syncStart;
	if (syncLength < constraints.min_sync_length)
		syncLength = constraints.min_sync_length;
	else if (syncLength > constraints.max_sync_length)
		syncLength = constraints.max_sync_length;

	if (total < syncStart + syncLength + constraints.min_after_sync)
		total = syncStart + syncLength + constraints.min_after_sync;

	if (total > constraints.max_total) {
		total = constraints.max_total;
		syncLength = min_c(syncLength, uint16(total - syncStart));
	}

	syncEnd = round(syncStart + syncLength, constraints.resolution);
	syncStart = round(syncStart, constraints.resolution);
	display = round(display, constraints.resolution);
	total = round(total, constraints.resolution);
}


/*! Makes sure the passed in \a mode fulfills the specified \a constraints.
	Returns whether or not the mode had to be changed.
*/
bool
sanitize_display_mode(display_mode& mode,
	const display_constraints& constraints, const edid1_info* edid)
{
	display_mode originalMode = mode;

	// size

	if (mode.timing.h_display < constraints.min_h_display)
		mode.timing.h_display = constraints.min_h_display;
	else if (mode.timing.h_display > constraints.max_h_display)
		mode.timing.h_display = constraints.max_h_display;

	if (mode.timing.v_display < constraints.min_v_display)
		mode.timing.v_display = constraints.min_v_display;
	else if (mode.timing.v_display > constraints.max_v_display)
		mode.timing.v_display = constraints.max_v_display;

	// horizontal timing

	sanitize_timing(mode.timing.h_display, mode.timing.h_sync_start,
		mode.timing.h_sync_end, mode.timing.h_total,
		constraints.horizontal_timing);

	// vertical timing

	sanitize_timing(mode.timing.v_display, mode.timing.v_sync_start,
		mode.timing.v_sync_end, mode.timing.v_total,
		constraints.vertical_timing);

	// TODO: take EDID and pixel clock into account!

	return memcmp(&mode, &originalMode, sizeof(display_mode)) != 0;
}


bool
is_display_mode_within_bounds(display_mode& mode, const display_mode& low,
	const display_mode& high)
{
	// Check horizontal timing
	if (mode.timing.h_display < low.timing.h_display
		|| mode.timing.h_display > high.timing.h_display
		|| mode.timing.h_sync_start < low.timing.h_sync_start
		|| mode.timing.h_sync_start > high.timing.h_sync_start
		|| mode.timing.h_sync_end < low.timing.h_sync_end
		|| mode.timing.h_sync_end > high.timing.h_sync_end
		|| mode.timing.h_total < low.timing.h_total
		|| mode.timing.h_total > high.timing.h_total)
		return false;

	// Check vertical timing
	if (mode.timing.h_display < low.timing.h_display
		|| mode.timing.h_display > high.timing.h_display
		|| mode.timing.h_sync_start < low.timing.h_sync_start
		|| mode.timing.h_sync_start > high.timing.h_sync_start
		|| mode.timing.h_sync_end < low.timing.h_sync_end
		|| mode.timing.h_sync_end > high.timing.h_sync_end
		|| mode.timing.h_total < low.timing.h_total
		|| mode.timing.h_total > high.timing.h_total)
		return false;

	// Check pixel clock
	if (mode.timing.pixel_clock > high.timing.pixel_clock
		|| mode.timing.pixel_clock < low.timing.pixel_clock)
		return false;

	// Check horizontal size
	if (mode.virtual_width > high.virtual_width
		|| mode.virtual_width < low.virtual_width)
		return false;

	// Check vertical size
	if (mode.virtual_height > high.virtual_height
		|| mode.virtual_height < low.virtual_height)
		return false;

	return true;
}
