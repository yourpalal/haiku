/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_MESSAGES_DATA_H
#define _ICU_MESSAGES_DATA_H


#include "ICUCategoryData.h"
#include "LocaleBackend.h"


namespace BPrivate {


class ICUMessagesData : public ICUCategoryData {
	typedef	ICUCategoryData		inherited;
public:
	virtual	status_t			SetTo(const Locale& locale,
									const char* posixLocaleName);
	virtual	status_t			SetToPosix();

			void				Initialize(
									LocaleMessagesDataBridge* dataBridge);

			const char*			GetLanginfo(int index);

private:
			char				fYesExpression[80];
			char				fNoExpression[80];

			const char**		fPosixLanginfo;
};


}	// namespace BPrivate


#endif	// _ICU_MESSAGES_DATA_H
