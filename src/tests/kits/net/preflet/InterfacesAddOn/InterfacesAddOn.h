/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 * 		Fredrik Modéen
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 */
#ifndef INTERFACES_ADDON_H
#define INTERFACES_ADDON_H


#include <Box.h>
#include <ListView.h>
#include <ListItem.h>
#include <Button.h>

#include "NetworkSetupAddOn.h"
#include "InterfacesListView.h"


class InterfacesAddOn : public NetworkSetupAddOn, public BBox
{
public:
								InterfacesAddOn(image_id addon_image);
								~InterfacesAddOn();

			const char* 		Name();
			status_t			Save();

			BView*				CreateView(BRect *bounds);

	static	const uint32        kMsgInterfaceSel = 'ifce';
	static	const uint32        kMsgInterfaceAct = 'ifac';
	static	const uint32        kMsgInterfaceCfg = 'ifcf';
	static	const uint32        kMsgInterfaceTog = 'onof';

			void				AttachedToWindow();
			void				MessageReceived(BMessage* msg);

private:
			InterfacesListView*	fListview;
			BButton*			fConfigure;
			BButton*			fOnOff;
};


#endif /*INTERFACES_ADDON_H*/

