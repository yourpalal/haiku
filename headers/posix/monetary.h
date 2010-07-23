/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MONETARY_H_
#define _MONETARY_H_


#include <locale.h>
#include <stddef.h>
#include <sys/cdefs.h>
#include <sys/types.h>


__BEGIN_DECLS

extern ssize_t strfmon(char* s, size_t maxsize, const char* format, ...);

__END_DECLS


#endif /* _MONETARY_H_ */
