//-----------------------------------------------------------------------------
//	Copyright (c) 2003 Stefano Ceccherini
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
//	File Name:		OptionControl.cpp
//	Description:	An abstract base class for option controls.  
//------------------------------------------------------------------------------
#include <OptionControl.h>

#include <cstring>

/*! \brief Creates and initializes a BOptionControl.
	\param frame The control's frame rectangle.
	\param name The control's name.
	\param label The label displayed by the control.
	\param message The message which the control will send when operated.
	\param resize Resize mask, passed to the base class's constructor.
	\param flags View flags, passed to the base class's constructor.
*/
BOptionControl::BOptionControl(BRect frame, const char *name, const char *label,
								BMessage *message, uint32 resize, uint32 flags)
	:
	BControl(frame, name, label, message, resize, flags)
{
}


BOptionControl::~BOptionControl()
{
}


void
BOptionControl::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OPTION_CONTROL_VALUE:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK)
				SetValue(value);				
			break;
		}
		default:
			BControl::MessageReceived(message);
			break;
	}
}


/*! \brief Adds an "option" after the last one.
	\param name The name of the option.
	\param value The value of the option.
	\return \c B_OK if the option was added succesfully,
		an error code otherwise.
*/
status_t
BOptionControl::AddOption(const char *name, int32 value)
{
	int32 numOptions = CountOptions();
	return AddOptionAt(name, value, numOptions);
}


/*! \brief Select the option which has the given value.
	\param value The value of the option.
	\return \c B_OK if there was an option with that value,
		and it was correctly selected, an error code otherwise.
	It works exactly as SetValue(value);
*/
status_t
BOptionControl::SelectOptionFor(int32 value)
{
	// XXX: I wonder why this method was created in the first place,
	// since you can obtain the same result simply by calling SetValue().
	// The only difference I can see is that this method iterates over 
	// all the options contained in the contrl, and thens elects the right one.
	int32 numOptions = CountOptions();
	for (int32 c = 0; c < numOptions; c++) {
		const char *name;
		int32 optionValue;
		if (GetOptionAt(c, &name, &optionValue) && optionValue == value) {
			SetValue(optionValue);
			return B_OK;
		}
	}

	return B_ERROR;
}


/*! \brief Select the option which has the given name.
	\param name The name of the option.
	\return \c B_OK if there was an option with that name,
		and it was correctly selected, an error code otherwise.
*/
status_t
BOptionControl::SelectOptionFor(const char *name)
{
	int32 numOptions = CountOptions();
	for (int32 c = 0; c < numOptions; c++) {
		const char *optionName = NULL;
		int32 optionValue;
		if (GetOptionAt(c, &optionName, &optionValue)
						&& !strcmp(name, optionName)) {
			SetValue(optionValue);
			return B_OK;
		}
	}
	return B_ERROR;
}


// Protected
/*! \brief Creates a BMessage which contains the given value.
	\param The value to be added to the message.
	\return A pointer to a BMessage, NULL if something went wrong.
*/
BMessage *
BOptionControl::MakeValueMessage(int32 value)
{
	BMessage *message = new BMessage(B_OPTION_CONTROL_VALUE);	
	if (message->AddInt32("be:value", value) != B_OK) {
		delete message;
		message = NULL;
	}		
	
	return message;
}


// FBC
status_t BOptionControl::_Reserved_OptionControl_0(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_1(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_2(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_3(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_4(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_5(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_6(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_7(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_8(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_9(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_10(void *, ...) { return B_ERROR; }
status_t BOptionControl::_Reserved_OptionControl_11(void *, ...) { return B_ERROR; }
