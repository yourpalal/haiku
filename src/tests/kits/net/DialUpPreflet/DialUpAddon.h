/*
	This software may be distributed under the OpenBeOS license.
	Copyright (c) 2004, OpenBeOS
	
	Dial-Up add-ons must export the following function:
	bool register(BMessage *addons);
	
	Most add-ons are simple pointers to a DialUpAddon object.
	
	Note for tab-addons: The BView object is deleted AFTER the DialUpAddon (except you
	remove and delete it in the DialUpAddon destructor).
*/

#ifndef _DIAL_UP_ADDON__H
#define _DIAL_UP_ADDON__H

#include <SupportDefs.h>
#include <Point.h>


#define DUN_MAXIMUM_PRIORITY			50

// add-on types
#define DUN_TAB_ADDON_TYPE				"Tab"
#define DUN_AUTHENTICATOR_ADDON_TYPE	"Authenticator"
#define DUN_DEVICE_ADDON_TYPE			"Device"
#define DUN_PROTOCOL_ADDON_TYPE			"Protocol"

// other information contained in the add-ons BMessage object
#define DUN_DELETE_ON_QUIT				"DeleteMe"
	// the DialUpAddon objects in this list will be deleted when the preflet quits
#define DUN_MESSENGER					"Messenger"
#define DUN_TAB_VIEW_RECT				"TabViewRect"
#define DUN_DEVICE_VIEW_WIDTH			"DeviceViewWidth"


class DialUpAddon {
		friend class DialUpView;

	public:
		DialUpAddon(BMessage *addons) : fAddons(addons) {}
		virtual ~DialUpAddon() {}
		
		BMessage *Addons() const
			{ return fAddons; }
		
		virtual const char *FriendlyName() const
			{ return NULL; }
		virtual const char *TechnicalName() const
			{ return NULL; }
		virtual const char *KernelModuleName() const
			{ return NULL; }
				// the kernel module for this add-on (if needed)
		virtual int32 Position() const
			{ return -1; }
				// mostly used by tabs to describe where they should appear
		virtual int32 Priority() const
			{ return 0; }
				// allows to set order in which modules are asked to add the settings
		
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
			{ return false; }
		virtual bool HasTemporaryProfile() const
			{ return false; }
		virtual void IsModified(bool *settings, bool *profile) const
			{ *settings = *profile = false; }
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
				bool saveTemporary)
			{ return false; }
				// temporary settings are passwords, for example
		virtual bool GetPreferredSize(float *width, float *height) const
			{ return false; }
				// if false is returned your add-on does not want to add a view
		virtual BView *CreateView(BPoint leftTop)
			{ return NULL; }

	private:
		virtual void _Reserved1() {}
		virtual void _Reserved2() {}
		virtual void _Reserved3() {}
		virtual void _Reserved4() {}

	private:
		BMessage *fAddons;
		
		int32 _reserved[7];
};


#endif
