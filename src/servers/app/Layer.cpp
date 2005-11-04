//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Layer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@cotty.iren.ro>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <AppDefs.h>
#include <Message.h>
#include <Region.h>
#include <View.h>

#include "DebugInfoManager.h"
#include "DisplayDriver.h"
#include "DrawState.h"
#include "PortLink.h"
#include "RootLayer.h"
#include "ServerProtocol.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include "Layer.h"
#include "ServerBitmap.h"

//#define DEBUG_LAYER
#ifdef DEBUG_LAYER
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

//#define DEBUG_LAYER_REBUILD
#ifdef DEBUG_LAYER_REBUILD
#	define RBTRACE(x) printf x
#else
#	define RBTRACE(x) ;
#endif


Layer::Layer(BRect frame, const char* name, int32 token,
	uint32 resize, uint32 flags, DisplayDriver* driver)
	:
	fFrame(frame), // in parent coordinates
//	fBoundsLeftTop(0.0, 0.0),

	// Layer does not start out as a part of the tree
	fOwner(NULL),

	fParent(NULL),
	fPreviousSibling(NULL),
	fNextSibling(NULL),
	fFirstChild(NULL),
	fLastChild(NULL),

	fCurrent(NULL),

	// all regions (fVisible, fFullVisible, fFull) start empty
#ifndef NEW_CLIPPING
	fVisible(),
	fFullVisible(),
	fFull(),
	fFrameAction(B_LAYER_ACTION_NONE),
	fClipReg(&fVisible),
#else
	fVisible2(),
	fFullVisible2(),
	fClipReg(&fVisible2),
#endif

	fServerWin(NULL),
	fName(name),
	fViewToken(token),

	fFlags(flags),
	fResizeMode(resize),
	fEventMask(0UL),
	fEventOptions(0UL),
	fHidden(false),
	fIsTopLayer(false),

	fAdFlags(0),

	fDriver(driver),
	fDrawState(new DrawState),

	fRootLayer(NULL),

	fViewColor(255, 255, 255, 255),
	fBackgroundBitmap(NULL),
	fOverlayBitmap(NULL)
{
	if (!frame.IsValid()) {
char helper[1024];
sprintf(helper, "Layer::Layer(BRect(%.1f, %.1f, %.1f, %.1f), name: %s, token: %ld) - frame is invalid\n",
		frame.left, frame.top, frame.right, frame.bottom, name, token);
CRITICAL(helper);
		fFrame.Set(0, 0, 1, 1);
	}

	if (!fDriver)
		CRITICAL("You MUST have a valid driver to init a Layer object\n");

	// NOTE: This flag is forwarded to a DrawState setting, even
	// though it is actually not part of a "state". However,
	// it is an important detail of a graphics context, and we
	// have no other means to pass this setting on to the DisplayDriver
	// other than through the DrawState. If we ever add a flag
	// B_ANTI_ALIASING to the view flags, it would have to be passed
	// in the same way. Though when breaking binary compatibility,
	// we might want to make this an actual part of a "state" (with
	// a different API to set these).
	// Note that the flag is also tested (updated) in Push/PopState and
	// SetFlags().
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);

	STRACE(("Layer(%s) successfuly created\n", Name()));
}


Layer::~Layer()
{
	delete fDrawState;

	// TODO: uncomment!
	//PruneTree();
}

/*!
	\brief Adds a child layer to the current one
	\param layer a new child layer
	\param serverWin the serverwindow to which the layer will belong
	
	Unlike the BView version, if the layer already belongs to another, then
	it spits an error to stdout and returns.
*/
void
Layer::AddChild(Layer* layer, ServerWindow* serverWin)
{
	STRACE(("Layer(%s)::AddChild(%s) START\n", Name(), layer->Name()));
	
	if (layer->fParent != NULL) {
		printf("ERROR: AddChild(): Layer already has a parent\n");
		return;
	}
	
	// 1) attach layer to the tree structure
	layer->fParent = this;
	
	// if we have children already, bump the current last child back one and
	// make the new child the last layer
	if (fLastChild) {
		layer->fPreviousSibling = fLastChild;
		fLastChild->fNextSibling = layer;
	} else {
		fFirstChild = layer;
	}
	fLastChild = layer;

	// if we have no RootLayer yet, then there is no need to set any parameters --
	// they will be set when the RootLayer for this tree will be added
	// to the main tree structure.
	if (!fRootLayer) {
		STRACE(("Layer(%s)::AddChild(%s) END\n", Name(), layer->Name()));
		return;
	}

	// 2) Iterate over the newly-added layer and all its children, setting the 
	//	root layer and server window and also rebuilding the full-size region
	//	for every descendant of the newly-added layer
	
	//c = short for: current
	Layer* c = layer;
	Layer* stop = layer;
	while (true) {
		// action block

		// 2.1) set the RootLayer for this object.
		c->SetRootLayer(c->fParent->fRootLayer);
		
		// 2.2) this Layer must know if it has a ServerWindow object attached.
		c->fServerWin=serverWin;
		
		// 2.3) we are attached to the main tree so build our full region.
#ifndef NEW_CLIPPING
		c->RebuildFullRegion();
#endif
		// tree parsing algorithm
		if (c->fFirstChild) {
			// go deep
			c = c->fFirstChild;
		} else {
			// go right or up
			
			if (c == stop) // our trip is over
				break;
				
			if (c->fNextSibling) {
				// go right
				c = c->fNextSibling;
			} else {
				// go up
				while (!c->fParent->fNextSibling && c->fParent != stop)
					c = c->fParent;
				
				if (c->fParent == stop) // that's enough!
					break;
				
				c = c->fParent->fNextSibling;
			}
		}
	}

	STRACE(("Layer(%s)::AddChild(%s) END\n", Name(), layer->Name()));
}

/*!
	\brief Removes a child layer from the current one
	\param layer the layer to remove
	
	If the layer does not belong to the the current layer, then this function 
	spits out an error to stdout and returns
*/
void
Layer::RemoveChild(Layer *layer)
{
	STRACE(("Layer(%s)::RemoveChild(%s) START\n", Name(), layer->Name()));
	
	if (!layer->fParent) {
		printf("ERROR: RemoveChild(): Layer doesn't have a parent\n");
		return;
	}
	
	if (layer->fParent != this) {
		printf("ERROR: RemoveChild(): Layer is not a child of this layer\n");
		return;
	}

	// 1) remove this layer from the main tree.
	
	// Take care of fParent
	layer->fParent = NULL;
	
	if (fFirstChild == layer)
		fFirstChild = layer->fNextSibling;
	
	if (fLastChild == layer)
		fLastChild = layer->fPreviousSibling;
	
	// Take care of siblings
	if (layer->fPreviousSibling != NULL)
		layer->fPreviousSibling->fNextSibling	= layer->fNextSibling;
	
	if (layer->fNextSibling != NULL)
		layer->fNextSibling->fPreviousSibling = layer->fPreviousSibling;
	
	layer->fPreviousSibling = NULL;
	layer->fNextSibling = NULL;

#ifdef NEW_CLIPPING
	layer->clear_visible_regions();
#endif

	// 2) Iterate over all of the removed-layer's descendants and unset the
	//	root layer, server window, and all redraw-related regions
	
	Layer* c = layer; //c = short for: current
	Layer* stop = layer;
	
	while (true) {
		// action block
		{
			// 2.1) set the RootLayer for this object.
			c->SetRootLayer(NULL);
			// 2.2) this Layer must know if it has a ServerWindow object attached.
			c->fServerWin = NULL;
			// 2.3) we were removed from the main tree so clear our full region.
#ifndef NEW_CLIPPING
			c->fFull.MakeEmpty();
			// 2.4) clear fullVisible region.
			c->fFullVisible.MakeEmpty();
			// 2.5) we don't have a visible region anymore.
			c->fVisible.MakeEmpty();
#endif
		}

		// tree parsing algorithm
		if (c->fFirstChild) {	
			// go deep
			c = c->fFirstChild;
		} else {	
			// go right or up
			if (c == stop) // out trip is over
				break;

			if (c->fNextSibling) {
				// go right
				c = c->fNextSibling;
			} else {
				// go up
				while(!c->fParent->fNextSibling && c->fParent != stop)
					c = c->fParent;
				
				if (c->fParent == stop) // that enough!
					break;
				
				c = c->fParent->fNextSibling;
			}
		}
	}
	STRACE(("Layer(%s)::RemoveChild(%s) END\n", Name(), layer->Name()));
}

//! Removes the calling layer from the tree
void
Layer::RemoveSelf()
{
	// A Layer removes itself from the tree (duh)
	if (fParent == NULL) {
		printf("ERROR: RemoveSelf(): Layer doesn't have a parent\n");
		return;
	}
	fParent->RemoveChild(this);
}

/*!
	\return true if the child is owned by this Layer, false if not
*/
bool
Layer::HasChild(Layer* layer)
{
	for (Layer* child = FirstChild(); child; child = NextChild()) {
		if (child == layer)
			return true;
	}
	return false;
}

//! Returns the number of children
uint32
Layer::CountChildren() const
{
	uint32 count = 0;
	Layer* child = FirstChild();
	while (child != NULL) {
		child = NextChild();
		count++;
	}
	return count;
}

/*!
	\brief Finds a child of the caller based on its token ID
	\param token ID of the layer to find
	\return Pointer to the layer or NULL if not found
*/
Layer*
Layer::FindLayer(const int32 token)
{
	// (recursive) search for a layer based on its view token
	
	// iterate only over direct child layers first
	for (Layer* child = FirstChild(); child; child = NextChild()) {
		if (child->fViewToken == token)
			return child;
	}
	
	// try a recursive search
	for (Layer* child = FirstChild(); child; child = NextChild()) {
		if (Layer* layer = child->FindLayer(token))
			return layer;
	}
	
	return NULL;
}

/*!
	\brief Returns the layer at the given point
	\param pt The point to look the layer at
	\return The layer containing the point or NULL if no layer found
*/
Layer*
Layer::LayerAt(const BPoint &pt, bool recursive)
{
	//printf("%p:%s:LayerAt(x = %g, y = %g)\n", this, Name(), pt.x, pt.y);
	if (!recursive)	{
		if (VisibleRegion().Contains(pt))
			return this;

		for (Layer* child = LastChild(); child; child = PreviousChild())
			if (child->FullVisible().Contains(pt))
				return child;

		return NULL;
	}

#ifndef NEW_CLIPPING
	if (fVisible.Contains(pt))
		return this;

	if (fFullVisible.Contains(pt)) {
		for (Layer* child = LastChild(); child; child = PreviousChild()) {
			if (Layer* layer = child->LayerAt(pt))
				return layer;
		}
	}
#else
	if (fVisible2.Contains(pt))
		return this;

	if (fFullVisible2.Contains(pt)) {
		for (Layer* child = LastChild(); child; child = PreviousChild()) {
			if (Layer* layer = child->LayerAt(pt))
				return layer;
		}
	}
#endif	
	return NULL;
}

// FirstChild
Layer*
Layer::FirstChild() const
{
	fCurrent = fFirstChild;
	return fCurrent;
}

// NextChild
Layer*
Layer::NextChild() const
{
	fCurrent = fCurrent->fNextSibling;
	return fCurrent;
}

// PreviousChild
Layer*
Layer::PreviousChild() const
{
	fCurrent = fCurrent->fPreviousSibling;
	return fCurrent;
}

// LastChild
Layer*
Layer::LastChild() const
{
	fCurrent = fLastChild;
	return fCurrent;
}

// SetName
void
Layer::SetName(const char* name)
{
	fName.SetTo(name);
}

// SetFlags
void
Layer::SetFlags(uint32 flags)
{
	fFlags = flags;
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}

#ifndef NEW_CLIPPING

//! Rebuilds the layer's "completely visible" region
void
Layer::RebuildFullRegion(void)
{
	STRACE(("Layer(%s)::RebuildFullRegion()\n", Name()));
	
	if (fParent)
		fFull.Set(fParent->ConvertToTop(fFrame ));
	else
		fFull.Set(fFrame);

	// TODO: restrict to screen coordinates
	
	// TODO: Convert to screen coordinates

	DrawState *ld;
	ld = fDrawState;
	do {
		// clip to user region
		if (const BRegion* userClipping = ld->ClippingRegion())
			fFull.IntersectWith(userClipping);
		
	} while ((ld = ld->prevState));
}

// StartRebuildRegions
void
Layer::StartRebuildRegions( const BRegion& reg, Layer *target, uint32 action, BPoint& pt)
{
	STRACE(("Layer(%s)::StartRebuildRegions() START\n", Name()));
	RBTRACE(("\n\nLayer(%s)::StartRebuildRegions() START\n", Name()));
	if (!fParent)
		fFullVisible = fFull;
	
	BRegion oldVisible = fVisible;
	
	fVisible = fFullVisible;

	// Rebuild regions for children...
	for (Layer *lay = LastChild(); lay; lay = PreviousChild()) {
		if (lay == target)
			lay->RebuildRegions(reg, action, pt, BPoint(0.0f, 0.0f));
		else
			lay->RebuildRegions(reg, B_LAYER_NONE, pt, BPoint(0.0f, 0.0f));
	}
	
	#ifdef DEBUG_LAYER_REBUILD
		printf("\nSRR: Layer(%s) ALMOST done regions:\n", Name());
		printf("\tVisible Region:\n");
		fVisible.PrintToStream();
		printf("\tFull Visible Region:\n");
		fFullVisible.PrintToStream();
	#endif
	
	BRegion redrawReg(fVisible);
	
	// if this is the first time
	if (oldVisible.CountRects() > 0)
		redrawReg.Exclude(&oldVisible);

	if (redrawReg.CountRects() > 0)
		fRootLayer->fRedrawReg.Include(&redrawReg);
	
	#ifdef DEBUG_LAYER_REBUILD
		printf("\nLayer(%s)::StartRebuildRegions() DONE. Results:\n", Name());
		printf("\tRedraw Region:\n");
		fRootLayer->fRedrawReg.PrintToStream();
		printf("\tCopy Region:\n");
		for (int32 k=0; k<fRootLayer->fCopyRegList.CountItems(); k++) {
			((BRegion*)(fRootLayer->fCopyRegList.ItemAt(k)))->PrintToStream();
			((BPoint*)(fRootLayer->fCopyList.ItemAt(k)))->PrintToStream();
		}
		printf("\n");
	#endif

	STRACE(("Layer(%s)::StartRebuildRegions() END\n", Name()));
	RBTRACE(("Layer(%s)::StartRebuildRegions() END\n", Name()));
}

// RebuildRegions
void
Layer::RebuildRegions( const BRegion& reg, uint32 action, BPoint pt, BPoint ptOffset)
{
	STRACE(("Layer(%s)::RebuildRegions() START\n", Name()));

	// TODO:/NOTE: this method must be executed as quickly as possible.
	
	// Currently SendView[Moved/Resized]Msg() simply constructs a message and calls
	// ServerWindow::SendMessageToClient(). This involves the alternative use of 
	// kernel and this code in the CPU, so there are a lot of context switches. 
	// This is NOT good at all!
	
	// One alternative would be the use of a BMessageQueue per ServerWindows OR only
	// one for app_server which will be emptied as soon as this critical operation ended.
	// Talk to DW, Gabe.
	
	BRegion	oldRegion;
	uint32 newAction = action;
	BPoint newPt = pt;
	BPoint newOffset = ptOffset; // used for resizing only
	
	BPoint dummyNewLocation;

	RRLabel1:
	switch(action) {
		case B_LAYER_NONE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_NONE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_NONE\n", Name()));
			oldRegion = fVisible;
			break;
		}
		case B_LAYER_MOVE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_MOVE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_MOVE\n", Name()));
			oldRegion = fFullVisible;
			fFrame.OffsetBy(pt.x, pt.y);
			fFull.OffsetBy(pt.x, pt.y);
			
			// TODO: investigate combining frame event messages for efficiency
			//SendViewMovedMsg();
			AddToViewsWithInvalidCoords();

			newAction	= B_LAYER_SIMPLE_MOVE;
			break;
		}
		case B_LAYER_SIMPLE_MOVE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_SIMPLE_MOVE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_SIMPLE_MOVE\n", Name()));
			fFull.OffsetBy(pt.x, pt.y);
			
			break;
		}
		case B_LAYER_RESIZE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_RESIZE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_RESIZE\n", Name()));
			oldRegion	= fVisible;
			
			fFrame.right	+= pt.x;
			fFrame.bottom	+= pt.y;
			RebuildFullRegion();
			
			// TODO: investigate combining frame event messages for efficiency
			//SendViewResizedMsg();
			AddToViewsWithInvalidCoords();
			
			newAction = B_LAYER_MASK_RESIZE;
			break;
		}
		case B_LAYER_MASK_RESIZE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_MASK_RESIZE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_MASK_RESIZE\n", Name()));
			oldRegion = fVisible;
			
			BPoint offset, rSize;
			BPoint coords[2];
			
			ResizeOthers(pt.x, pt.y, coords, NULL);
			offset = coords[0];
			rSize = coords[1];
			newOffset = offset + ptOffset;
			
			if (!(rSize.x == 0.0f && rSize.y == 0.0f)) {
				fFrame.OffsetBy(offset);
				fFrame.right += rSize.x;
				fFrame.bottom += rSize.y;
				RebuildFullRegion();
				
				// TODO: investigate combining frame event messages for efficiency
				//SendViewResizedMsg();
				AddToViewsWithInvalidCoords();
				
				newAction = B_LAYER_MASK_RESIZE;
				newPt = rSize;
				dummyNewLocation = newOffset;
			} else {
				if (!(offset.x == 0.0f && offset.y == 0.0f)) {
					pt = newOffset;
					action = B_LAYER_MOVE;
					newPt = pt;
					goto RRLabel1;
				} else {
					pt = ptOffset;
					action = B_LAYER_MOVE;
					newPt = pt;
					goto RRLabel1;
				}
			}
			break;
		}
	}

	if (!IsHidden()) {
		#ifdef DEBUG_LAYER_REBUILD
			printf("Layer(%s) real action START\n", Name());
			fFull.PrintToStream();
		#endif
		fFullVisible.MakeEmpty();
		fVisible = fFull;
		
		if (fParent && fVisible.CountRects() > 0) {
			// not the usual case, but support fot this is needed.
			if (fParent->fAdFlags & B_LAYER_CHILDREN_DEPENDANT) {
				#ifdef DEBUG_LAYER_REBUILD
					printf("   B_LAYER_CHILDREN_DEPENDANT Parent\n");
				#endif
				
				// because we're skipping one level, we need to do out
				// parent business as well.
				
				// our visible area is relative to our parent's parent.
				if (fParent->fParent)
					fVisible.IntersectWith(&(fParent->fParent->fVisible));
					
				// exclude parent's visible area which could be composed by
				// prior siblings' visible areas.
				if (fVisible.CountRects() > 0)
					fVisible.Exclude(&(fParent->fVisible));
				
				// we have a final visible area. Include it to our parent's one,
				// exclude from parent's parent.
				if (fVisible.CountRects() > 0) {
					fParent->fFullVisible.Include(&fVisible);
						
					if (fParent->fParent)
						fParent->fParent->fVisible.Exclude(&fVisible);
				}
			} else {
				// for 95+% of cases
				
				#ifdef DEBUG_LAYER_REBUILD
					printf("   (!)B_LAYER_CHILDREN_DEPENDANT Parent\n");
				#endif
				
				// the visible area is the one common with parent's one.
				fVisible.IntersectWith(&(fParent->fVisible));
				
				// exclude from parent's visible area. we're the owners now.
				if (fVisible.CountRects() > 0)
					fParent->fVisible.Exclude(&fVisible);
			}
		}
		fFullVisible = fVisible;
	}
	
	// Rebuild regions for children...
	for(Layer *lay = LastChild(); lay != NULL; lay = PreviousChild())
		lay->RebuildRegions(reg, newAction, newPt, newOffset);

	#ifdef DEBUG_LAYER_REBUILD
		printf("\nLayer(%s) ALMOST done regions:\n", Name());
		printf("\tVisible Region:\n");
		fVisible.PrintToStream();
		printf("\tFull Visible Region:\n");
		fFullVisible.PrintToStream();
	#endif

	if(!IsHidden()) {
		switch(action) {
			case B_LAYER_NONE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_NONE\n", Name()));
				BRegion r(fVisible);
				if (oldRegion.CountRects() > 0)
					r.Exclude(&oldRegion);
				
				if(r.CountRects() > 0)
					fRootLayer->fRedrawReg.Include(&r);
				break;
			}
			case B_LAYER_MOVE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_MOVE\n", Name()));
				BRegion redrawReg;
				BRegion	*copyReg = new BRegion();
				BRegion	screenReg(fRootLayer->Bounds());
				
				oldRegion.OffsetBy(pt.x, pt.y);
				oldRegion.IntersectWith(&fFullVisible);
				
				*copyReg = oldRegion;
				copyReg->IntersectWith(&screenReg);
				if (copyReg->CountRects() > 0 && !(pt.x == 0.0f && pt.y == 0.0f)) {
					copyReg->OffsetBy(-pt.x, -pt.y);
					BPoint		*point = new BPoint(pt);
					fRootLayer->fCopyRegList.AddItem(copyReg);
					fRootLayer->fCopyList.AddItem(point);
				} else {
					delete copyReg;
				}
				
				redrawReg	= fFullVisible;
				redrawReg.Exclude(&oldRegion);
				if (redrawReg.CountRects() > 0 && !(pt.x == 0.0f && pt.y == 0.0f)) {
					fRootLayer->fRedrawReg.Include(&redrawReg);
				}
	
				break;
			}
			case B_LAYER_RESIZE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_RESIZE\n", Name()));
				BRegion redrawReg;
				
				redrawReg = fVisible;
				redrawReg.Exclude(&oldRegion);
				if(redrawReg.CountRects() > 0)
					fRootLayer->fRedrawReg.Include(&redrawReg);
	
				break;
			}
			case B_LAYER_MASK_RESIZE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_MASK_RESIZE\n", Name()));
				BRegion redrawReg;
				BRegion	*copyReg = new BRegion();
				
				oldRegion.OffsetBy(dummyNewLocation.x, dummyNewLocation.y);
				
				redrawReg	= fVisible;
				redrawReg.Exclude(&oldRegion);
				if (redrawReg.CountRects() > 0)
					fRootLayer->fRedrawReg.Include(&redrawReg);
				
				*copyReg = fVisible;
				copyReg->IntersectWith(&oldRegion);
				copyReg->OffsetBy(-dummyNewLocation.x, -dummyNewLocation.y);
				if (copyReg->CountRects() > 0
					&& !(dummyNewLocation.x == 0.0f && dummyNewLocation.y == 0.0f)) {
					fRootLayer->fCopyRegList.AddItem(copyReg);
					fRootLayer->fCopyList.AddItem(new BPoint(dummyNewLocation));
				}
				
				break;
			}
			default:
				RBTRACE(("2) Layer(%s): Action default\n", Name()));
				break;
		}
	}
/*	if (IsHidden()) {
		fFullVisible.MakeEmpty();
		fVisible.MakeEmpty();
	}
*/

	STRACE(("Layer(%s)::RebuildRegions() END\n", Name()));
}

// ResizeOthers
uint32
Layer::ResizeOthers(float x, float y, BPoint coords[], BPoint *ptOffset)
{
	STRACE(("Layer(%s)::ResizeOthers() START\n", Name()));
	uint32 rmask = fResizeMode;
	
	// offset
	coords[0].x	= 0.0f;
	coords[0].y	= 0.0f;
	
	// resize by width/height
	coords[1].x	= 0.0f;
	coords[1].y	= 0.0f;

	if ((rmask & 0x00000f00UL)>>8 == _VIEW_LEFT_ &&
		(rmask & 0x0000000fUL)>>0 == _VIEW_RIGHT_) {
		coords[1].x		= x;
	} else if ((rmask & 0x00000f00UL)>>8 == _VIEW_LEFT_) {
	} else if ((rmask & 0x0000000fUL)>>0 == _VIEW_RIGHT_) {
		coords[0].x		= x;
	} else if ((rmask & 0x00000f00UL)>>8 == _VIEW_CENTER_) {
		coords[0].x		= x/2;
	} else {
		// illegal flag. Do nothing.
	}


	if ((rmask & 0x0000f000UL)>>12 == _VIEW_TOP_ &&
		(rmask & 0x000000f0UL)>>4 == _VIEW_BOTTOM_) {
		coords[1].y		= y;
	} else if ((rmask & 0x0000f000UL)>>12 == _VIEW_TOP_) {
	} else if ((rmask & 0x000000f0UL)>>4 == _VIEW_BOTTOM_) {
		coords[0].y		= y;
	} else if ((rmask & 0x0000f000UL)>>12 == _VIEW_CENTER_) {
		coords[0].y		= y/2;
	} else {
		// illegal flag. Do nothing.
	}

	STRACE(("Layer(%s)::ResizeOthers() END\n", Name()));
	return 0UL;
}

#endif

// Redraw
void
Layer::Redraw(const BRegion& reg, Layer *startFrom)
{
	STRACE(("Layer(%s)::Redraw();\n", Name()));
	if (IsHidden())
		// this layer has nothing visible on screen, so bail out.
		return;

	BRegion *pReg = const_cast<BRegion*>(&reg);

	if (pReg->CountRects() > 0)
		RequestDraw(reg, startFrom);
	
	STRACE(("Layer(%s)::Redraw() ENDED\n", Name()));
}

// Draw
void
Layer::Draw(const BRect &rect)
{
#ifdef DEBUG_LAYER
	printf("Layer(%s)::Draw: ", Name());
	rect.PrintToStream();
#endif	

	if (!ViewColor().IsTransparentMagic())
		fDriver->FillRect(rect, ViewColor());
}

#ifndef NEW_CLIPPING
// EmptyGlobals
void
Layer::EmptyGlobals()
{
	fRootLayer->fRedrawReg.MakeEmpty();

	int32 count = fRootLayer->fCopyRegList.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BRegion*)fRootLayer->fCopyRegList.ItemAt(i);
	fRootLayer->fCopyRegList.MakeEmpty();
	
	count = fRootLayer->fCopyList.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BPoint*)fRootLayer->fCopyList.ItemAt(i);
	fRootLayer->fCopyList.MakeEmpty();
}
#endif

/*!
	\brief Shows the layer
	\param invalidate Invalidate the region when showing the layer. defaults to true
*/
void
Layer::Show(bool invalidate)
{
	STRACE(("Layer(%s)::Show()\n", Name()));
	if(!IsHidden()) {
		// an ancestor may be hidden. OK, we're not visible,
		// but we're changing our visibility state
		fHidden	= false;
		return;
	}
	
	fHidden	= false;

	SendViewCoordUpdateMsg();

// NOTE: I added this here and it solves the invalid region problem
// for Windows that have been resized before they were shown. -Stephan
#ifndef NEW_CLIPPING
RebuildFullRegion();

	if (invalidate)
		GetRootLayer()->GoInvalidate(this, fFull);
#else
	if (invalidate) {
		// compute the region this layer wants for itself
		BRegion	invalidRegion;
		_GetWantedRegion(invalidRegion);
		if (invalidRegion.CountRects() > 0)
			GetRootLayer()->GoInvalidate(this, invalidRegion);
	}
#endif
}

/*!
	\brief Shows the layer
	\param invalidate Invalidate the region when hiding the layer. defaults to true
*/
void
Layer::Hide(bool invalidate)
{
	STRACE(("Layer(%s)::Hide()\n", Name()));
	if (IsHidden()) {
		// an ancestor may be hidden. OK, we're not visible,
		// but we're changing our visibility state
		fHidden	= true;		
		return;
	}

	fHidden	= true;
#ifndef NEW_CLIPPING	
	if (invalidate)
		GetRootLayer()->GoInvalidate(this, fFullVisible);
#else
	if (invalidate && fFullVisible2.CountRects() > 0) {
		GetRootLayer()->GoInvalidate(this, fFullVisible2);
	}
#endif
}

//! Returns true if the layer is hidden
bool
Layer::IsHidden(void) const
{
	if (fHidden)
		return true;

	if (fParent)
			return fParent->IsHidden();
	
	return fHidden;
}


void
Layer::PushState()
{
	fDrawState = fDrawState->PushState();
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


void
Layer::PopState()
{
	if (fDrawState->PreviousState() == NULL) {
		fprintf(stderr, "WARNING: User called BView(%s)::PopState(), but there is NO state on stack!\n", Name());
		return;
	}

	fDrawState = fDrawState->PopState();
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


//! Matches the BView call of the same name
BRect
Layer::Bounds(void) const
{
	BRect r(fFrame);
//	r.OffsetTo(fBoundsLeftTop);
	r.OffsetTo(BoundsOrigin());
	return r;
}

//! Matches the BView call of the same name
BRect
Layer::Frame(void) const
{
	return fFrame;
}

//! Moves the layer by specified values, complete with redraw
void
Layer::MoveBy(float x, float y)
{
	STRACE(("Layer(%s)::MoveBy() START\n", Name()));
	if (!fParent) {
		CRITICAL("ERROR: in Layer::MoveBy()! - No parent!\n");
		return;
	}

	GetRootLayer()->Lock();
#ifndef NEW_CLIPPING
	move_layer(x, y);
#else
	do_MoveBy(x, y);
#endif
	GetRootLayer()->Unlock();

	STRACE(("Layer(%s)::MoveBy() END\n", Name()));
}

//! Resize the layer by the specified amount, complete with redraw
void
Layer::ResizeBy(float x, float y)
{
	STRACE(("Layer(%s)::ResizeBy() START\n", Name()));

	if (!fParent) {
		printf("ERROR: in Layer::ResizeBy()! - No parent!\n");
		return;
	}

	GetRootLayer()->Lock();
#ifndef NEW_CLIPPING
	resize_layer(x, y);
#else
	do_ResizeBy(x, y);
#endif
	GetRootLayer()->Unlock();

	STRACE(("Layer(%s)::ResizeBy() END\n", Name()));
}

//! scrolls the layer by the specified amount, complete with redraw
void
Layer::ScrollBy(float x, float y)
{
	STRACE(("Layer(%s)::ScrollBy() START\n", Name()));

	GetRootLayer()->Lock();
#ifndef NEW_CLIPPING
	// nothing
#else
	do_ScrollBy(x, y);
#endif
	GetRootLayer()->Unlock();

	STRACE(("Layer(%s)::ScrollBy() END\n", Name()));
}

void
Layer::MouseDown(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, fViewToken, false);
	}
}

void
Layer::MouseUp(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, fViewToken, false);
	}
}

void
Layer::MouseMoved(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, fViewToken, false);
	}
}

void
Layer::MouseWheelChanged(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, fViewToken, false);
	}
}

void
Layer::KeyDown(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, B_NULL_TOKEN, true);
	}
}

void
Layer::KeyUp(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, B_NULL_TOKEN, true);
	}
}

void
Layer::UnmappedKeyDown(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, B_NULL_TOKEN, true);
	}
}

void
Layer::UnmappedKeyUp(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, B_NULL_TOKEN, true);
	}
}

void
Layer::ModifiersChanged(const BMessage *msg)
{
	if (Window() && !IsTopLayer()) {
		Window()->SendMessageToClient(msg, B_NULL_TOKEN, true);
	}
}

void
Layer::WorkspaceActivated(int32 index, bool active)
{
	// Empty
}

void
Layer::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	// Empty
}

void
Layer::Activated(bool active)
{
	// Empty
}


BPoint
Layer::BoundsOrigin() const
{
	BPoint origin(0,0);
	float scale = Scale();

	DrawState* layerData = fDrawState;
	do {
		origin += layerData->Origin();
	} while ((layerData = layerData->PreviousState()) != NULL);

	origin.x *= scale;
	origin.y *= scale;

	return origin;
}


float
Layer::Scale() const
{
	return CurrentState()->Scale();
}


//! Converts the passed point to parent coordinates
BPoint
Layer::ConvertToParent(BPoint pt)
{
#ifndef NEW_CLIPPING
	pt -= BoundsOrigin();
	pt += fFrame.LeftTop();
	return pt;
#else
	ConvertToParent2(&pt);
	return pt;
#endif
}

//! Converts the passed rectangle to parent coordinates
BRect
Layer::ConvertToParent(BRect rect)
{
#ifndef NEW_CLIPPING
//	rect.OffsetBy(fFrame.LeftTop());
//	return rect;
	rect.OffsetBy(-BoundsOrigin().x, -BoundsOrigin().y);
	rect.OffsetBy(fFrame.LeftTop());
	return rect;
#else
	ConvertToParent2(&rect);
	return rect;
#endif
}

//! Converts the passed region to parent coordinates
BRegion
Layer::ConvertToParent(BRegion* reg)
{
#ifndef NEW_CLIPPING
	// TODO: wouldn't it be more efficient to use the copy
	// constructor for BRegion and then call OffsetBy()?
	BRegion newreg;
	for (int32 i = 0; i < reg->CountRects(); i++)
		newreg.Include(ConvertToParent(reg->RectAt(i)));
	return newreg;
#else
	BRegion newReg(*reg);
	ConvertToParent2(&newReg);
	return newReg;
#endif
}

//! Converts the passed point from parent coordinates
BPoint
Layer::ConvertFromParent(BPoint pt)
{
#ifndef NEW_CLIPPING
//	return pt - fFrame.LeftTop();
	pt -= fFrame.LeftTop();
	pt += BoundsOrigin();
	return pt;
#else
	ConvertFromParent2(&pt);
	return pt;
#endif
}

//! Converts the passed rectangle from parent coordinates
BRect
Layer::ConvertFromParent(BRect rect)
{
#ifndef NEW_CLIPPING
//	rect.OffsetBy(-fFrame.left, -fFrame.top);
//	return rect;
	rect.OffsetBy(-fFrame.left, -fFrame.top);
	rect.OffsetBy(BoundsOrigin());
	return rect;
#else
	ConvertFromParent2(&rect);
	return rect;
#endif
}

//! Converts the passed region from parent coordinates
BRegion
Layer::ConvertFromParent(BRegion *reg)
{
#ifndef NEW_CLIPPING
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertFromParent(reg->RectAt(i)));
	return newreg;
#else
	BRegion newReg(*reg);
	ConvertFromParent2(&newReg);
	return newReg;
#endif
}

// ConvertToTop
BPoint
Layer::ConvertToTop(BPoint pt)
{
#ifndef NEW_CLIPPING
	if (fParent) {
//		return (fParent->ConvertToTop(pt + fFrame.LeftTop()));
		pt = ConvertToParent(pt);
		return fParent->ConvertToTop(pt);
	} else
		return pt;
#else
	ConvertToScreen2(&pt);
	return pt;
#endif
}

//! Converts the passed rectangle to screen coordinates
BRect
Layer::ConvertToTop(BRect rect)
{
#ifndef NEW_CLIPPING
	if (fParent) {
//		return fParent->ConvertToTop(rect.OffsetByCopy(fFrame.LeftTop()));
		rect = ConvertToParent(rect);
		return fParent->ConvertToTop(rect);
	} else
		return rect;
#else
	ConvertToScreen2(&rect);
	return rect;
#endif
}

//! Converts the passed region to screen coordinates
BRegion
Layer::ConvertToTop(BRegion *reg)
{
#ifndef NEW_CLIPPING
	BRegion newreg;
	for (int32 i = 0; i < reg->CountRects();i++)
		newreg.Include(ConvertToTop(reg->RectAt(i)));
	return newreg;
#else
	BRegion newReg(*reg);
	ConvertToScreen2(&newReg);
	return newReg;
#endif
}

// ConvertFromTop
BPoint
Layer::ConvertFromTop(BPoint pt)
{
#ifndef NEW_CLIPPING
	if (fParent) {
//		return fParent->ConvertFromTop(pt-fFrame.LeftTop());
		pt = ConvertFromParent(pt);
		return fParent->ConvertFromTop(pt);
	} else
		return pt;
#else
	ConvertFromScreen2(&pt);
	return pt;
#endif
}

//! Converts the passed rectangle from screen coordinates
BRect
Layer::ConvertFromTop(BRect rect)
{
#ifndef NEW_CLIPPING
	if (fParent) {
//		return fParent->ConvertFromTop(rect.OffsetByCopy(-fFrame.LeftTop().x,
//														 -fFrame.LeftTop().y));
		rect = ConvertFromParent(rect);
		return fParent->ConvertFromTop(rect);
	} else
		return rect;
#else
	ConvertFromScreen2(&rect);
	return rect;
#endif
}

//! Converts the passed region from screen coordinates
BRegion
Layer::ConvertFromTop(BRegion *reg)
{
#ifndef NEW_CLIPPING
	BRegion newreg;
	
	for (int32 i = 0; i < reg->CountRects(); i++)
		newreg.Include(ConvertFromTop(reg->RectAt(i)));
	
	return newreg;
#else
	BRegion newReg(*reg);
	ConvertFromScreen2(&newReg);
	return newReg;
#endif
}

//! Recursively deletes all children of the calling layer
void
Layer::PruneTree(void)
{

	Layer* lay;
	Layer* nextlay;
	
	lay = fFirstChild;
	fFirstChild = NULL;
	
	while (lay != NULL) {
		if (lay->fFirstChild != NULL)
			lay->PruneTree();
		
		nextlay = lay->fNextSibling;
		lay->fNextSibling = NULL;
		
		delete lay;
		lay = nextlay;
	}
	// Man, this thing is short. Elegant, ain't it? :P
}

//! Prints information about the layer's current state
void
Layer::PrintToStream()
{
	printf("\n *** Layer %s:\n", Name());
	printf("\t Parent: %s", fParent ? fParent->Name() : "<no parent>");

	printf("\t us: %s\t ls: %s\n",
		fPreviousSibling ? fPreviousSibling->Name() : "<none>",
		fNextSibling ? fNextSibling->Name() : "<none>");

	printf("\t topChild: %s\t bottomChild: %s\n",
		fFirstChild ? fFirstChild->Name() : "<none>",
		fLastChild ? fLastChild->Name() : "<none>");
	
	printf("Frame: (%f, %f, %f, %f)\n", fFrame.left, fFrame.top, fFrame.right, fFrame.bottom);
	printf("LocalOrigin: (%f, %f)\n", BoundsOrigin().x, BoundsOrigin().y);
	printf("Token: %ld\n", fViewToken);
	printf("Hidden - direct: %s ", fHidden?"true":"false");
	printf("Hidden - indirect: %s\n", IsHidden()?"true":"false");
	printf("ResizingMode: %lx ", fResizeMode);
	printf("Flags: %lx\n", fFlags);

	if (fDrawState)
		fDrawState->PrintToStream();
	else
		printf(" NO DrawState valid pointer\n");
}

//! Prints pointer info kept by the current layer
void
Layer::PrintNode()
{
	printf("-----------\nLayer %s\n", Name());
	if (fParent)
		printf("Parent: %s (%p)\n", fParent->Name(), fParent);
	else
		printf("Parent: NULL\n");

	if (fPreviousSibling)
		printf("Upper sibling: %s (%p)\n", fPreviousSibling->Name(), fPreviousSibling);
	else
		printf("Upper sibling: NULL\n");

	if (fNextSibling)
		printf("Lower sibling: %s (%p)\n", fNextSibling->Name(), fNextSibling);
	else
		printf("Lower sibling: NULL\n");

	if (fFirstChild)
		printf("Top child: %s (%p)\n", fFirstChild->Name(), fFirstChild);
	else
		printf("Top child: NULL\n");

	if (fLastChild)
		printf("Bottom child: %s (%p)\n", fLastChild->Name(), fLastChild);
	else
		printf("Bottom child: NULL\n");
#ifndef NEW_CLIPPING
	printf("Visible Areas: "); fVisible.PrintToStream();
#endif
}

//! Prints the tree hierarchy from the current layer down
void
Layer::PrintTree()
{
	printf("\n Tree structure:\n");
	printf("\t%s\t%s\n", Name(), IsHidden()? "Hidden": "NOT hidden");
	for(Layer *lay = LastChild(); lay != NULL; lay = PreviousChild())
		printf("\t%s\t%s\n", lay->Name(), lay->IsHidden()? "Hidden": "NOT hidden");
}

// RequestDraw
void
Layer::RequestDraw(const BRegion &reg, Layer *startFrom)
{
	STRACE(("Layer(%s)::RequestDraw()\n", Name()));

	// do not redraw any child until you must
	int redraw = false;
	if (!startFrom)
		redraw = true;

#ifndef NEW_CLIPPING
	if (HasClient() && IsTopLayer()) {
		// calculate the minimum region/rectangle to be updated with
		// a single message to the client.
		BRegion	updateReg(fFullVisible);

		if (fFlags & B_FULL_UPDATE_ON_RESIZE
			&& fFrameAction	== B_LAYER_ACTION_RESIZE)
		{
			// do nothing
		} else {
			updateReg.IntersectWith(&reg);
		}
		if (updateReg.CountRects() > 0) {
			fOwner->fCumulativeRegion.Include(&updateReg);			
			if (!fOwner->InUpdate() && !fOwner->fRequestSent) {
				fOwner->fInUpdateRegion = fOwner->fCumulativeRegion;
fOwner->cnt++;
if (fOwner->cnt != 1)
	CRITICAL("Layer::RequestDraw(): fOwner->cnt != 1 -> Not Allowed!");
				fOwner->fRequestSent = true; // this is here to avoid a possible de-synchronization
				if (SendUpdateMsg(fOwner->fInUpdateRegion) == B_OK) {
					fOwner->fCumulativeRegion.MakeEmpty();
				}
				else {
					fOwner->fRequestSent = false;
					fOwner->fInUpdateRegion.MakeEmpty();
				}
			}
		}
	}

	if (fVisible.CountRects() > 0) {
		BRegion	updateReg(fVisible);
		// calculate the update region
		if (fFlags & B_FULL_UPDATE_ON_RESIZE && fFrameAction == B_LAYER_ACTION_RESIZE) {
			// do nothing
		} else {
			updateReg.IntersectWith(&reg);
		}

		if (updateReg.CountRects() > 0) {
			fDriver->ConstrainClippingRegion(&updateReg);
			Draw(updateReg.Frame());
			fDriver->ConstrainClippingRegion(NULL);
		}
	}

	for (Layer *lay = LastChild(); lay != NULL; lay = PreviousChild()) {
		if (lay == startFrom)
			redraw = true;

		if (redraw && !(lay->IsHidden())) {
			// no need to go deeper if not even the FullVisible region intersects
			// Update one.
			BRegion common(lay->fFullVisible);
			common.IntersectWith(&reg);
			
			if (common.CountRects() > 0)
				lay->RequestDraw(reg, NULL);
		}
	}
#else
	if (HasClient() && IsTopLayer()) {
		// calculate the minimum region/rectangle to be updated with
		// a single message to the client.
		BRegion	updateReg(fFullVisible2);

		updateReg.IntersectWith(&reg);

		if (updateReg.CountRects() > 0) {
			fOwner->fCumulativeRegion.Include(&updateReg);			
			if (fOwner->fUpdateRequestsEnabled && !fOwner->InUpdate() && !fOwner->fRequestSent) {
				fOwner->fInUpdateRegion = fOwner->fCumulativeRegion;
fOwner->cnt++;
if (fOwner->cnt != 1)
	CRITICAL("Layer::RequestDraw(): fOwner->cnt != 1 -> Not Allowed!");
				fOwner->fRequestSent = true; // this is here to avoid a possible de-synchronization
				if (SendUpdateMsg(fOwner->fInUpdateRegion) == B_OK) {
					fOwner->fCumulativeRegion.MakeEmpty();
				}
				else {
					fOwner->fRequestSent = false;
					fOwner->fInUpdateRegion.MakeEmpty();
				}
			}
		}
	}

	if (fVisible2.CountRects() > 0) {
		BRegion	updateReg(fVisible2);
			updateReg.IntersectWith(&reg);

		if (updateReg.CountRects() > 0) {
			fDriver->ConstrainClippingRegion(&updateReg);
			Draw(updateReg.Frame());
			fDriver->ConstrainClippingRegion(NULL);
		}
	}

	for (Layer *lay = LastChild(); lay != NULL; lay = PreviousChild()) {
		if (lay == startFrom)
			redraw = true;

		if (redraw && !(lay->IsHidden())) {
			// no need to go deeper if not even the FullVisible region intersects
			// Update one.
			BRegion common(lay->fFullVisible2);
			common.IntersectWith(&reg);
			
			if (common.CountRects() > 0)
				lay->RequestDraw(reg, NULL);
		}
	}
#endif
}

#ifndef NEW_CLIPPING

// move_layer
void
Layer::move_layer(float x, float y)
{
/*	if (fClassID == AS_WINBORDER_CLASS) {
		WinBorder	*wb = (WinBorder*)this;
		wb->fCumulativeRegion.OffsetBy(x, y);
		wb->fInUpdateRegion.OffsetBy(x, y);
		wb->fSavedForUpdateRegion.OffsetBy(x, y);
	}*/
	
	fFrameAction = B_LAYER_ACTION_MOVE;

	BPoint pt(x, y);	
	BRect rect(fFull.Frame().OffsetByCopy(pt));

if (!fParent) {
printf("no parent in Layer::move_layer() (%s)\n", Name());
fFrameAction = B_LAYER_ACTION_NONE;
return;
}

	fParent->StartRebuildRegions(BRegion(rect), this, B_LAYER_MOVE, pt);

	fDriver->CopyRegionList(&fRootLayer->fCopyRegList,
							&fRootLayer->fCopyList,
							fRootLayer->fCopyRegList.CountItems(),
							&fFullVisible);

	fParent->Redraw(fRootLayer->fRedrawReg, this);

	// redraw workspaces layer
	if (dynamic_cast<WinBorder*>(this) != NULL && fRootLayer->WorkspacesLayer() != NULL) {
		fRootLayer->GoRedraw(fRootLayer->WorkspacesLayer(), fRootLayer->WorkspacesLayer()->fVisible);
	}

	SendViewCoordUpdateMsg();
	
	EmptyGlobals();

	fFrameAction = B_LAYER_ACTION_NONE;
}

// resize_layer
void
Layer::resize_layer(float x, float y)
{
	fFrameAction = B_LAYER_ACTION_RESIZE;

	BPoint pt(x,y);	

	BRect rect(fFull.Frame());
	rect.right += x;
	rect.bottom += y;

if (!fParent) {
printf("no parent in Layer::resize_layer() (%s)\n", Name());
fFrameAction = B_LAYER_ACTION_NONE;
return;
}

	fParent->StartRebuildRegions(BRegion(rect), this, B_LAYER_RESIZE, pt);

	fDriver->CopyRegionList(&fRootLayer->fCopyRegList, &fRootLayer->fCopyList, fRootLayer->fCopyRegList.CountItems(), &fFullVisible);

	fParent->Redraw(fRootLayer->fRedrawReg, this);

	// redraw workspaces layer
	if (dynamic_cast<WinBorder*>(this) != NULL && fRootLayer->WorkspacesLayer() != NULL) {
		fRootLayer->GoRedraw(fRootLayer->WorkspacesLayer(), fRootLayer->WorkspacesLayer()->fVisible);
	}

	SendViewCoordUpdateMsg();
	
	EmptyGlobals();

	fFrameAction = B_LAYER_ACTION_NONE;
}

// FullInvalidate
void
Layer::FullInvalidate(const BRect &rect)
{
	FullInvalidate(BRegion(rect));
}

// FullInvalidate
void
Layer::FullInvalidate(const BRegion& region)
{
	STRACE(("Layer(%s)::FullInvalidate():\n", Name()));
	
#ifdef DEBUG_LAYER
	region.PrintToStream();
	printf("\n");
#endif

	BPoint pt(0,0);
	StartRebuildRegions(region, NULL,/* B_LAYER_INVALIDATE, pt); */B_LAYER_NONE, pt);

	Redraw(fRootLayer->fRedrawReg);
	
	EmptyGlobals();
}

// Invalidate
void
Layer::Invalidate(const BRegion& region)
{
	STRACE(("Layer(%s)::Invalidate():\n", Name()));
#ifdef DEBUG_LAYER
	region.PrintToStream();
	printf("\n");
#endif
	
	fRootLayer->fRedrawReg	= region;
	
	Redraw(fRootLayer->fRedrawReg);
	
	EmptyGlobals();
}

#endif // 5 methods


/*!
	\brief Returns the layer's ServerWindow
	
	If the layer's ServerWindow has not been assigned, it attempts to find 
	the owning ServerWindow in the tree.
*/
ServerWindow*
Layer::SearchForServerWindow()
{
	if (!fServerWin)
		fServerWin=fParent->SearchForServerWindow();
	
	return fServerWin;
}

//! Sends an _UPDATE_ message to the client BWindow
status_t
Layer::SendUpdateMsg(BRegion& reg)
{
	BMessage msg;
	msg.what = _UPDATE_;
#ifndef NEW_CLIPPING
	msg.AddRect("_rect", ConvertFromTop(reg.Frame()));
#else
	BRect	rect(reg.Frame());
	ConvertFromScreen2(&rect);
	msg.AddRect("_rect", rect );
#endif
	msg.AddRect("debug_rect", reg.Frame());
//	msg.AddInt32("_token",fViewToken);
		
	return Owner()->Window()->SendMessageToClient(&msg);
}

// AddToViewsWithInvalidCoords
void
Layer::AddToViewsWithInvalidCoords() const
{
	if (fServerWin) {
		fServerWin->ClientViewsWithInvalidCoords().AddInt32("_token", fViewToken);
		fServerWin->ClientViewsWithInvalidCoords().AddPoint("where", fFrame.LeftTop());
		fServerWin->ClientViewsWithInvalidCoords().AddFloat("width", fFrame.Width());
		fServerWin->ClientViewsWithInvalidCoords().AddFloat("height", fFrame.Height());
	}
}

// SendViewCoordUpdateMsg
void
Layer::SendViewCoordUpdateMsg() const
{
	if (fServerWin && !fServerWin->ClientViewsWithInvalidCoords().IsEmpty()) {
		fServerWin->SendMessageToClient(&fServerWin->ClientViewsWithInvalidCoords());
		fServerWin->ClientViewsWithInvalidCoords().MakeEmpty();
	}
}

// SetViewColor
void
Layer::SetViewColor(const RGBColor& color)
{
	fViewColor = color;
}

// SetBackgroundBitmap
void
Layer::SetBackgroundBitmap(const ServerBitmap* bitmap)
{
	// TODO: What about reference counting?
	// "Release" old fBackgroundBitmap and "Aquire" new one?
	fBackgroundBitmap = bitmap;
}

// SetOverlayBitmap
void
Layer::SetOverlayBitmap(const ServerBitmap* bitmap)
{
	// TODO: What about reference counting?
	// "Release" old fOverlayBitmap and "Aquire" new one?
	fOverlayBitmap = bitmap;
}

void
Layer::CopyBits(BRect& src, BRect& dst, int32 xOffset, int32 yOffset) {

	GetRootLayer()->Lock();
	do_CopyBits(src, dst, xOffset, yOffset);
	GetRootLayer()->Unlock();
}		

void
Layer::do_CopyBits(BRect& src, BRect& dst, int32 xOffset, int32 yOffset) {
	// NOTE: The correct behaviour is this:
	// * The region that is copied is the
	//   src rectangle, no matter if it fits
	//   into the dst rectangle. It is copied
	//   by the offset dst.LeftTop() - src.LeftTop()
	// * The dst rectangle is used for invalidation:
	//   Any area in the dst rectangle that could
	//   not be copied from src (because either the
	//   src rectangle was not big enough, or because there
	//   were parts cut off by the current layer clipping),
	//   are triggering BView::Draw() to be called
	//   and for these parts only.

	// TODO: having moved this into Layer broke
	// offscreen windows (bitmaps)
	// -> move back into ServerWindow...
	if (!GetRootLayer())
		return;

#ifndef NEW_CLIPPING

	// the region that is going to be copied
	BRegion copyRegion(src);
	// apply the current clipping of the layer
	copyRegion.IntersectWith(&fVisible);
	// don't scroll regions that are pending for
	// (or already in) an update
	// NOTE: this fixes the visible glitches that resulted from
	// scrolling, but it brings up a new problem, which looks
	// like the the client receives multiple update messages for
	// the same area
	// NOTE2: if you don't include both regions, it doesn't work
	copyRegion.Exclude(&fOwner->CulmulatedUpdateRegion());
	copyRegion.Exclude(&fOwner->RegionToBeUpdated());

	// offset the region to the destination
	// and apply the current clipping there as well
	copyRegion.OffsetBy(xOffset, yOffset);
	copyRegion.IntersectWith(&fVisible);

	// the region at the destination that needs invalidation
	GetRootLayer()->fRedrawReg.Set(dst);
	// exclude the region drawn by the copy operation
	GetRootLayer()->fRedrawReg.Exclude(&copyRegion);
	// apply the current clipping as well
	GetRootLayer()->fRedrawReg.IntersectWith(&fVisible);

	// move the region back for the actual operation
	copyRegion.OffsetBy(-xOffset, -yOffset);

	GetDisplayDriver()->CopyRegion(&copyRegion, xOffset, yOffset);

	// trigger the redraw			
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, NULL);
#else
	// the region that is going to be copied
	BRegion copyRegion(src);
	// apply the current clipping of the layer

	copyRegion.IntersectWith(&fVisible2);

	// offset the region to the destination
	// and apply the current clipping there as well
	copyRegion.OffsetBy(xOffset, yOffset);
	copyRegion.IntersectWith(&fVisible2);

	// the region at the destination that needs invalidation
	GetRootLayer()->fRedrawReg.Set(dst);
	// exclude the region drawn by the copy operation
// TODO: quick fix for our scrolling problem. FIX THIS!
//	GetRootLayer()->fRedrawReg.Exclude(&copyRegion);
	// apply the current clipping as well
	GetRootLayer()->fRedrawReg.IntersectWith(&fVisible2);

	// move the region back for the actual operation
	copyRegion.OffsetBy(-xOffset, -yOffset);

	GetDisplayDriver()->CopyRegion(&copyRegion, xOffset, yOffset);

	// trigger the redraw			
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, NULL);
#endif

}

#ifdef NEW_CLIPPING

void
Layer::MovedByHook(float dx, float dy)
{
	if (Window() && !IsTopLayer())
		AddToViewsWithInvalidCoords();
}

void
Layer::ResizedByHook(float dx, float dy, bool automatic)
{
	if (Window() && !IsTopLayer())
		AddToViewsWithInvalidCoords();
}

void
Layer::ScrolledByHook(float dx, float dy)
{
	// empty.
}


void
Layer::GetWantedRegion(BRegion& reg) const
{
	// this is the same as get_user_region.
	// because get_user_region modifies nothing.
	const_cast<Layer*>(this)->Layer::_GetWantedRegion(reg);
}

//! converts a point from local to parent's coordinate system 
void
Layer::ConvertToParent2(BPoint* pt) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		pt->x -= origin.x;
		pt->y -= origin.y;
		pt->x += fFrame.left;
		pt->y += fFrame.top;
	}
}

//! converts a rect from local to parent's coordinate system 
void
Layer::ConvertToParent2(BRect* rect) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		rect->OffsetBy(-origin.x, -origin.y);
		rect->OffsetBy(fFrame.left, fFrame.top);
	}
}

//! converts a region from local to parent's coordinate system 
void
Layer::ConvertToParent2(BRegion* reg) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		reg->OffsetBy(-origin.x, -origin.y);
		reg->OffsetBy(fFrame.left, fFrame.top);
	}
}

//! converts a point from parent's to local coordinate system 
void
Layer::ConvertFromParent2(BPoint* pt) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		pt->x += origin.x;
		pt->y += origin.y;
		pt->x -= fFrame.left;
		pt->y -= fFrame.top;
	}
}

//! converts a rect from parent's to local coordinate system 
void
Layer::ConvertFromParent2(BRect* rect) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		rect->OffsetBy(origin.x, origin.y);
		rect->OffsetBy(-fFrame.left, -fFrame.top);
	}
}

//! converts a region from parent's to local coordinate system 
void
Layer::ConvertFromParent2(BRegion* reg) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		reg->OffsetBy(origin.x, origin.y);
		reg->OffsetBy(-fFrame.left, -fFrame.top);
	}
}

//! converts a point from local to screen coordinate system 
void
Layer::ConvertToScreen2(BPoint* pt) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			pt->x -= origin.x;
			pt->y -= origin.y;
			pt->x += fFrame.left;
			pt->y += fFrame.top;

			fParent->ConvertToScreen2(pt);
		}
}

//! converts a rect from local to screen coordinate system 
void
Layer::ConvertToScreen2(BRect* rect) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			rect->OffsetBy(-origin.x, -origin.y);
			rect->OffsetBy(fFrame.left, fFrame.top);

			fParent->ConvertToScreen2(rect);
		}
}

//! converts a region from local to screen coordinate system 
void
Layer::ConvertToScreen2(BRegion* reg) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			reg->OffsetBy(-origin.x, -origin.y);
			reg->OffsetBy(fFrame.left, fFrame.top);

			fParent->ConvertToScreen2(reg);
		}
}

//! converts a point from screen to local coordinate system 
void
Layer::ConvertFromScreen2(BPoint* pt) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			pt->x += origin.x;
			pt->y += origin.y;
			pt->x -= fFrame.left;
			pt->y -= fFrame.top;

			fParent->ConvertToScreen2(pt);
		}
}

//! converts a rect from screen to local coordinate system 
void
Layer::ConvertFromScreen2(BRect* rect) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			rect->OffsetBy(origin.x, origin.y);
			rect->OffsetBy(-fFrame.left, -fFrame.top);

			fParent->ConvertFromScreen2(rect);
		}
}

//! converts a region from screen to local coordinate system 
void
Layer::ConvertFromScreen2(BRegion* reg) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			reg->OffsetBy(origin.x, origin.y);
			reg->OffsetBy(-fFrame.left, -fFrame.top);

			fParent->ConvertFromScreen2(reg);
		}
}


void
Layer::do_Hide()
{
	fHidden = true;

	if (fParent && !fParent->IsHidden() && GetRootLayer()) {
		// save fullVisible so we know what to invalidate
		BRegion invalid(fFullVisible2);

		clear_visible_regions();

		if (invalid.Frame().IsValid())
			fParent->do_Invalidate(invalid, this);
	}
}

void
Layer::do_Show()
{
	fHidden = false;

	if (fParent && !fParent->IsHidden() && GetRootLayer()) {
		BRegion invalid;

		_GetWantedRegion(invalid);

		if (invalid.CountRects() > 0)
			fParent->do_Invalidate(invalid, this);
	}
}

void
Layer::do_Invalidate(const BRegion &invalid, const Layer *startFrom)
{
	BRegion		localVisible(fFullVisible2);
	localVisible.IntersectWith(&invalid);
	rebuild_visible_regions(invalid, localVisible,
		startFrom? startFrom: LastChild());

	// add localVisible to our RootLayer's redraw region.
//	GetRootLayer()->fRedrawReg.Include(&localVisible);
	GetRootLayer()->fRedrawReg = localVisible;
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, NULL);
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
}

void
Layer::do_Redraw(const BRegion &invalid, const Layer *startFrom)
{
	BRegion		localVisible(fFullVisible2);
	localVisible.IntersectWith(&invalid);

	// add localVisible to our RootLayer's redraw region.
//	GetRootLayer()->fRedrawReg.Include(&localVisible);
	GetRootLayer()->fRedrawReg = localVisible;
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, NULL);
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
}

inline void
Layer::resize_layer_frame_by(float x, float y)
{
	uint16		rm = fResizeMode & 0x0000FFFF;
	BRect		newFrame = fFrame;

	if ((rm & 0x0F00U) == _VIEW_LEFT_ << 8)
		newFrame.left += 0.0f;
	else if ((rm & 0x0F00U) == _VIEW_RIGHT_ << 8)
		newFrame.left += x;
	else if ((rm & 0x0F00U) == _VIEW_CENTER_ << 8)
		newFrame.left += x/2;

	if ((rm & 0x000FU) == _VIEW_LEFT_)
		newFrame.right += 0.0f;
	else if ((rm & 0x000FU) == _VIEW_RIGHT_)
		newFrame.right += x;
	else if ((rm & 0x000FU) == _VIEW_CENTER_)
		newFrame.right += x/2;

	if ((rm & 0xF000U) == _VIEW_TOP_ << 12)
		newFrame.top += 0.0f;
	else if ((rm & 0xF000U) == _VIEW_BOTTOM_ << 12)
		newFrame.top += y;
	else if ((rm & 0xF000U) == _VIEW_CENTER_ << 12)
		newFrame.top += y/2;

	if ((rm & 0x00F0U) == _VIEW_TOP_ << 4)
		newFrame.bottom += 0.0f;
	else if ((rm & 0x00F0U) == _VIEW_BOTTOM_ << 4)
		newFrame.bottom += y;
	else if ((rm & 0x00F0U) == _VIEW_CENTER_ << 4)
		newFrame.bottom += y/2;
/*
	if (newFrame != fFrame) {
		float offsetX, offsetY;
		float dx, dy;

		dx = newFrame.Width() - fFrame.Width();
		dy = newFrame.Height() - fFrame.Height();
		offsetX = newFrame.left - fFrame.left;
		offsetY = newFrame.top - fFrame.top;

		fFrame = newFrame;

		if (offsetX != 0.0f || offsetY != 0.0f) {
			MovedByHook(offsetX, offsetY);
		}

		if (dx != 0.0f || dy != 0.0f) {
			// call hook function
			ResizedByHook(dx, dy, true); // automatic

			for (Layer *lay = LastChild(); lay; lay = PreviousChild())
				lay->resize_layer_frame_by(dx, dy);
		}
	}
*/
// TODO: the above code is CORRECT!!!
// It's commented because BView::FrameResized()/Moved() be called twice a given view. FIX THIS!
	if (newFrame != fFrame) {
		float		dx, dy;

		dx	= newFrame.Width() - fFrame.Width();
		dy	= newFrame.Height() - fFrame.Height();

		fFrame	= newFrame;

		if (dx != 0.0f || dy != 0.0f) {
			// call hook function
			ResizedByHook(dx, dy, true); // automatic

			for (Layer *lay = LastChild(); lay; lay = PreviousChild())
				lay->resize_layer_frame_by(dx, dy);
		}
		else
			MovedByHook(dx, dy);
	}
}

inline void
Layer::rezize_layer_redraw_more(BRegion &reg, float dx, float dy)
{
	if (dx == 0 && dy == 0)
		return;

	for (Layer *lay = LastChild(); lay; lay = PreviousChild()) {
		uint16		rm = lay->fResizeMode & 0x0000FFFF;

		if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT || (rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM) {
			// NOTE: this is not exactly corect, but it works :-)
			// Normaly we shoud've used the lay's old, required region - the one returned
			// from get_user_region() with the old frame, and the current one. lay->Bounds()
			// works for the moment so we leave it like this.

			// calculate the old bounds.
			BRect	oldBounds(lay->Bounds());		
			if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT)
				oldBounds.right -=dx;
			if ((rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM)
				oldBounds.bottom -=dy;
			
			// compute the region that became visible because we got bigger OR smaller.
			BRegion	regZ(lay->Bounds());
			regZ.Include(oldBounds);
			regZ.Exclude(oldBounds&lay->Bounds());

			lay->ConvertToScreen2(&regZ);

			// intersect that with this'(not lay's) fullVisible region
			regZ.IntersectWith(&fFullVisible2);
			reg.Include(&regZ);

			lay->rezize_layer_redraw_more(reg,
				(rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT? dx: 0,
				(rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM? dy: 0);

			// above, OR this:
			// reg.Include(&lay->fFullVisible2);
		}
		else
		if (((rm & 0x0F0F) == (uint16)B_FOLLOW_RIGHT && dx != 0) ||
			((rm & 0x0F0F) == (uint16)B_FOLLOW_H_CENTER && dx != 0) ||
			((rm & 0xF0F0) == (uint16)B_FOLLOW_BOTTOM && dy != 0)||
			((rm & 0xF0F0) == (uint16)B_FOLLOW_V_CENTER && dy != 0))
		{
			reg.Include(&lay->fFullVisible2);
		}
	}
}

inline void
Layer::resize_layer_full_update_on_resize(BRegion &reg, float dx, float dy)
{
	if (dx == 0 && dy == 0)
		return;

	for (Layer *lay = LastChild(); lay; lay = PreviousChild()) {
		uint16		rm = lay->fResizeMode & 0x0000FFFF;		

		if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT || (rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM) {
			if (lay->fFlags & B_FULL_UPDATE_ON_RESIZE && lay->fVisible2.CountRects() > 0)
				reg.Include(&lay->fVisible2);

			lay->resize_layer_full_update_on_resize(reg,
				(rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT? dx: 0,
				(rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM? dy: 0);
		}
	}
}

void
Layer::do_ResizeBy(float dx, float dy)
{
	fFrame.Set(fFrame.left, fFrame.top, fFrame.right+dx, fFrame.bottom+dy);

	// resize children using their resize_mask.
	for (Layer *lay = LastChild(); lay; lay = PreviousChild())
			lay->resize_layer_frame_by(dx, dy);

	// call hook function
	if (dx != 0.0f || dy != 0.0f)
		ResizedByHook(dx, dy, false); // manual

	if (!IsHidden() && GetRootLayer()) {
		BRegion oldFullVisible(fFullVisible2);
		// this is required to invalidate the old border
		BRegion oldVisible(fVisible2);

		// in case they moved, bottom, right and center aligned layers must be redrawn
		BRegion redrawMore;
		rezize_layer_redraw_more(redrawMore, dx, dy);

		// we'll invalidate the old area and the new, maxmial one.
		BRegion invalid;
		_GetWantedRegion(invalid);
		invalid.Include(&fFullVisible2);

		clear_visible_regions();

		fParent->do_RebuildVisibleRegions(invalid, this);

		// done rebuilding regions, now redraw regions that became visible

		// what's invalid, are the differences between to old and the new fullVisible region
		// 1) in case we grow.
		BRegion		redrawReg(fFullVisible2);
		redrawReg.Exclude(&oldFullVisible);
		// 2) in case we shrink
		BRegion		redrawReg2(oldFullVisible);
		redrawReg2.Exclude(&fFullVisible2);
		// 3) combine.
		redrawReg.Include(&redrawReg2);

		// for center, right and bottom alligned layers, redraw their old positions
		redrawReg.Include(&redrawMore);

		// layers that had their frame modified must be entirely redrawn.
		rezize_layer_redraw_more(redrawReg, dx, dy);

		// add redrawReg to our RootLayer's redraw region.
//		GetRootLayer()->fRedrawReg.Include(&redrawReg);
		GetRootLayer()->fRedrawReg = redrawReg;
		// include layer's visible region in case we want a full update on resize
		if (fFlags & B_FULL_UPDATE_ON_RESIZE && fVisible2.Frame().IsValid()) {
			resize_layer_full_update_on_resize(GetRootLayer()->fRedrawReg, dx, dy);

			GetRootLayer()->fRedrawReg.Include(&fVisible2);
			GetRootLayer()->fRedrawReg.Include(&oldVisible);
		}
		// clear canvas and set invalid regions for affected WinBorders
		GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, NULL);
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}

	SendViewCoordUpdateMsg();
}

void Layer::do_MoveBy(float dx, float dy)
{
	if (dx == 0.0f && dy == 0.0f)
		return;

//	fFrame.Set(fFrame.left+dx, fFrame.top+dy, fFrame.right+dx, fFrame.bottom+dy);
	fFrame.OffsetBy(dx, dy);

	// call hook function
	MovedByHook(dx, dy);

	if (!IsHidden() && GetRootLayer()) {
		BRegion oldFullVisible(fFullVisible2);

		// we'll invalidate the old position and the new, maxmial one.
		BRegion invalid;
		_GetWantedRegion(invalid);
		invalid.Include(&fFullVisible2);

		clear_visible_regions();

		fParent->do_RebuildVisibleRegions(invalid, this);

		// done rebuilding regions, now copy common parts and redraw regions that became visible

		// include the actual and the old fullVisible regions. later, we'll exclude the common parts.
		BRegion		redrawReg(fFullVisible2);
		redrawReg.Include(&oldFullVisible);

		// offset to layer's new location so that we can calculate the common region.
		oldFullVisible.OffsetBy(dx, dy);

		// finally we have the region that needs to be redrawn.
		redrawReg.Exclude(&oldFullVisible);

		// by intersecting the old fullVisible offseted to layer's new location, with the current
		// fullVisible, we'll have the common region which can be copied using HW acceleration.
		oldFullVisible.IntersectWith(&fFullVisible2);

		// offset back and instruct the HW to do the actual copying.
		oldFullVisible.OffsetBy(-dx, -dy);
		GetDisplayDriver()->CopyRegion(&oldFullVisible, dx, dy);

		// add redrawReg to our RootLayer's redraw region.
//		GetRootLayer()->fRedrawReg.Include(&redrawReg);
		GetRootLayer()->fRedrawReg = redrawReg;
		GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, NULL);
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}

	SendViewCoordUpdateMsg();
}

void
Layer::do_ScrollBy(float dx, float dy)
{
	fDrawState->OffsetOrigin(BPoint(dx, dy));
//	fOrigin.Set(fOrigin.x + dx, fOrigin.y + dy);

	if (!IsHidden() && GetRootLayer()) {
		// set the region to be invalidated.
		BRegion		invalid(fFullVisible2);

		clear_visible_regions();

		rebuild_visible_regions(invalid, invalid, LastChild());

		// for the moment we say that the whole surface needs to be redraw.
		BRegion		redrawReg(fFullVisible2);

		// offset old region so that we can start comparing.
		invalid.OffsetBy(dx, dy);

		// compute the common region. we'll use HW acc to copy this to the new location.
		invalid.IntersectWith(&fFullVisible2);
		GetDisplayDriver()->CopyRegion(&invalid, -dx, -dy);

		// common region goes back to its original location. then, by excluding
		// it from curent fullVisible we'll obtain the region that needs to be redrawn.
		invalid.OffsetBy(-dx, -dy);
// TODO: a quick fix for the scrolling problem!!! FIX THIS!
//		redrawReg.Exclude(&invalid);

//		GetRootLayer()->fRedrawReg.Include(&redrawReg);
		GetRootLayer()->fRedrawReg = redrawReg;
		GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, NULL);
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}

	if (dx != 0.0f || dy != 0.0f)
		ScrolledByHook(dx, dy);

	SendViewCoordUpdateMsg();
}

void
Layer::_GetWantedRegion(BRegion &reg)
{
	// 1) set to frame in screen coords
	BRect screenFrame(Bounds());
	ConvertToScreen2(&screenFrame);
	reg.Set(screenFrame);

	// 2) intersect with screen region
	BRegion screenReg(GetRootLayer()->Bounds());
	reg.IntersectWith(&screenReg);


	// 3) impose user constrained regions
	DrawState *stackData = fDrawState;
	while (stackData) {
		if (stackData->ClippingRegion()) {
			// transform in screen coords
// NOTE: Already is in screen coords, but I leave this here in
// case we change it
//			BRegion screenReg(*stackData->ClippingRegion());
//			ConvertToScreen2(&screenReg);
//			reg.IntersectWith(&screenReg);
			reg.IntersectWith(stackData->ClippingRegion());
		}
		stackData = stackData->PreviousState();
	}
}

void
Layer::do_RebuildVisibleRegions(const BRegion &invalid, const Layer *startFrom)
{
	BRegion		localVisible(fFullVisible2);
	localVisible.IntersectWith(&invalid);
	rebuild_visible_regions(invalid, localVisible, startFrom);
}

void
Layer::rebuild_visible_regions(const BRegion &invalid,
								const BRegion &parentLocalVisible,
								const Layer *startFrom)
{
	// no point in continuing if this layer is hidden.
	if (fHidden)
		return;

	// no need to go deeper if the parent doesn't have a visible region anymore
	// and our fullVisible region is also empty.
	if (!parentLocalVisible.Frame().IsValid() && !(fFullVisible2.CountRects() > 0))
		return;

	bool fullRebuild = false;

	// intersect maximum wanted region with the invalid region
	BRegion common;
	_GetWantedRegion(common);
	common.IntersectWith(&invalid);

	// if the resulted region is not valid, this layer is not in the catchment area
	// of the region being invalidated
	if (!common.CountRects() > 0)
		return;

	// now intersect with parent's visible part of the region that was/is invalidated
	common.IntersectWith(&parentLocalVisible);

	// exclude the invalid region
	fFullVisible2.Exclude(&invalid);
	fVisible2.Exclude(&invalid);

	// put in what's really visible
	fFullVisible2.Include(&common);

	// allow this layer to hide some parts from its children
	_ReserveRegions(common);

	for (Layer *lay = LastChild(); lay; lay = PreviousChild()) {
		if (lay == startFrom)
			fullRebuild = true;

		if (fullRebuild)
			lay->rebuild_visible_regions(invalid, common, lay->LastChild());

		// to let children know much they can take from parent's visible region
		common.Exclude(&lay->fFullVisible2);
	}

	// include what's left after all children took what they could.
	fVisible2.Include(&common);
}

void
Layer::_ReserveRegions(BRegion &reg)
{
	// Empty for Layer objects
}

void
Layer::clear_visible_regions()
{
	// OPT: maybe we should uncomment these lines for performance
	//if (fFullVisible2.CountRects() <= 0)
	//	return;

	fVisible2.MakeEmpty();
	fFullVisible2.MakeEmpty();
	for (Layer *child = LastChild(); child; child = PreviousChild())
		child->clear_visible_regions();
}

#endif

