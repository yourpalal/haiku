/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>

#include <dev/iwn/if_iwnreg.h>
#include <dev/iwn/if_iwnvar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(iprowifi4965, iwn, pci)
NO_HAIKU_FBSD_MII_DRIVER();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES|FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(44417);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct iwn_softc* sc = (struct iwn_softc*)device_get_softc(dev);
	uint32 r1, r2;
	HAIKU_INTR_REGISTER_STATE;

	HAIKU_INTR_REGISTER_ENTER();
	r1 = IWN_READ(sc, IWN_INTR);
	r2 = IWN_READ(sc, IWN_INTR_STATUS);

	if (r1 == 0 && r2 == 0) {
		/* not for us */
		IWN_WRITE(sc, IWN_MASK, IWN_INTR_MASK);
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if (r1 == 0xffffffff) {
		/* hardware gone */
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	/* disable interrupts */
	IWN_WRITE(sc, IWN_MASK, 0);

	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct iwn_softc* sc = (struct iwn_softc*)device_get_softc(dev);

	/* enable interrupts */
	IWN_WRITE(sc, IWN_MASK, IWN_INTR_MASK);
}
