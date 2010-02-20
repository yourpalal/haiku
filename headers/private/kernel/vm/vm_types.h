/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_TYPES_H
#define _KERNEL_VM_VM_TYPES_H


#include <arch/vm_types.h>
#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/DoublyLinkedQueue.h>
#include <util/SplayTree.h>

#include <sys/uio.h>

#include "kernel_debug_config.h"


class AsyncIOCallback;
struct vm_page_mapping;
struct VMCache;
struct VMCacheRef;
typedef DoublyLinkedListLink<vm_page_mapping> vm_page_mapping_link;


typedef struct vm_page_mapping {
	vm_page_mapping_link page_link;
	vm_page_mapping_link area_link;
	struct vm_page *page;
	struct VMArea *area;
} vm_page_mapping;

class DoublyLinkedPageLink {
	public:
		inline vm_page_mapping_link *operator()(vm_page_mapping *element) const
		{
			return &element->page_link;
		}

		inline const vm_page_mapping_link *operator()(
			const vm_page_mapping *element) const
		{
			return &element->page_link;
		}
};

class DoublyLinkedAreaLink {
	public:
		inline vm_page_mapping_link *operator()(vm_page_mapping *element) const
		{
			return &element->area_link;
		}

		inline const vm_page_mapping_link *operator()(
			const vm_page_mapping *element) const
		{
			return &element->area_link;
		}
};

typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedPageLink>
	vm_page_mappings;
typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedAreaLink>
	VMAreaMappings;

typedef uint32 page_num_t;


struct VMCacheRef {
			VMCache*			cache;
			int32				ref_count;

								VMCacheRef(VMCache* cache);
};


struct vm_page {
	DoublyLinkedListLink<vm_page> queue_link;

	addr_t					physical_page_number;

private:
	VMCacheRef*				cache_ref;
public:
	page_num_t				cache_offset;
								// in page size units

	SplayTreeLink<vm_page>	cache_link;
	vm_page*				cache_next;

	vm_page_mappings		mappings;

#if DEBUG_PAGE_QUEUE
	void*					queue;
#endif

#if DEBUG_PAGE_ACCESS
	vint32					accessing_thread;
#endif

	uint8					state : 3;
	bool					busy : 1;
	bool					busy_writing : 1;
		// used in VMAnonymousCache::Merge()
	bool					accessed : 1;
	bool					modified : 1;
	uint8					unused : 1;

	uint8					usage_count;
	uint16					wired_count;

	VMCacheRef* CacheRef() const			{ return cache_ref; }
	void SetCacheRef(VMCacheRef* cacheRef)	{ this->cache_ref = cacheRef; }

	VMCache* Cache() const
		{ return cache_ref != NULL ? cache_ref->cache : NULL; }

	bool IsMapped() const
		{ return wired_count > 0 || !mappings.IsEmpty(); }
};


enum {
	PAGE_STATE_ACTIVE = 0,
	PAGE_STATE_INACTIVE,
	PAGE_STATE_MODIFIED,
	PAGE_STATE_CACHED,
	PAGE_STATE_FREE,
	PAGE_STATE_CLEAR,
	PAGE_STATE_WIRED,
	PAGE_STATE_UNUSED,

	PAGE_STATE_COUNT,

	PAGE_STATE_FIRST_UNQUEUED = PAGE_STATE_WIRED
};


#define VM_PAGE_ALLOC_STATE	0x00000007
#define VM_PAGE_ALLOC_CLEAR	0x00000010
#define VM_PAGE_ALLOC_BUSY	0x00000020


#if DEBUG_PAGE_ACCESS
#	include <thread.h>

static inline void
vm_page_debug_access_start(vm_page* page)
{
	thread_id threadID = thread_get_current_thread_id();
	thread_id previousThread = atomic_test_and_set(&page->accessing_thread,
		threadID, -1);
	if (previousThread != -1) {
		panic("Invalid concurrent access to page %p (start), currently "
			"accessed by: %" B_PRId32, page, previousThread);
	}
}


static inline void
vm_page_debug_access_end(vm_page* page)
{
	thread_id threadID = thread_get_current_thread_id();
	thread_id previousThread = atomic_test_and_set(&page->accessing_thread, -1,
		threadID);
	if (previousThread != threadID) {
		panic("Invalid concurrent access to page %p (end) by current thread, "
			"current accessor is: %" B_PRId32, page, previousThread);
	}
}


static inline void
vm_page_debug_access_check(vm_page* page)
{
	thread_id thread = page->accessing_thread;
	if (thread != thread_get_current_thread_id()) {
		panic("Invalid concurrent access to page %p (check), currently "
			"accessed by: %" B_PRId32, page, thread);
	}
}


static inline void
vm_page_debug_access_transfer(vm_page* page, thread_id expectedPreviousThread)
{
	thread_id threadID = thread_get_current_thread_id();
	thread_id previousThread = atomic_test_and_set(&page->accessing_thread,
		threadID, expectedPreviousThread);
	if (previousThread != expectedPreviousThread) {
		panic("Invalid access transfer for page %p, currently accessed by: "
			"%" B_PRId32 ", expected: %" B_PRId32, page, previousThread,
			expectedPreviousThread);
	}
}

#	define DEBUG_PAGE_ACCESS_START(page)	vm_page_debug_access_start(page)
#	define DEBUG_PAGE_ACCESS_END(page)		vm_page_debug_access_end(page)
#	define DEBUG_PAGE_ACCESS_CHECK(page)	vm_page_debug_access_check(page)
#	define DEBUG_PAGE_ACCESS_TRANSFER(page, thread)	\
		vm_page_debug_access_transfer(page, thread)
#else
#	define DEBUG_PAGE_ACCESS_START(page)			do {} while (false)
#	define DEBUG_PAGE_ACCESS_END(page)				do {} while (false)
#	define DEBUG_PAGE_ACCESS_CHECK(page)			do {} while (false)
#	define DEBUG_PAGE_ACCESS_TRANSFER(page, thread)	do {} while (false)
#endif


#endif	// _KERNEL_VM_VM_TYPES_H
