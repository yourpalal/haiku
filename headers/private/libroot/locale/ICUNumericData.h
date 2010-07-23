/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_NUMERIC_DATA_H
#define _ICU_NUMERIC_DATA_H


#include "ICULocaleconvData.h"
#include "LocaleBackend.h"


namespace BPrivate {


class ICUNumericData : public ICULocaleconvData {
	typedef	ICULocaleconvData	inherited;

public:
								ICUNumericData(struct lconv& localeConv);

			void				Initialize(LocaleNumericDataBridge* dataBridge);

	virtual	status_t			SetTo(const Locale& locale,
									const char* posixLocaleName);
	virtual	status_t			SetToPosix();

	virtual	const char*			GetLanginfo(int index);

private:
			char				fDecimalPoint[skLCBufSize];
			char				fThousandsSep[skLCBufSize];
			char				fGrouping[skLCBufSize];

			struct lconv&		fLocaleConv;
			LocaleNumericDataBridge* fDataBridge;
};


}	// namespace BPrivate


#endif	// _ICU_NUMERIC_DATA_H
