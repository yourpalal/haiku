/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DefaultMediaTheme.h"
#include "debug.h"

#include <ParameterWeb.h>

#include <Slider.h>
#include <StringView.h>
#include <Button.h>
#include <TextControl.h>
#include <OptionPopUp.h>
#include <ChannelSlider.h>
#include <Box.h>
#include <CheckBox.h>
#include <TabView.h>
#include <MenuField.h>


using namespace BPrivate;


namespace BPrivate {

class SeparatorView : public BView {
	public:
		SeparatorView(BRect frame);
		virtual ~SeparatorView();

		virtual void Draw(BRect updateRect);

	private:
		bool	fVertical;
};

class TitleView : public BView {
	public:
		TitleView(BRect frame, const char *title);
		virtual ~TitleView();

		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float *width, float *height);

	private:
		const char *fTitle;
};

}	// namespace BPrivate


SeparatorView::SeparatorView(BRect frame)
	: BView(frame, "-", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fVertical = frame.Width() < frame.Height();
}


SeparatorView::~SeparatorView()
{
}


void 
SeparatorView::Draw(BRect updateRect)
{
	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect rect = updateRect & Bounds();

	SetHighColor(tint_color(color, B_DARKEN_1_TINT));
	if (fVertical)
		StrokeLine(BPoint(0, rect.top), BPoint(0, rect.bottom));
	else
		StrokeLine(BPoint(rect.left, 0), BPoint(rect.right, 0));

	SetHighColor(tint_color(color, B_LIGHTEN_1_TINT));
	if (fVertical)
		StrokeLine(BPoint(1, rect.top), BPoint(1, rect.bottom));
	else
		StrokeLine(BPoint(rect.left, 1), BPoint(rect.right, 1));
}


//	#pragma mark -


TitleView::TitleView(BRect frame, const char *title)
	: BView(frame, title, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	fTitle = strdup(title);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ViewColor());
}


TitleView::~TitleView()
{
	free((char *)fTitle);
}


void 
TitleView::Draw(BRect updateRect)
{
	BRect rect(Bounds());

	SetDrawingMode(B_OP_COPY);
	SetHighColor(240, 240, 240);
	DrawString(fTitle, BPoint(rect.left + 1, rect.bottom - 9));

	SetDrawingMode(B_OP_OVER);
	SetHighColor(80, 20, 20);
	DrawString(fTitle, BPoint(rect.left, rect.bottom - 8));

	//SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	//StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.right, rect.bottom));
}


void 
TitleView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = StringWidth(fTitle) + 2;

	if (_height) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		*_height = fontHeight.ascent + fontHeight.descent + fontHeight.leading + 8;
	}
}


//	#pragma mark -


DefaultMediaTheme::DefaultMediaTheme()
	: BMediaTheme("BeOS Theme", "BeOS built-in theme version 0.1")
{
	CALLED();
}


BControl *
DefaultMediaTheme::MakeControlFor(BParameter *parameter)
{
	CALLED();

	BRect rect(0, 0, 150, 100);
	return MakeViewFor(parameter, &rect);
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterWeb *web, const BRect *hintRect)
{
	CALLED();

	if (web == NULL)
		return NULL;

	BRect rect;
	if (hintRect)
		rect = *hintRect;
	else
		rect.Set(0, 0, 80, 100);

	// do we have more than one attached parameter group?
	// if so, use a tabbed view with a tab for each group

	BTabView *tabView = NULL;

	if (web->CountGroups() > 1)
		tabView = new BTabView(rect, "web");

	rect.OffsetTo(B_ORIGIN);

	for (int32 i = 0; i < web->CountGroups(); i++) {
		BParameterGroup *group = web->GroupAt(i);
		if (group == NULL)
			continue;

		BView *groupView = MakeViewFor(*group, rect);
		if (groupView == NULL)
			continue;

		if (tabView == NULL) {
			// if we don't need a container to put that view into,
			// we're done here
			return groupView;
		}

		tabView->AddTab(groupView);

		if (groupView->Bounds().Width() + 5 > rect.Width())
			rect.right = groupView->Bounds().Width() - 1;
	}

	if (tabView != NULL) {
		tabView->ResizeTo(rect.right + 10, rect.bottom + tabView->TabHeight());

		rect = tabView->Bounds();
		rect.InsetBySelf(1, 1);
		tabView->ContainerView()->ResizeTo(rect.Width(), rect.Height());
	}

	return tabView;
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterGroup &group, const BRect &hintRect)
{
	CALLED();

	BRect rect(hintRect);
	BBox *view = new BBox(rect, group.Name());
	view->SetBorder(B_NO_BORDER);

	// Add the sub-group views

	rect.OffsetTo(B_ORIGIN);
	rect.InsetBySelf(5, 5);
	rect.right = rect.left + 50;

	for (int32 i = 0; i < group.CountGroups(); i++) {
		BParameterGroup *subGroup = group.GroupAt(i);
		if (subGroup == NULL)
			continue;

		BView *groupView = MakeViewFor(*subGroup, rect);
		if (groupView == NULL)
			continue;

		if (i > 0) {
			// add separator view
			BRect separatorRect(rect);
			separatorRect.left -= 3;
			separatorRect.right = separatorRect.left + 1;

			view->AddChild(new SeparatorView(separatorRect));
		}

		view->AddChild(groupView);

		rect.OffsetBy(groupView->Bounds().Width() + 5, 0);

		if (groupView->Bounds().Height() > rect.Height())
			rect.bottom = groupView->Bounds().Height() - 1;
	}

	view->ResizeTo(rect.left + 10, rect.bottom + 10);
	rect.left = 5;

	// add the parameter views part of the group

	if (group.CountParameters() == 0)
		return view;

	if (view->CountChildren() > 0)
		rect.OffsetBy(0, rect.Height() + 10);

	BList views;
	bool center = false;

	for (int32 i = 0; i < group.CountParameters(); i++) {
		BParameter *parameter = group.ParameterAt(i);
		if (parameter == NULL)
			continue;

		BView *parameterView = MakeSelfHostingViewFor(*parameter, rect);
		if (parameterView == NULL)
			continue;

		// if there is a BChannelSlider (ToDo: or any vertical slider?)
		// the views will be centered
		if (dynamic_cast<BChannelSlider *>(parameterView) != NULL)
			center = true;

		parameterView->SetViewColor(view->ViewColor());
			// ToDo: dunno why this is needed, but the controls
			// sometimes (!) have a white background without it

		view->AddChild(parameterView);
		views.AddItem(parameterView);

		rect.OffsetBy(0, parameterView->Bounds().Height() + 5);

		if (parameterView->Bounds().Width() + 5 > rect.Width())
			rect.right = parameterView->Bounds().Width() + rect.left + 5;
	}

	if (group.CountParameters() > 0)
		view->ResizeTo(rect.right + 5, rect.top + 5);

	// center the parameter views if needed, and tweak some views

	float width = view->Bounds().Width();

	for (int32 i = 0; i < views.CountItems(); i++) {
		BView *subView = static_cast<BView *>(views.ItemAt(i));
		BRect frame = subView->Frame();

		if (center)
			subView->MoveTo((width - frame.Width()) / 2, frame.top);
		else {
			// tweak the PopUp views to look better
			if (dynamic_cast<BOptionPopUp *>(subView) != NULL) {
				subView->ResizeTo(width, frame.Height());
				//subView->SetDivider(10);
			}
		}
	}

	return view;
}


/** This creates a view that handles all incoming messages itself - that's
 *	what is meant with self-hosting.
 */

BView *
DefaultMediaTheme::MakeSelfHostingViewFor(BParameter &parameter, const BRect &hintRect)
{
	BView *view = MakeViewFor(&parameter, &hintRect);
	if (view == NULL) {
		// The MakeViewFor() method above returns a BControl - which we
		// don't need for a null parameter; that's why it returns NULL.
		// But we want to see something anyway, so we add a string view
		// here.
		if (parameter.Type() == BParameter::B_NULL_PARAMETER) {
			if (parameter.Group()->ParameterAt(0) == &parameter) {
				// this is the first parameter in this group, so
				// let's use a nice title view
				
				TitleView *titleView = new TitleView(hintRect, parameter.Name());
				titleView->ResizeToPreferred();

				return titleView;
			}
			BStringView *stringView = new BStringView(hintRect, parameter.Name(), parameter.Name());
			stringView->SetAlignment(B_ALIGN_CENTER);
			stringView->ResizeToPreferred();

			return stringView;
		}

		return NULL;
	}

	// ToDo: create message filters for this view!

	return view;
}


BControl *
DefaultMediaTheme::MakeViewFor(BParameter *parameter, const BRect *hintRect)
{
	BRect rect;
	if (hintRect)
		rect = *hintRect;
	else
		rect.Set(0, 0, 50, 100);

	switch (parameter->Type()) {
		case BParameter::B_NULL_PARAMETER:
			// there is no default view for a null parameter
			return NULL;

		case BParameter::B_DISCRETE_PARAMETER:
		{
			BDiscreteParameter &discrete = static_cast<BDiscreteParameter &>(*parameter);

			if (!strcmp(discrete.Kind(), B_ENABLE)
				|| !strcmp(discrete.Kind(), B_MUTE)
				|| discrete.CountItems() == 0) {
				// create a checkbox item

				BCheckBox *checkBox = new BCheckBox(rect, discrete.Name(), discrete.Name(), NULL);
				checkBox->ResizeToPreferred();

				return checkBox;
			} else {
				// create a pop up menu field

				// ToDo: replace BOptionPopUp (or fix it in OpenBeOS...)
				// this is a workaround for a bug in BOptionPopUp - you need to
				// know the actual width before creating the object - very nice...

				BFont font;
				float width = 0;
				for (int32 i = 0; i < discrete.CountItems(); i++) {
					float labelWidth = font.StringWidth(discrete.ItemNameAt(i));
					if (labelWidth > width)
						width = labelWidth;
				}
				width += font.StringWidth(discrete.Name()) + 55;
				rect.right = rect.left + width;

				BOptionPopUp *popUp = new BOptionPopUp(rect, discrete.Name(), discrete.Name(), NULL);

				for (int32 i = 0; i < discrete.CountItems(); i++) {
					popUp->AddOption(discrete.ItemNameAt(i), discrete.ItemValueAt(i));
				}

				popUp->ResizeToPreferred();

				return popUp;
			}
		}

		case BParameter::B_CONTINUOUS_PARAMETER:
		{
			BContinuousParameter &continuous = static_cast<BContinuousParameter &>(*parameter);

			if (!strcmp(continuous.Kind(), B_MASTER_GAIN)
				|| !strcmp(continuous.Kind(), B_GAIN))
			{
				BChannelSlider *slider = new BChannelSlider(rect, continuous.Name(),
					continuous.Name(), NULL, B_VERTICAL, continuous.CountChannels());

				char minLabel[64], maxLabel[64];

				const char *unit = continuous.Unit();
				if (unit[0]) {
					// if we have a unit, print it next to the limit values
					sprintf(minLabel, "%g %s", continuous.MinValue(), continuous.Unit());
					sprintf(maxLabel, "%g %s", continuous.MaxValue(), continuous.Unit());
				} else {
					sprintf(minLabel, "%g", continuous.MinValue());
					sprintf(maxLabel, "%g", continuous.MaxValue());
				}
				slider->SetLimitLabels(minLabel, maxLabel);

				float width, height;
				slider->GetPreferredSize(&width, &height);
				slider->ResizeTo(width, 190);

				// ToDo: take BContinuousParameter::GetResponse() & ValueStep() into account!

				for (int32 i = 0; i < continuous.CountChannels(); i++)
					slider->SetLimitsFor(i, continuous.MinValue(), continuous.MaxValue());

				return slider;
			}

			BSlider *slider = new BSlider(rect, parameter->Name(), parameter->Name(),
				NULL, 0, 100);

			float width, height;
			slider->GetPreferredSize(&width, &height);
			slider->ResizeTo(100, height);

			return slider;
		}

		default:
			ERROR("BMediaTheme: Don't know parameter type: 0x%lx\n", parameter->Type());
	}
	return NULL;
}

