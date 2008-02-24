/*
 * Copyright 2001-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

//!	BMenuWindow is a custom BWindow for BMenus.

#include <Debug.h>
#include <Menu.h>

#include <MenuPrivate.h>
#include <MenuWindow.h>
#include <WindowPrivate.h>


namespace BPrivate {

class BMenuScroller : public BView {
	public:
		BMenuScroller(BRect frame);
		
		bool IsEnabled() const;
		void SetEnabled(const bool &enabled);
	private:
		bool fEnabled;
};


class BMenuFrame : public BView {
	public:
		BMenuFrame(BMenu *menu);
		
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void Draw(BRect updateRect);
		
	private:
		friend class BMenuWindow;

		BMenu *fMenu;
};


class UpperScroller : public BMenuScroller {
public:
	UpperScroller(BRect frame);
	virtual void Draw(BRect updateRect);
};


class LowerScroller : public BMenuScroller {
public:
	LowerScroller(BRect frame);
	virtual void Draw(BRect updateRect);
};


}	// namespace BPrivate


using namespace BPrivate;


const int kScrollerHeight = 10;
const int kScrollStep = 19;


BMenuScroller::BMenuScroller(BRect frame)
	: BView(frame, "menu scroller", 0, B_WILL_DRAW | B_FRAME_EVENTS),
	fEnabled(false)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));	
}


bool
BMenuScroller::IsEnabled() const
{
	return fEnabled;
}


void
BMenuScroller::SetEnabled(const bool &enabled)
{
	fEnabled = enabled;
}


//	#pragma mark -


UpperScroller::UpperScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


void
UpperScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	float middle = Bounds().right / 2;

	// Draw the upper arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), 
					B_DARKEN_2_TINT));

	FillRect(Bounds(), B_SOLID_LOW);
	
	FillTriangle(BPoint(middle, (kScrollerHeight / 2) - 3),
			BPoint(middle + 5, (kScrollerHeight / 2) + 2),
			BPoint(middle - 5, (kScrollerHeight / 2) + 2));
}


//	#pragma mark -


LowerScroller::LowerScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


void
LowerScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	
	BRect frame = Bounds();
	// Draw the lower arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));

	FillRect(frame, B_SOLID_LOW);

	float middle = Bounds().right / 2;

	FillTriangle(BPoint(middle, frame.bottom - (kScrollerHeight / 2) + 3),
			BPoint(middle + 5, frame.bottom - (kScrollerHeight / 2) - 2),
			BPoint(middle - 5, frame.bottom - (kScrollerHeight / 2) - 2));
}


//	#pragma mark -


BMenuFrame::BMenuFrame(BMenu *menu)
	: BView(BRect(0, 0, 1, 1), "menu frame", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fMenu(menu)
{
}
	

void
BMenuFrame::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	if (fMenu != NULL)
		AddChild(fMenu);
		
	ResizeTo(Window()->Bounds().Width(), Window()->Bounds().Height());
	if (fMenu != NULL) {
		BFont font;
		fMenu->GetFont(&font);
		SetFont(&font);
	}
}
	

void
BMenuFrame::DetachedFromWindow()
{
	if (fMenu != NULL)
		RemoveChild(fMenu);
}


void
BMenuFrame::Draw(BRect updateRect)
{
	if (fMenu != NULL && fMenu->CountItems() == 0) {
		// TODO: Review this as it's a bit hacky.
		// Menu has a size of 0, 0, since there are no items in it.
		// So the BMenuFrame class has to fake it and draw an empty item.
		// Note that we can't add a real "empty" item because then we couldn't
		// tell if the item was added by us or not.
		// See also BMenu::UpdateWindowViewSize()
		SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
		SetLowColor(HighColor());
		FillRect(updateRect);

		font_height height;
		GetFontHeight(&height);
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT));
		BPoint where((Bounds().Width() - fMenu->StringWidth(kEmptyMenuLabel)) / 2, ceilf(height.ascent + 1));
		DrawString(kEmptyMenuLabel, where);
	}

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));	
	BRect bounds(Bounds());

	StrokeLine(BPoint(bounds.right, bounds.top),
		BPoint(bounds.right, bounds.bottom - 1));
	StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
		BPoint(bounds.right, bounds.bottom));
}



//	#pragma mark -


BMenuWindow::BMenuWindow(const char *name)
	// The window will be resized by BMenu, so just pass a dummy rect
	: BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
			B_NOT_ZOOMABLE | B_AVOID_FOCUS),
	fMenu(NULL),	
	fMenuFrame(NULL),
	fUpperScroller(NULL),
	fLowerScroller(NULL)
{
}


BMenuWindow::~BMenuWindow()
{
	DetachMenu();
}


void
BMenuWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	BWindow::DispatchMessage(message, handler);
}


void
BMenuWindow::AttachMenu(BMenu *menu)
{
	if (fMenuFrame)
		debugger("BMenuWindow: a menu is already attached!");
	if (menu != NULL) {
		fMenuFrame = new BMenuFrame(menu);
		AddChild(fMenuFrame);
		menu->MakeFocus(true);
		fMenu = menu;
	}
}


void
BMenuWindow::DetachMenu()
{
	DetachScrollers();	
	if (fMenuFrame) {
		RemoveChild(fMenuFrame);
		delete fMenuFrame;
		fMenuFrame = NULL;
		fMenu = NULL;
	}
}


void
BMenuWindow::AttachScrollers()
{
	// We want to attach a scroller only if there's a
	// menu frame already existing.
	if (!fMenu || !fMenuFrame)
		return;
 	
	fMenu->MakeFocus(true);

	BRect frame = Bounds();
	
	if (fUpperScroller == NULL) {
		fUpperScroller = new UpperScroller(
							BRect(0, 0, frame.right, kScrollerHeight));
		AddChild(fUpperScroller);
	}
	
	if (fLowerScroller == NULL) {
		fLowerScroller = new LowerScroller(
							BRect(0, frame.bottom - kScrollerHeight,
										frame.right, frame.bottom));
		AddChild(fLowerScroller);
	}
	
	fUpperScroller->SetEnabled(false);
	fLowerScroller->SetEnabled(true);

	fMenuFrame->ResizeBy(0, -2 * kScrollerHeight - 1);
	fMenuFrame->MoveBy(0, kScrollerHeight);
	
	fValue = 0;
	fLimit = fMenu->Bounds().Height() - (frame.Height() - 2 * kScrollerHeight);
}


void
BMenuWindow::DetachScrollers()
{
	// BeOS doesn't remember the position where the last scrolling ended,
	// so we just scroll back to the beginning.
	if (fMenu)	
		fMenu->ScrollTo(0, 0);

	if (fLowerScroller) {
		RemoveChild(fLowerScroller);
		delete fLowerScroller;
		fLowerScroller = NULL;
	}

	if (fUpperScroller) {
		RemoveChild(fUpperScroller);
		delete fUpperScroller;
		fUpperScroller = NULL;
	}		
}


bool
BMenuWindow::CheckForScrolling(BPoint cursor)
{
	if (!fMenuFrame || !fUpperScroller || !fLowerScroller)
		return false;
	
	return _Scroll(cursor);
}


bool
BMenuWindow::_Scroll(BPoint cursor)
{
	ASSERT((fLowerScroller != NULL));
	ASSERT((fUpperScroller != NULL));
	
	ConvertFromScreen(&cursor);

	BRect lowerFrame = fLowerScroller->Frame();
	BRect upperFrame = fUpperScroller->Frame();

	if (fLowerScroller->IsEnabled() && lowerFrame.Contains(cursor)) {		
		if (fValue == 0) {
			fUpperScroller->SetEnabled(true);
			fUpperScroller->Invalidate();
		}

		if (fValue + kScrollStep >= fLimit) {
			// If we reached the limit, we don't want to scroll a whole
			// 'step' if not needed.
			fMenu->ScrollBy(0, fLimit - fValue);
			fValue = fLimit;
			fLowerScroller->SetEnabled(false);
			fLowerScroller->Invalidate();

		} else {
			fMenu->ScrollBy(0, kScrollStep);
			fValue += kScrollStep;
		}
	} else if (fUpperScroller->IsEnabled() && upperFrame.Contains(cursor)) {		
		if (fValue == fLimit) {
			fLowerScroller->SetEnabled(true);
			fLowerScroller->Invalidate();
		}

		if (fValue - kScrollStep <= 0) {
			fMenu->ScrollBy(0, -fValue);
			fValue = 0;
			fUpperScroller->SetEnabled(false);
			fUpperScroller->Invalidate();

		} else {
			fMenu->ScrollBy(0, -kScrollStep);
			fValue -= kScrollStep;
		}
	} else
		return false;

	snooze(10000);

	return true;
}

