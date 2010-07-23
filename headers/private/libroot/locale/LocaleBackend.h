/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALE_BACKEND_H
#define _LOCALE_BACKEND_H


#include <SupportDefs.h>

#include <wctype.h>


struct lconv;
struct lc_time_t;


namespace BPrivate {


struct LocaleCtypeDataBridge {
	const unsigned short**	addrOfClassInfoTable;
	const int**				addrOfToLowerTable;
	const int**				addrOfToUpperTable;

	const unsigned short*	posixClassInfo;
	const int*				posixToLowerMap;
	const int*				posixToUpperMap;

	LocaleCtypeDataBridge();
};


struct LocaleMessagesDataBridge {
	const char** posixLanginfo;

	LocaleMessagesDataBridge();
};


struct LocaleMonetaryDataBridge {
	const struct lconv* posixLocaleConv;

	LocaleMonetaryDataBridge();
};


struct LocaleNumericDataBridge {
	const char** 		addrOfGlibcDecimalPoint;
	const char** 		addrOfGlibcThousandsSep;
	const char** 		addrOfGlibcGrouping;
	uint32_t*	 		addrOfGlibcWCDecimalPoint;
	uint32_t*	 		addrOfGlibcWCThousandsSep;
	const struct lconv* posixLocaleConv;

	LocaleNumericDataBridge();
};


struct LocaleTimeDataBridge {
	const struct lc_time_t* posixLCTimeInfo;

	LocaleTimeDataBridge();
};


struct LocaleDataBridge {
	LocaleCtypeDataBridge		ctypeDataBridge;
	LocaleMessagesDataBridge	messagesDataBridge;
	LocaleMonetaryDataBridge	monetaryDataBridge;
	LocaleNumericDataBridge		numericDataBridge;
	LocaleTimeDataBridge		timeDataBridge;
	const char**				posixLanginfo;

	LocaleDataBridge();
};


class LocaleBackend {
public:
								LocaleBackend();
	virtual						~LocaleBackend();

	virtual	const char*			SetLocale(int category, const char* locale) = 0;
	virtual	const struct lconv*	LocaleConv() = 0;
	virtual	const struct lc_time_t*	LCTimeInfo() = 0;

	virtual	int					IsWCType(wint_t wc, wctype_t charClass) = 0;
	virtual	status_t			ToWCTrans(wint_t wc, wctrans_t transition,
									wint_t& result) = 0;

	virtual	const char*			GetLanginfo(int index) = 0;

	virtual	status_t			Strcoll(const char* a, const char* b,
									int& out) = 0;
	virtual status_t			Strxfrm(char* out, const char* in, size_t size,
									size_t& outSize) = 0;

	virtual void				Initialize(LocaleDataBridge* dataBridge) = 0;

	static	status_t			LoadBackend();
};


extern LocaleBackend* gLocaleBackend;


}	// namespace BPrivate


#endif	// _LOCALE_BACKEND_H
