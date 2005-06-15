/*
 * Copyright 2002-2005 Haiku
 * Distributed under the terms of the MIT license.
 *
 * Updated by Sikosis (beos@gravity24hr.com)
 *
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Written by:	Daniel Switkin
 */


#include "PulseApp.h"
#include "Common.h"
#include "PulseWindow.h"
#include "DeskbarPulseView.h"

#include <Alert.h>
#include <Rect.h>
#include <Screen.h>
#include <Deskbar.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>


PulseApp::PulseApp(int argc, char **argv)
	: BApplication(APP_SIGNATURE)
{
	prefs = new Prefs();

	int mini = false, deskbar = false, normal = false;
	uint32 framecolor = 0, activecolor = 0, idlecolor = 0;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"deskbar", 0, &deskbar, true},
			{"width", 1, 0, 'w'},
			{"framecolor", 1, 0, 0},
			{"activecolor", 1, 0, 0},
			{"idlecolor", 1, 0, 0},
			{"mini", 0, &mini, true},
			{"normal", 0, &normal, true},
			{"help", 0, 0, 'h'},
			{0,0,0,0}
		};
		int c = getopt_long(argc, argv, "hw:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				switch (option_index) {
					case 2: /* framecolor */
					case 3: /* activecolor */
					case 4: /* idlecolor */
						uint32 rgb = strtoul(optarg, NULL, 0);
						rgb = rgb << 8;
						rgb |= 0x000000ff;
						
						switch (option_index) {
							case 2:
								framecolor = rgb;
								break;
							case 3:
								activecolor = rgb;
								break;
							case 4:
								idlecolor = rgb;
								break;
						}
						break;
				}
				break;
			case 'w':
				prefs->deskbar_icon_width = atoi(optarg);
				if (prefs->deskbar_icon_width < GetMinimumViewWidth())
					prefs->deskbar_icon_width = GetMinimumViewWidth();
				else if (prefs->deskbar_icon_width > 50) prefs->deskbar_icon_width = 50;
				break;
			case 'h':
			case '?':
				Usage();
				break;
			default:
				printf("?? getopt returned character code 0%o ??\n", c);
				break;
		}
	}
	
	if (deskbar) {
		prefs->window_mode = DESKBAR_MODE;
		if (activecolor != 0)
			prefs->deskbar_active_color = activecolor;
		if (idlecolor != 0)
			prefs->deskbar_idle_color = idlecolor;
		if (framecolor != 0)
			prefs->deskbar_frame_color = framecolor;
	} else if (mini) {
		prefs->window_mode = MINI_WINDOW_MODE;
		if (activecolor != 0)
			prefs->mini_active_color = activecolor;
		if (idlecolor != 0)
			prefs->mini_idle_color = idlecolor;
		if (framecolor != 0)
			prefs->mini_frame_color = framecolor;
	} else if (normal)
		prefs->window_mode = NORMAL_WINDOW_MODE;

	prefs->Save();
	BuildPulse();
}


void
PulseApp::BuildPulse()
{
	// Remove this case for Deskbar add on API

	// If loading the replicant fails, launch the app instead
	// This allows having the replicant and the app open simultaneously
	if (prefs->window_mode == DESKBAR_MODE && LoadInDeskbar()) {
		PostMessage(new BMessage(B_QUIT_REQUESTED));
		return;
	} else
		prefs->window_mode = NORMAL_WINDOW_MODE;

	PulseWindow *pulseWindow = NULL;

	if (prefs->window_mode == MINI_WINDOW_MODE)
		pulseWindow = new PulseWindow(prefs->mini_window_rect);
	else
		pulseWindow = new PulseWindow(prefs->normal_window_rect);

	// check if the window is on screen, and move it if not
	BRect frame = pulseWindow->Frame();
	BRect screenFrame = BScreen().Frame();

	if (frame.left > screenFrame.right)
		pulseWindow->MoveBy(screenFrame.right - frame.right - 10, 0);
	else if (frame.right < 0)
		pulseWindow->MoveTo(10, frame.top);

	if (frame.top > screenFrame.bottom)
		pulseWindow->MoveBy(0, screenFrame.bottom - frame.bottom - 10);
	else if (frame.bottom < 0)
		pulseWindow->MoveTo(frame.left, 10);

	pulseWindow->Show();
}


PulseApp::~PulseApp()
{
	// Load the replicant after we save our preferences so they don't
	// get overwritten by DeskbarPulseView's instance
	prefs->Save();
	if (prefs->window_mode == DESKBAR_MODE)
		LoadInDeskbar();

	delete prefs;
}


//	#pragma mark -


/** Make sure we don't disable the last CPU - this is needed by
 *	descendants of PulseView for the popup menu and for CPUButton
 *	both as a replicant and not.
 */

bool
LastEnabledCPU(int my_cpu)
{
	system_info sys_info;
	get_system_info(&sys_info);
	if (sys_info.cpu_count == 1)
		return true;

	for (int x = 0; x < sys_info.cpu_count; x++) {
		if (x == my_cpu)
			continue;
		if (_kget_cpu_state_(x) == 1)
			return false;
	}
	return true;
}


/**	Ensure that the mini mode and deskbar mode always show an indicator
 *	for each CPU, at least one pixel wide.
 */

int
GetMinimumViewWidth()
{
	system_info sys_info;
	get_system_info(&sys_info);
	return (sys_info.cpu_count * 2) + 1;
}


void
Usage()
{
	printf("Usage: Pulse [--mini] [-w width] [--width=width]\n"
	       "\t[--deskbar] [--normal] [--framecolor 0xrrggbb]\n"
	       "\t[--activecolor 0xrrggbb] [--idlecolor 0xrrggbb]\n");
	exit(0);
}


bool
LoadInDeskbar()
{
	PulseApp *pulseapp = (PulseApp *)be_app;
	BDeskbar *deskbar = new BDeskbar();
	// Don't allow two copies in the Deskbar at once
	if (deskbar->HasItem("DeskbarPulseView")) {
		delete deskbar;
		return false;
	}
	
	// Must be 16 pixels high, the width is retrieved from the Prefs class
	int width = pulseapp->prefs->deskbar_icon_width;
	int min_width = GetMinimumViewWidth();
	if (width < min_width) {
		pulseapp->prefs->deskbar_icon_width = min_width;
		width = min_width;
	}
			
	BRect rect(0, 0, width - 1, 15);
	DeskbarPulseView *replicant = new DeskbarPulseView(rect);
	status_t err = deskbar->AddItem(replicant);
	delete replicant;
	delete deskbar;
	if (err != B_OK) {
		BAlert *alert = new BAlert(NULL, strerror(err), "OK");
		alert->Go(NULL);
		return false;
	} else return true;
}


int
main(int argc, char **argv)
{
	PulseApp *pulseapp = new PulseApp(argc, argv);
	pulseapp->Run();
	delete pulseapp;
	return 0;
}
