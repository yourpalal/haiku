/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Maxim Shemanarev <mcseemagg@yahoo.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 */

//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//			mcseemagg@yahoo.com
//			http://www.antigrain.com
//----------------------------------------------------------------------------

#ifndef FONT_CACHE_ENTRY_H
#define FONT_CACHE_ENTRY_H


#include <Locker.h>

#include <agg_conv_curve.h>
#include <agg_conv_contour.h>
#include <agg_conv_transform.h>

#include "ServerFont.h"
#include "FontEngine.h"
#include "MultiLocker.h"
#include "Referenceable.h"
#include "Transformable.h"


struct GlyphCache {
	unsigned		glyph_index;
	uint8*			data;
	unsigned		data_size;
	glyph_data_type	data_type;
	agg::rect_i		bounds;
	float			advance_x;
	float			advance_y;
	float			inset_left;
	float			inset_right;
};

class FontCache;

class FontCacheEntry : public MultiLocker, public Referenceable {
 public:
	typedef FontEngine::PathAdapter					GlyphPathAdapter;
	typedef FontEngine::Gray8Adapter				GlyphGray8Adapter;
	typedef GlyphGray8Adapter::embedded_scanline	GlyphGray8Scanline;
	typedef FontEngine::MonoAdapter					GlyphMonoAdapter;
	typedef GlyphMonoAdapter::embedded_scanline		GlyphMonoScanline;
	typedef FontEngine::SubpixAdapter				SubpixAdapter;
	typedef agg::conv_curve<GlyphPathAdapter>		CurveConverter;
	typedef agg::conv_contour<CurveConverter>		ContourConverter;

	typedef agg::conv_transform<CurveConverter, Transformable>
													TransformedOutline;

	typedef agg::conv_transform<ContourConverter, Transformable>
													TransformedContourOutline;


								FontCacheEntry();
	virtual						~FontCacheEntry();

			bool				Init(const ServerFont& font);

			bool				HasGlyphs(const char* utf8String,
									ssize_t glyphCount) const;

			const GlyphCache*	Glyph(uint16 glyphCode);

			void				InitAdaptors(const GlyphCache* glyph,
									double x, double y,
									GlyphMonoAdapter& monoAdapter,
									GlyphGray8Adapter& gray8Adapter,
									GlyphPathAdapter& pathAdapter,
									double scale = 1.0);

			bool				GetKerning(uint16 glyphCode1,
									uint16 glyphCode2, double* x, double* y);

	static	void				GenerateSignature(char* signature,
									const ServerFont& font);
									
	static void					SetDefaultRenderType(glyph_rendering renderType);
	static glyph_rendering		DefaultRenderType()
									{ return sDefaultRenderType; }
	static void					SetDefaultHinting(bool hinting);
	static bool					DefaultHinting()
									{ return sDefaultHinting; }

	// private to FontCache class:
			void				UpdateUsage();
			bigtime_t			LastUsed() const
									{ return fLastUsedTime; }
			uint64				UsedCount() const
									{ return fUseCounter; }

 private:
								FontCacheEntry(const FontCacheEntry&);
			const FontCacheEntry& operator=(const FontCacheEntry&);

	static	glyph_rendering		_RenderTypeFor(const ServerFont& font);

			class GlyphCachePool;

			GlyphCachePool*		fGlyphCache;
			FontEngine			fEngine;
	static	glyph_rendering		sDefaultRenderType;
	static	bool				sDefaultHinting;

	static	BLocker				sUsageUpdateLock;
			bigtime_t			fLastUsedTime;
			uint64				fUseCounter;
};

#endif // FONT_CACHE_ENTRY_H

