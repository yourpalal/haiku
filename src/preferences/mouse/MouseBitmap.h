/*
 * Copyright 2003-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Andrew McCall <mccall@digitalparadise.co.uk>
 */

const int32 kMouseWidth = 57;
const int32 kMouseHeight = 82;
const color_space kMouseColorSpace = B_CMAP8;

const unsigned char kMouseBits [] = {
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x0f,0x1b,0x1b,0x0b,0x17,0x17,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x0f,0x1b,0x1b,0x0b,0x17,0x17,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0b,0x17,0x17,0x17,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x0f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x0b,0x17,0x17,0x17,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x0f,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x0b,
	0x0b,0x17,0x17,0x17,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x0f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x0b,0x17,0x17,
	0x17,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,0x1b,0x0f,0x0f,0x19,0x19,0x19,0x19,0x19,
	0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,
	0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,
	0x19,0x19,0x19,0x19,0x0b,0x0b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,
	0x1b,0x1b,0x1b,0x0f,0x0f,0x19,0x19,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1d,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1c,0x1e,
	0x1c,0x1c,0x0b,0x0b,0x17,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x0f,0x19,
	0x19,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1e,
	0x1e,0x1d,0x1e,0x1e,0x1e,0x1c,0x1d,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1b,0x1a,0x1a,
	0x0b,0x17,0x17,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x0f,0x19,0x1d,0x1e,0x1e,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x1b,0x1a,0x19,0x17,0x0b,0x17,0x17,
	0x1b,0x3f,0x3f,0x3f,0x1b,0x0f,0x19,0x1d,0x1e,0x1e,0x3f,0x1e,0x1e,0x1d,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1d,
	0x1e,0x1e,0x1e,0x1d,0x1c,0x1e,0x1e,0x1e,0x1d,0x1c,0x1e,0x1c,0x1c,0x1c,0x1c,0x1b,
	0x1c,0x1b,0x1b,0x1a,0x1a,0x19,0x19,0x17,0x18,0x0b,0x17,0x17,0x1b,0x3f,0x3f,0x3f,
	0x1b,0x0f,0x19,0x1e,0x1d,0x3f,0x1d,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1d,0x1e,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1d,0x1c,0x1e,
	0x1c,0x1e,0x1e,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,
	0x1a,0x19,0x17,0x17,0x16,0x15,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,
	0x1e,0x3f,0x1d,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1d,0x1e,0x1e,0x1e,0x1d,0x1e,
	0x1e,0x1e,0x1d,0x1e,0x1e,0x1c,0x1e,0x1d,0x1e,0x1e,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
	0x1c,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1a,0x1b,0x1a,0x1a,0x1a,0x19,0x19,0x17,0x15,
	0x16,0x15,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1e,0x3f,0x1d,0x1c,
	0x1e,0x1e,0x1e,0x1c,0x1d,0x1e,0x1e,0x1e,0x1d,0x1c,0x1e,0x1e,0x1e,0x1d,0x1e,0x1c,
	0x1e,0x1c,0x1e,0x1d,0x1c,0x1c,0x1e,0x1c,0x1b,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1c,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x15,0x13,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1e,0x1e,0x1d,0x3f,0x1d,0x1e,0x1e,0x1e,0x1c,0x1d,
	0x1e,0x1e,0x1c,0x1c,0x1c,0x1c,0x1e,0x1d,0x1e,0x1c,0x1c,0x1e,0x1e,0x1d,0x1e,0x1c,
	0x1c,0x1c,0x1c,0x1b,0x1c,0x1c,0x1c,0x1b,0x1b,0x1c,0x1b,0x1a,0x1b,0x1b,0x1a,0x1a,
	0x1a,0x19,0x19,0x19,0x17,0x18,0x15,0x15,0x15,0x13,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1d,0x1e,0x1e,0x3f,0x1c,0x1c,0x1c,0x1c,0x1d,0x1e,0x1e,0x1e,0x1c,0x1c,
	0x1c,0x1c,0x1d,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1c,
	0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1b,0x1a,0x1a,0x19,0x19,0x19,0x18,0x17,
	0x17,0x17,0x17,0x16,0x13,0x12,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1e,0x1d,
	0x1e,0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1d,0x1c,0x1e,0x1c,0x1c,0x1c,0x1c,0x1e,
	0x1c,0x1c,0x1c,0x1c,0x1c,0x1b,0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,
	0x1b,0x1a,0x1a,0x1b,0x1a,0x1a,0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x17,0x15,0x15,
	0x15,0x13,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1e,0x3f,0x1c,0x1c,
	0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
	0x1c,0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,
	0x19,0x1a,0x19,0x19,0x19,0x17,0x17,0x18,0x17,0x15,0x15,0x15,0x13,0x11,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1e,0x1d,0x1e,0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
	0x1c,0x1c,0x1c,0x1c,0x1b,0x1c,0x1c,0x1c,0x1b,0x1b,0x1c,0x1c,0x1c,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,0x19,0x1a,0x19,0x19,
	0x19,0x17,0x17,0x17,0x17,0x16,0x15,0x13,0x15,0x12,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1e,0x1e,0x1d,0x3f,0x1c,0x1c,0x1c,0x1c,0x1b,0x1c,0x1c,0x1c,0x1c,0x1c,
	0x1b,0x1c,0x1c,0x1b,0x1c,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1b,
	0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,0x1a,0x19,0x1a,0x19,0x19,0x19,0x17,0x17,0x18,0x17,
	0x17,0x15,0x15,0x13,0x12,0x11,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,
	0x1e,0x3f,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1c,0x1b,0x1c,
	0x1c,0x1b,0x1c,0x1b,0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1b,0x1a,0x1a,
	0x19,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x17,0x17,0x18,0x17,0x17,0x15,0x15,0x15,0x13,
	0x13,0x12,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1e,0x3f,0x1c,0x1c,
	0x1c,0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1c,0x1b,0x1c,0x1c,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1a,0x1b,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x19,
	0x19,0x19,0x17,0x17,0x18,0x17,0x17,0x15,0x15,0x15,0x15,0x13,0x12,0x11,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1c,0x3f,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1b,
	0x1b,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x1a,0x19,0x19,0x19,0x19,0x19,0x17,0x17,0x17,
	0x18,0x17,0x17,0x17,0x15,0x15,0x15,0x12,0x13,0x11,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1d,0x1e,0x1e,0x1e,0x1c,0x1c,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1a,0x1b,0x1a,0x1a,0x1a,0x1b,0x1a,0x1a,0x1a,
	0x19,0x1a,0x1a,0x1a,0x19,0x19,0x17,0x17,0x18,0x17,0x17,0x18,0x17,0x17,0x15,0x16,
	0x15,0x15,0x15,0x13,0x12,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,
	0x1e,0x1c,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,0x1b,0x1a,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1a,0x1b,0x1a,0x1a,0x1a,0x1a,0x1a,0x19,
	0x19,0x19,0x19,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x13,0x13,0x13,
	0x12,0x11,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1e,0x1c,0x1c,0x1c,
	0x1b,0x1b,0x1b,0x1b,0x1a,0x1a,0x1b,0x1b,0x1a,0x1b,0x1b,0x1a,0x1b,0x1b,0x1b,0x1a,
	0x1a,0x1a,0x1b,0x1a,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x19,0x19,0x17,0x19,0x19,
	0x18,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x13,0x13,0x13,0x11,0x11,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1e,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1a,
	0x1a,0x1a,0x1a,0x1a,0x1a,0x1b,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
	0x1a,0x1a,0x19,0x1a,0x19,0x19,0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x17,0x17,0x17,
	0x17,0x16,0x15,0x15,0x15,0x13,0x13,0x12,0x12,0x11,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1d,0x1e,0x1e,0x1c,0x1c,0x1c,0x1b,0x1a,0x1b,0x1a,0x1a,0x1a,0x1a,0x1a,
	0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x19,0x1a,0x19,
	0x19,0x19,0x19,0x17,0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x15,0x16,0x15,0x15,
	0x15,0x13,0x13,0x13,0x11,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1e,0x1d,
	0x1c,0x1c,0x1b,0x1b,0x1c,0x1b,0x1a,0x1b,0x19,0x1a,0x1a,0x19,0x1a,0x19,0x1a,0x19,
	0x1a,0x1a,0x1a,0x19,0x1a,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x19,0x19,0x19,0x19,
	0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x15,0x15,0x15,0x15,0x14,0x12,0x12,
	0x11,0x11,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1e,0x1c,0x1c,0x1b,
	0x1b,0x1a,0x1b,0x1a,0x1a,0x19,0x19,0x1a,0x19,0x19,0x19,0x19,0x19,0x1a,0x1a,0x1a,
	0x1a,0x1a,0x1a,0x19,0x19,0x1a,0x19,0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x15,0x13,0x13,0x12,0x11,0x11,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1c,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x12,0x11,0x11,0x11,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1d,0x17,0x17,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
	0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
	0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
	0x0b,0x0b,0x10,0x10,0x11,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x17,0x0b,
	0x0b,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x0b,0x0b,
	0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x0b,0x0b,0x1e,0x1e,0x1d,0x1c,0x1b,
	0x1b,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x18,0x19,0x19,0x19,0x19,0x19,0x19,0x17,
	0x19,0x19,0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x16,
	0x17,0x17,0x17,0x15,0x15,0x15,0x15,0x13,0x13,0x18,0x17,0x17,0x0b,0x0b,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x3f,0x1d,0x1c,0x1c,0x1b,0x1b,0x1a,0x1b,0x1a,0x1a,
	0x1a,0x19,0x1a,0x19,0x19,0x18,0x17,0x17,0x19,0x19,0x19,0x18,0x19,0x17,0x19,0x18,
	0x17,0x19,0x17,0x18,0x19,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x15,0x16,0x15,0x15,
	0x15,0x15,0x15,0x15,0x14,0x12,0x11,0x12,0x17,0x17,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1d,0x1e,0x1e,0x1c,0x1c,0x1b,0x1b,0x1b,0x1b,0x1a,0x19,0x1a,0x19,0x19,
	0x17,0x18,0x19,0x19,0x17,0x19,0x17,0x18,0x17,0x19,0x19,0x17,0x19,0x19,0x18,0x17,
	0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x17,0x15,0x16,0x15,0x15,0x15,0x13,
	0x13,0x12,0x12,0x11,0x11,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,
	0x1e,0x1b,0x1b,0x1c,0x1a,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x19,0x17,0x18,0x19,
	0x17,0x17,0x18,0x19,0x17,0x17,0x18,0x17,0x19,0x18,0x19,0x17,0x17,0x18,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x15,0x15,0x13,0x12,0x12,0x11,0x11,
	0x11,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1c,0x1c,0x1c,0x1b,
	0x1a,0x1a,0x1b,0x1a,0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x18,0x17,0x17,0x18,0x17,
	0x17,0x18,0x17,0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x18,0x17,0x17,0x17,0x15,0x16,
	0x15,0x15,0x15,0x15,0x15,0x15,0x14,0x12,0x13,0x13,0x12,0x11,0x11,0x10,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1c,0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,
	0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x18,0x17,
	0x18,0x17,0x17,0x17,0x17,0x18,0x17,0x17,0x17,0x15,0x17,0x15,0x15,0x16,0x15,0x15,
	0x15,0x13,0x13,0x13,0x12,0x12,0x11,0x10,0x11,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1c,0x1c,0x1c,0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,0x19,0x1a,0x19,0x19,0x19,
	0x17,0x17,0x18,0x17,0x17,0x17,0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x18,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x13,
	0x13,0x12,0x12,0x11,0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,
	0x1c,0x1c,0x1b,0x1b,0x1a,0x1b,0x1a,0x19,0x19,0x19,0x19,0x17,0x18,0x17,0x18,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x13,0x12,0x12,0x12,0x10,
	0x11,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1c,0x1c,0x1c,0x1b,0x1b,
	0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x15,0x15,
	0x16,0x15,0x15,0x15,0x15,0x13,0x13,0x12,0x12,0x12,0x12,0x11,0x10,0x10,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1c,0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,
	0x19,0x17,0x19,0x19,0x18,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x15,0x17,0x15,0x16,0x15,0x15,0x15,0x15,
	0x13,0x15,0x13,0x12,0x12,0x12,0x11,0x11,0x11,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1d,0x1c,0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x18,0x17,
	0x17,0x18,0x17,0x17,0x17,0x17,0x15,0x17,0x17,0x15,0x17,0x16,0x17,0x17,0x17,0x17,
	0x17,0x15,0x17,0x15,0x15,0x17,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x12,0x12,
	0x12,0x12,0x11,0x11,0x10,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,
	0x1b,0x1b,0x1b,0x1b,0x1a,0x1a,0x1a,0x19,0x17,0x18,0x17,0x19,0x18,0x17,0x17,0x17,
	0x17,0x17,0x15,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x15,0x17,
	0x17,0x16,0x15,0x15,0x15,0x15,0x15,0x14,0x15,0x13,0x13,0x13,0x12,0x12,0x11,0x10,
	0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1c,0x1c,0x1b,0x1b,0x1b,
	0x1a,0x19,0x19,0x19,0x19,0x19,0x17,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x15,0x15,
	0x15,0x17,0x16,0x15,0x15,0x15,0x16,0x15,0x17,0x15,0x15,0x16,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x14,0x12,0x12,0x12,0x13,0x12,0x12,0x11,0x10,0x0f,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1e,0x1b,0x1b,0x1a,0x1b,0x1a,0x1a,0x19,0x19,
	0x17,0x18,0x17,0x18,0x17,0x17,0x17,0x17,0x15,0x17,0x17,0x15,0x16,0x15,0x15,0x15,
	0x16,0x17,0x15,0x17,0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x13,0x15,
	0x13,0x13,0x12,0x12,0x12,0x12,0x11,0x11,0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1c,0x1d,0x1c,0x1b,0x1a,0x1a,0x1a,0x19,0x19,0x19,0x19,0x18,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,
	0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x13,0x13,0x12,0x12,
	0x12,0x11,0x11,0x10,0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1c,
	0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,0x19,0x19,0x18,0x17,0x17,0x18,0x17,0x17,0x15,0x17,
	0x17,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x13,0x13,0x13,0x12,0x12,0x11,0x11,0x11,0x10,
	0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1c,0x1c,0x1b,0x1a,0x1a,
	0x19,0x19,0x19,0x19,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x15,0x16,0x15,0x15,0x15,
	0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x16,0x15,
	0x15,0x15,0x15,0x15,0x13,0x13,0x12,0x12,0x12,0x12,0x11,0x11,0x10,0x10,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,0x19,0x19,0x18,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x15,
	0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x16,0x15,0x15,0x13,0x13,0x13,
	0x13,0x13,0x13,0x12,0x12,0x11,0x10,0x10,0x10,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1c,0x1c,0x1c,0x1b,0x1b,0x1a,0x19,0x19,0x19,0x17,0x17,0x18,0x17,0x17,
	0x17,0x17,0x17,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x13,0x12,0x12,
	0x12,0x11,0x11,0x10,0x10,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1c,
	0x1b,0x1b,0x1a,0x1a,0x19,0x19,0x17,0x17,0x18,0x17,0x17,0x17,0x17,0x15,0x17,0x16,
	0x15,0x15,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x13,0x13,0x13,0x12,0x12,0x11,0x12,0x11,0x10,
	0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1c,0x1b,0x1b,0x1a,0x1a,
	0x19,0x19,0x19,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x15,0x15,0x16,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x13,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x13,0x13,0x13,0x15,0x12,0x12,0x12,0x11,0x12,0x11,0x11,0x10,0x10,0x10,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1c,0x1b,0x1a,0x1a,0x19,0x19,0x19,0x18,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x15,
	0x13,0x15,0x13,0x13,0x15,0x15,0x15,0x13,0x15,0x15,0x15,0x15,0x15,0x14,0x13,0x13,
	0x13,0x12,0x12,0x12,0x11,0x11,0x10,0x10,0x11,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1c,0x1c,0x1b,0x1a,0x1a,0x19,0x19,0x17,0x19,0x17,0x18,0x17,0x17,0x17,
	0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x13,0x15,0x14,0x13,0x13,
	0x15,0x13,0x13,0x13,0x13,0x13,0x14,0x15,0x13,0x15,0x13,0x13,0x13,0x12,0x12,0x11,
	0x11,0x11,0x11,0x10,0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1c,
	0x1b,0x1a,0x19,0x1a,0x19,0x18,0x17,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x13,0x15,0x13,0x14,0x13,0x13,0x13,0x13,0x15,0x13,0x13,0x15,
	0x15,0x13,0x14,0x13,0x13,0x13,0x13,0x12,0x13,0x13,0x12,0x11,0x12,0x11,0x11,0x10,
	0x10,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1c,0x1b,0x1a,0x1a,0x19,
	0x19,0x17,0x17,0x17,0x18,0x17,0x17,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,
	0x13,0x15,0x15,0x13,0x13,0x13,0x12,0x13,0x13,0x14,0x13,0x13,0x13,0x13,0x13,0x13,
	0x13,0x14,0x13,0x13,0x12,0x12,0x11,0x11,0x12,0x12,0x10,0x10,0x10,0x10,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1d,0x1b,0x1b,0x1a,0x19,0x19,0x18,0x17,0x17,0x17,
	0x17,0x15,0x16,0x17,0x15,0x15,0x15,0x15,0x15,0x13,0x15,0x15,0x14,0x13,0x13,0x13,
	0x13,0x13,0x12,0x13,0x13,0x14,0x13,0x13,0x13,0x12,0x13,0x13,0x13,0x13,0x14,0x13,
	0x13,0x12,0x12,0x12,0x11,0x11,0x11,0x10,0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1b,0x1c,0x1b,0x1a,0x19,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x16,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x15,0x14,0x13,0x13,0x13,0x13,0x13,0x12,
	0x12,0x13,0x13,0x14,0x13,0x12,0x12,0x13,0x13,0x12,0x13,0x13,0x12,0x13,0x12,0x11,
	0x11,0x11,0x11,0x10,0x10,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1b,
	0x1a,0x19,0x19,0x19,0x17,0x17,0x17,0x17,0x15,0x15,0x16,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x13,0x13,0x13,0x13,0x13,0x13,0x12,0x14,0x13,0x13,0x13,0x13,0x12,0x12,
	0x13,0x12,0x12,0x13,0x13,0x12,0x12,0x14,0x12,0x12,0x11,0x12,0x11,0x11,0x10,0x10,
	0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1b,0x1c,0x1a,0x19,0x19,0x17,
	0x17,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x15,
	0x12,0x12,0x13,0x12,0x13,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
	0x13,0x12,0x12,0x12,0x13,0x12,0x12,0x12,0x11,0x10,0x11,0x10,0x10,0x0f,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1a,0x19,0x19,0x17,0x17,0x17,0x17,0x17,0x17,
	0x16,0x15,0x15,0x15,0x15,0x15,0x13,0x15,0x15,0x15,0x14,0x13,0x13,0x12,0x13,0x12,
	0x12,0x13,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
	0x12,0x12,0x12,0x12,0x11,0x11,0x10,0x10,0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1b,0x1b,0x1a,0x19,0x17,0x17,0x17,0x16,0x17,0x15,0x15,0x15,0x15,0x16,
	0x15,0x13,0x15,0x15,0x13,0x13,0x13,0x13,0x12,0x13,0x12,0x13,0x12,0x12,0x12,0x12,
	0x12,0x12,0x12,0x12,0x11,0x11,0x12,0x12,0x12,0x12,0x11,0x11,0x12,0x12,0x11,0x12,
	0x11,0x11,0x11,0x10,0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1c,0x1a,
	0x19,0x19,0x17,0x17,0x17,0x17,0x16,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x13,0x14,
	0x13,0x13,0x12,0x13,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
	0x12,0x11,0x11,0x12,0x11,0x12,0x11,0x11,0x12,0x12,0x12,0x11,0x12,0x11,0x11,0x10,
	0x10,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1b,0x1a,0x19,0x19,0x17,0x17,
	0x15,0x17,0x15,0x16,0x15,0x15,0x15,0x15,0x15,0x13,0x15,0x13,0x13,0x13,0x12,0x14,
	0x12,0x12,0x12,0x12,0x12,0x12,0x11,0x12,0x11,0x12,0x11,0x11,0x12,0x11,0x12,0x12,
	0x11,0x12,0x12,0x12,0x11,0x11,0x11,0x11,0x11,0x10,0x11,0x10,0x10,0x0f,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1b,0x1a,0x19,0x17,0x17,0x17,0x15,0x15,0x16,0x15,
	0x15,0x15,0x13,0x15,0x13,0x13,0x13,0x12,0x12,0x12,0x14,0x12,0x12,0x12,0x12,0x12,
	0x11,0x12,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x10,0x10,0x0f,0x0f,0x10,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x1a,0x1a,0x19,0x19,0x17,0x15,0x17,0x15,0x15,0x15,0x15,0x15,0x15,0x13,
	0x13,0x13,0x14,0x13,0x12,0x12,0x12,0x11,0x12,0x12,0x12,0x12,0x11,0x11,0x11,0x12,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x11,0x11,0x11,0x11,0x11,0x10,
	0x11,0x11,0x10,0x10,0x0f,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1a,0x19,
	0x17,0x17,0x17,0x15,0x15,0x16,0x15,0x15,0x15,0x13,0x13,0x13,0x13,0x13,0x12,0x12,
	0x12,0x12,0x12,0x12,0x11,0x12,0x11,0x11,0x11,0x12,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x10,0x11,0x11,0x10,0x11,0x11,0x11,0x11,0x10,0x11,0x11,0x10,0x10,
	0x0f,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x19,0x1a,0x17,0x17,0x17,0x16,
	0x15,0x15,0x15,0x13,0x15,0x15,0x13,0x13,0x13,0x12,0x13,0x12,0x12,0x11,0x12,0x12,
	0x11,0x12,0x11,0x11,0x11,0x11,0x10,0x11,0x10,0x10,0x11,0x11,0x11,0x10,0x11,0x11,
	0x10,0x10,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,0x10,0x10,0x0b,0x17,
	0x17,0x3f,0x3f,0x3f,0x0f,0x19,0x1a,0x19,0x17,0x15,0x15,0x15,0x15,0x15,0x15,0x13,
	0x12,0x14,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x11,0x11,0x10,
	0x11,0x11,0x11,0x10,0x11,0x11,0x10,0x10,0x0f,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,
	0x0f,0x19,0x19,0x17,0x17,0x16,0x15,0x15,0x15,0x13,0x13,0x12,0x13,0x12,0x12,0x11,
	0x12,0x12,0x12,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x11,0x10,
	0x10,0x10,0x11,0x11,0x10,0x10,0x10,0x10,0x11,0x10,0x11,0x10,0x11,0x10,0x10,0x10,
	0x10,0x10,0x10,0x10,0x10,0x0f,0x0b,0x17,0x17,0x3f,0x3f,0x3f,0x1e,0x0f,0x19,0x17,
	0x17,0x15,0x15,0x15,0x15,0x13,0x13,0x14,0x12,0x12,0x12,0x12,0x12,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,0x11,0x10,0x11,0x10,0x10,0x10,
	0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x0f,
	0x10,0x0b,0x17,0x17,0x1b,0x3f,0x3f,0x3f,0x1b,0x0f,0x17,0x17,0x16,0x15,0x15,0x13,
	0x13,0x12,0x13,0x12,0x12,0x11,0x11,0x12,0x11,0x11,0x11,0x11,0x10,0x10,0x10,0x11,
	0x10,0x11,0x10,0x11,0x10,0x10,0x10,0x10,0x0f,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
	0x10,0x10,0x10,0x10,0x0f,0x10,0x10,0x0f,0x10,0x10,0x10,0x10,0x10,0x0b,0x17,0x17,
	0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x0f,0x17,0x15,0x13,0x13,0x13,0x12,0x12,0x12,0x12,
	0x11,0x11,0x12,0x11,0x11,0x11,0x10,0x11,0x10,0x11,0x11,0x10,0x10,0x10,0x10,0x10,
	0x10,0x10,0x10,0x10,0x10,0x0f,0x10,0x0f,0x10,0x10,0x10,0x10,0x10,0x0f,0x0f,0x0f,
	0x10,0x0f,0x10,0x10,0x0f,0x0f,0x0f,0x10,0x0b,0x17,0x17,0x1b,0x1b,0x3f,0x3f,0x3f,
	0x1b,0x1b,0x0f,0x17,0x15,0x15,0x12,0x12,0x12,0x11,0x11,0x11,0x11,0x10,0x10,0x11,
	0x10,0x11,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
	0x10,0x0f,0x0f,0x0f,0x0f,0x0f,0x10,0x0f,0x0f,0x0f,0x0f,0x10,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x10,0x0f,0x0f,0x0b,0x17,0x17,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x0b,
	0x0b,0x12,0x11,0x12,0x11,0x11,0x10,0x11,0x10,0x10,0x10,0x10,0x10,0x0f,0x10,0x10,
	0x0f,0x0f,0x10,0x10,0x0f,0x0f,0x10,0x10,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x10,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x10,0x0f,0x0f,0x0f,0x0f,0x0b,0x0b,
	0x17,0x17,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,0x1b,0x0b,0x10,0x10,
	0x11,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x0f,0x0f,0x10,0x0f,0x0f,0x0f,
	0x0f,0x0f,0x10,0x0f,0x0f,0x0f,0x10,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0b,0x0b,0x17,0x17,0x17,0x1b,0x1b,0x1b,
	0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
	0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
	0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
	0x0b,0x0b,0x0b,0x0b,0x17,0x17,0x17,0x17,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x3f,0x3f
};


const int32 kMouseDownWidth = 55;
const int32 kMouseDownHeight = 29;
const color_space kMouseDownColorSpace = B_CMAP8;

const unsigned char kMouseDownBits [] = {
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,
	0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,
	0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,0x09,0x08,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,0x1b,0x1b,0x1b,0x1b,0x1b,0x08,0x09,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x06,0x06,0x1b,0x1b,0x1b,0x1b,0x1b,0x3f,
	0x1b,0x1b,0x1b,0x08,0x09,0x11,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x14,0x13,
	0x13,0x13,0x13,0x13,0x13,0x13,0x14,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x14,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x14,0x13,0x13,0x13,0x13,0x11,0x13,
	0x11,0x11,0x06,0x06,0x18,0x1b,0x1b,0x3f,0x1b,0x1b,0x08,0x11,0x11,0x13,0x13,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x11,0x11,0x11,0x07,0x17,0x17,0x3f,
	0x1b,0x1b,0x08,0x11,0x13,0x13,0x15,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x0f,0x07,0x17,0x3f,0x1b,0x08,0x11,0x13,0x13,0x15,0x13,0x13,
	0x13,0x13,0x14,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x13,0x13,0x13,0x13,0x11,0x13,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x07,0x17,0x3f,
	0x1b,0x08,0x11,0x13,0x15,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
	0x13,0x14,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x13,
	0x11,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x0f,0x0f,0x0d,0x0d,0x06,0x3f,0x08,0x11,0x13,0x13,0x15,0x13,0x13,0x13,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
	0x13,0x11,0x13,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0d,0x0d,0x0d,0x06,0x3f,
	0x08,0x11,0x13,0x13,0x15,0x13,0x13,0x11,0x13,0x13,0x13,0x11,0x13,0x13,0x13,0x13,
	0x13,0x11,0x13,0x13,0x13,0x13,0x13,0x11,0x13,0x11,0x13,0x13,0x11,0x11,0x13,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x0f,0x0f,0x0f,0x0d,0x0c,0x06,0x3f,0x08,0x11,0x13,0x13,0x15,0x13,0x13,0x13,
	0x13,0x13,0x11,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x13,0x13,0x13,0x11,0x11,0x13,
	0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0d,0x0d,0x0d,0x0c,0x07,0x3f,
	0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,0x11,0x11,0x13,0x13,0x13,0x13,0x11,0x11,
	0x11,0x11,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,
	0x0f,0x0f,0x0f,0x0d,0x0c,0x0b,0x06,0x3f,0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,
	0x11,0x11,0x11,0x11,0x13,0x11,0x13,0x11,0x11,0x11,0x11,0x13,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,0x0c,0x06,0x3f,
	0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0f,
	0x0f,0x0d,0x0d,0x0d,0x0c,0x0b,0x06,0x3f,0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0c,0x0d,0x0b,0x06,0x3f,
	0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x0d,0x0d,0x0c,0x0b,0x0b,0x06,0x3f,0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,0x0c,0x0c,0x0b,0x06,0x3f,
	0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,
	0x0d,0x0d,0x0d,0x0c,0x0b,0x0a,0x07,0x3f,0x08,0x11,0x13,0x13,0x15,0x13,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,0x0b,0x0c,0x0a,0x07,0x3f,
	0x08,0x11,0x13,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,
	0x0d,0x0d,0x0d,0x0c,0x0b,0x0b,0x06,0x3f,0x08,0x11,0x13,0x13,0x13,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,0x0c,0x0c,0x0c,0x0b,0x0a,0x07,0x3f,
	0x08,0x11,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x0f,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,
	0x0d,0x0c,0x0c,0x0c,0x0a,0x0b,0x06,0x3f,0x08,0x11,0x13,0x13,0x13,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,0x0d,0x0c,0x0c,0x0b,0x0b,0x0a,0x07,0x3f,
	0x08,0x11,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,0x0d,
	0x0d,0x0c,0x0c,0x0c,0x0a,0x0b,0x06,0x3f,0x08,0x11,0x13,0x13,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0d,0x0d,0x0d,0x0d,0x0c,0x0b,0x0b,0x0a,0x0b,0x06,0x3f,
	0x08,0x11,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0e,0x0d,0x0d,0x0d,0x0d,
	0x0d,0x0c,0x0c,0x0b,0x0b,0x0b,0x06,0x3f,0x08,0x11,0x13,0x13,0x11,0x0f,0x0f,0x0f,
	0x0f,0x0e,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0e,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x0e,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0e,0x0f,0x0d,0x0d,0x0d,0x0d,0x0d,
	0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0b,0x0b,0x0b,0x0a,0x07,0x3f,
	0x08,0x11,0x13,0x0f,0x0f,0x06,0x07,0x06,0x06,0x07,0x06,0x06,0x07,0x06,0x07,0x06,
	0x06,0x07,0x06,0x06,0x07,0x06,0x07,0x06,0x06,0x07,0x06,0x06,0x07,0x06,0x07,0x06,
	0x06,0x07,0x06,0x06,0x07,0x06,0x07,0x06,0x06,0x07,0x06,0x06,0x07,0x06,0x07,0x06,
	0x06,0x07,0x0a,0x0b,0x0b,0x0a,0x07,0x3f,0x08,0x11,0x0f,0x06,0x07,0x1e,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x06,0x06,0x0b,0x0b,0x06,0x3f,
	0x08,0x06,0x07,0x1e,0x1e,0x1e,0x1c,0x1b,0x1b,0x1a,0x1a,0x1a,0x1a,0x19,0x19,0x19,
	0x17,0x19,0x19,0x19,0x19,0x19,0x19,0x17,0x19,0x19,0x19,0x19,0x19,0x19,0x18,0x17,
	0x17,0x18,0x17,0x18,0x17,0x17,0x17,0x15,0x17,0x17,0x17,0x15,0x15,0x15,0x15,0x13,
	0x14,0x17,0x17,0x18,0x06,0x06,0x07,0x3f
};


