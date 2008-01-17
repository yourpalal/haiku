/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*! Semaphore code */


#include <OS.h>

#include <sem.h>
#include <kernel.h>
#include <kscheduler.h>
#include <ksignal.h>
#include <smp.h>
#include <int.h>
#include <arch/int.h>
#include <debug.h>
#include <thread.h>
#include <team.h>
#include <vfs.h>
#include <vm_low_memory.h>
#include <vm_page.h>
#include <boot/kernel_args.h>
#include <wait_for_objects.h>

#include <string.h>
#include <stdlib.h>


//#define TRACE_SEM
#ifdef TRACE_SEM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define DEBUG_LAST_ACQUIRER

struct sem_entry {
	sem_id		id;
	spinlock	lock;	// protects only the id field when unused
	union {
		// when slot in use
		struct {
			int32				count;
			struct thread_queue queue;
			char				*name;
			team_id				owner;	// if set to -1, means owned by a port
			select_info			*select_infos;
#ifdef DEBUG_LAST_ACQUIRER
			thread_id			last_acquirer;
			int32				last_acquire_count;
			thread_id			last_releaser;
			int32				last_release_count;
#endif
		} used;

		// when slot unused
		struct {
			sem_id				next_id;
			struct sem_entry	*next;
		} unused;
	} u;
};

static const int32 kMaxSemaphores = 131072;
static int32 sMaxSems = 4096;
	// Final value is computed based on the amount of available memory
static int32 sUsedSems = 0;

static struct sem_entry *sSems = NULL;
static bool sSemsActive = false;
static struct sem_entry	*sFreeSemsHead = NULL;
static struct sem_entry	*sFreeSemsTail = NULL;

static spinlock sem_spinlock = 0;
#define GRAB_SEM_LIST_LOCK()     acquire_spinlock(&sem_spinlock)
#define RELEASE_SEM_LIST_LOCK()  release_spinlock(&sem_spinlock)
#define GRAB_SEM_LOCK(s)         acquire_spinlock(&(s).lock)
#define RELEASE_SEM_LOCK(s)      release_spinlock(&(s).lock)

static int remove_thread_from_sem(struct thread *thread, struct sem_entry *sem,
	struct thread_queue *queue, status_t acquireStatus, bool hasThreadLock);

struct sem_timeout_args {
	thread_id	blocked_thread;
	sem_id		blocked_sem_id;
	int32		sem_count;
};


static int
dump_sem_list(int argc, char **argv)
{
	const char *name = NULL;
	team_id owner = -1;
#ifdef DEBUG_LAST_ACQUIRER
	thread_id last = -1;
#endif
	int32 i;

	if (argc > 2) {
		if (!strcmp(argv[1], "team") || !strcmp(argv[1], "owner"))
			owner = strtoul(argv[2], NULL, 0);
		else if (!strcmp(argv[1], "name"))
			name = argv[2];
#ifdef DEBUG_LAST_ACQUIRER
		else if (!strcmp(argv[1], "last"))
			last = strtoul(argv[2], NULL, 0);
#endif
	} else if (argc > 1)
		owner = strtoul(argv[1], NULL, 0);

	kprintf("sem            id count   team"
#ifdef DEBUG_LAST_ACQUIRER
		"   last"
#endif
		"  name\n");

	for (i = 0; i < sMaxSems; i++) {
		struct sem_entry *sem = &sSems[i];
		if (sem->id < 0
#ifdef DEBUG_LAST_ACQUIRER
			|| (last != -1 && sem->u.used.last_acquirer != last)
#endif
			|| (name != NULL && strstr(sem->u.used.name, name) == NULL)
			|| (owner != -1 && sem->u.used.owner != owner))
			continue;

		kprintf("%p %6ld %5ld %6ld "
#ifdef DEBUG_LAST_ACQUIRER
			"%6ld "
#endif
			" %s\n", sem, sem->id, sem->u.used.count,
			sem->u.used.owner,
#ifdef DEBUG_LAST_ACQUIRER
			sem->u.used.last_acquirer > 0 ? sem->u.used.last_acquirer : 0,
#endif
			sem->u.used.name);
	}

	return 0;
}


static void
dump_sem(struct sem_entry *sem)
{
	kprintf("SEM: %p\n", sem);
	kprintf("id:      %ld (%#lx)\n", sem->id, sem->id);
	if (sem->id >= 0) {
		kprintf("name:    '%s'\n", sem->u.used.name);
		kprintf("owner:   %ld\n", sem->u.used.owner);
		kprintf("count:   %ld\n", sem->u.used.count);
		kprintf("queue:  ");
		if (sem->u.used.queue.head != NULL) {
			struct thread *thread = sem->u.used.queue.head;
			while (thread != NULL) {
				kprintf(" %ld", thread->id);
				thread = thread->queue_next;
			}
			kprintf("\n");
		} else
			kprintf(" -\n");
#ifdef DEBUG_LAST_ACQUIRER
		kprintf("last acquired by: %ld, count: %ld\n", sem->u.used.last_acquirer,
			sem->u.used.last_acquire_count);
		kprintf("last released by: %ld, count: %ld\n", sem->u.used.last_releaser,
			sem->u.used.last_release_count);
#endif
	} else {
		kprintf("next:    %p\n", sem->u.unused.next);
		kprintf("next_id: %ld\n", sem->u.unused.next_id);
	}
}


static int
dump_sem_info(int argc, char **argv)
{
	bool found = false;
	addr_t num;
	int32 i;

	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	num = strtoul(argv[1], NULL, 0);

	if (IS_KERNEL_ADDRESS(num)) {
		dump_sem((struct sem_entry *)num);
		return 0;
	} else if (num > 0) {
		uint32 slot = num % sMaxSems;
		if (sSems[slot].id != (int)num) {
			kprintf("sem 0x%lx (%ld) doesn't exist!\n", num, num);
			return 0;
		}

		dump_sem(&sSems[slot]);
		return 0;
	}

	// walk through the sem list, trying to match name
	for (i = 0; i < sMaxSems; i++) {
		if (sSems[i].u.used.name != NULL
			&& strcmp(argv[1], sSems[i].u.used.name) == 0) {
			dump_sem(&sSems[i]);
			found = true;
		}
	}

	if (!found)
		kprintf("sem \"%s\" doesn't exist!\n", argv[1]);
	return 0;
}


static inline void
clear_thread_queue(struct thread_queue *queue)
{
	queue->head = queue->tail = NULL;
}


/*!	\brief Appends a semaphore slot to the free list.

	The semaphore list must be locked.
	The slot's id field is not changed. It should already be set to -1.

	\param slot The index of the semaphore slot.
	\param nextID The ID the slot will get when reused. If < 0 the \a slot
		   is used.
*/
static void
free_sem_slot(int slot, sem_id nextID)
{
	struct sem_entry *sem = sSems + slot;
	// set next_id to the next possible value; for sanity check the current ID
	if (nextID < 0)
		sem->u.unused.next_id = slot;
	else
		sem->u.unused.next_id = nextID;
	// append the entry to the list
	if (sFreeSemsTail)
		sFreeSemsTail->u.unused.next = sem;
	else
		sFreeSemsHead = sem;
	sFreeSemsTail = sem;
	sem->u.unused.next = NULL;
}


static inline void
notify_sem_select_events(struct sem_entry* sem, uint16 events)
{
	if (sem->u.used.select_infos)
		notify_select_events_list(sem->u.used.select_infos, events);
}


/*! Called from a timer handler. Wakes up a semaphore */
static int32
sem_timeout(timer *data)
{
	struct sem_timeout_args *args = (struct sem_timeout_args *)data->entry.prev;
	struct thread *thread;
	int slot;
	int state;
	struct thread_queue wakeupQueue;

	thread = thread_get_thread_struct(args->blocked_thread);
	if (thread == NULL)
		return B_HANDLED_INTERRUPT;
	slot = args->blocked_sem_id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	TRACE(("sem_timeout: called on %p sem %ld, thread %ld\n",
		data, args->blocked_sem_id, args->blocked_thread));

	if (sSems[slot].id != args->blocked_sem_id) {
		// this thread was not waiting on this semaphore
		panic("sem_timeout: thread %ld was trying to wait on sem %ld which "
			"doesn't exist!\n", args->blocked_thread, args->blocked_sem_id);

		RELEASE_SEM_LOCK(sSems[slot]);
		restore_interrupts(state);
		return B_HANDLED_INTERRUPT;
	}

	clear_thread_queue(&wakeupQueue);
	remove_thread_from_sem(thread, &sSems[slot], &wakeupQueue, B_TIMED_OUT,
		false);

	RELEASE_SEM_LOCK(sSems[slot]);

	GRAB_THREAD_LOCK();
	// put the threads in the run q here to make sure we dont deadlock in sem_interrupt_thread
	while ((thread = thread_dequeue(&wakeupQueue)) != NULL) {
		scheduler_enqueue_in_run_queue(thread);
	}
	RELEASE_THREAD_LOCK();

	restore_interrupts(state);

	return B_INVOKE_SCHEDULER;
}


/*!	Fills the thread_info structure with information from the specified
	thread.
	The thread lock must be held when called.
*/
static void
fill_sem_info(struct sem_entry *sem, sem_info *info, size_t size)
{
	info->sem = sem->id;
	info->team = sem->u.used.owner;
	strlcpy(info->name, sem->u.used.name, sizeof(info->name));
	info->count = sem->u.used.count;

	// ToDo: not sure if this is the latest holder, or the next
	// holder...
	if (sem->u.used.queue.head != NULL)
		info->latest_holder = sem->u.used.queue.head->id;
	else
		info->latest_holder = -1;
}


//	#pragma mark - Private Kernel API


status_t
sem_init(kernel_args *args)
{
	area_id area;
	int32 i;

	TRACE(("sem_init: entry\n"));

	// compute maximal number of semaphores depending on the available memory
	// 128 MB -> 16384 semaphores, 448 kB fixed array size
	// 256 MB -> 32768, 896 kB
	// 512 MB -> 65536, 1.75 MB
	// 1024 MB and more -> 131072, 3.5 MB
	i = vm_page_num_pages() / 2;
	while (sMaxSems < i && sMaxSems < kMaxSemaphores)
		sMaxSems <<= 1;

	// create and initialize semaphore table
	area = create_area("sem_table", (void **)&sSems, B_ANY_KERNEL_ADDRESS,
		sizeof(struct sem_entry) * sMaxSems, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < 0)
		panic("unable to allocate semaphore table!\n");

	memset(sSems, 0, sizeof(struct sem_entry) * sMaxSems);
	for (i = 0; i < sMaxSems; i++) {
		sSems[i].id = -1;
		free_sem_slot(i, i);
	}

	// add debugger commands
	add_debugger_command_etc("sems", &dump_sem_list,
		"Dump a list of all active semaphores (for team, with name, etc.)",
		"[ ([ \"team\" | \"owner\" ] <team>) | (\"name\" <name>) ]"
#ifdef DEBUG_LAST_ACQUIRER
			" | (\"last\" <last acquirer>)"
#endif
		"\n"
		"Prints a list of all active semaphores meeting the given\n"
		"requirement. If no argument is given, all sems are listed.\n"
		"  <team>             - The team owning the semaphores.\n"
		"  <name>             - Part of the name of the semaphores.\n"
#ifdef DEBUG_LAST_ACQUIRER
		"  <last acquirer>    - The thread that last acquired the semaphore.\n"
#endif
		, 0);
	add_debugger_command_etc("sem", &dump_sem_info,
		"Dump info about a particular semaphore",
		"<sem>\n"
		"Prints info about the specified semaphore.\n"
		"  <sem>  - pointer to the semaphore structure, semaphore ID, or name\n"
		"           of the semaphore to print info for.\n", 0);

	TRACE(("sem_init: exit\n"));

	sSemsActive = true;

	return 0;
}


/*!	Creates a semaphore with the given parameters.
	Note, the team_id is not checked, it must be correct, or else
	that semaphore might not be deleted.
	This function is only available from within the kernel, and
	should not be made public - if possible, we should remove it
	completely (and have only create_sem() exported).
*/
sem_id
create_sem_etc(int32 count, const char *name, team_id owner)
{
	struct sem_entry *sem = NULL;
	cpu_status state;
	sem_id id = B_NO_MORE_SEMS;
	char *tempName;
	size_t nameLength;

	if (sSemsActive == false)
		return B_NO_MORE_SEMS;

#if 0
	// TODO: the code below might cause unwanted deadlocks,
	// we need an asynchronously running low resource handler.
	if (sUsedSems == sMaxSems) {
		// The vnode cache may have collected lots of semaphores.
		// Freeing some unused vnodes should improve our situation.
		// TODO: maybe create a generic "low resources" handler, instead
		//	of only the specialised low memory thing?
		vfs_free_unused_vnodes(B_LOW_MEMORY_WARNING);
	}
	if (sUsedSems == sMaxSems) {
		// try again with more enthusiasm
		vfs_free_unused_vnodes(B_LOW_MEMORY_CRITICAL);
	}
#endif
	if (sUsedSems == sMaxSems)
		return B_NO_MORE_SEMS;

	if (name == NULL)
		name = "unnamed semaphore";

	nameLength = strlen(name) + 1;
	nameLength = min_c(nameLength, B_OS_NAME_LENGTH);
	tempName = (char *)malloc(nameLength);
	if (tempName == NULL)
		return B_NO_MEMORY;
	strlcpy(tempName, name, nameLength);

	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	// get the first slot from the free list
	sem = sFreeSemsHead;
	if (sem) {
		// remove it from the free list
		sFreeSemsHead = sem->u.unused.next;
		if (!sFreeSemsHead)
			sFreeSemsTail = NULL;

		// init the slot
		GRAB_SEM_LOCK(*sem);
		sem->id = sem->u.unused.next_id;
		sem->u.used.count = count;
		clear_thread_queue(&sem->u.used.queue);
		sem->u.used.name = tempName;
		sem->u.used.owner = owner;
		sem->u.used.select_infos = NULL;
		id = sem->id;
		RELEASE_SEM_LOCK(*sem);

		atomic_add(&sUsedSems, 1);
	}

	RELEASE_SEM_LIST_LOCK();
	restore_interrupts(state);

	if (!sem)
		free(tempName);

	return id;
}


status_t
select_sem(int32 id, struct select_info* info, bool kernel)
{
	cpu_status state;
	int32 slot;
	status_t error = B_OK;

	if (id < 0)
		return B_BAD_SEM_ID;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != id) {
		// bad sem ID
		error = B_BAD_SEM_ID;
	} else if (!kernel
		&& sSems[slot].u.used.owner == team_get_kernel_team_id()) {
		// kernel semaphore, but call from userland
		error = B_NOT_ALLOWED;
	} else {
		info->selected_events &= B_EVENT_ACQUIRE_SEMAPHORE | B_EVENT_INVALID;

		if (info->selected_events != 0) {
			info->next = sSems[slot].u.used.select_infos;
			sSems[slot].u.used.select_infos = info;

			if (sSems[slot].u.used.count > 0)
				notify_select_events(info, B_EVENT_ACQUIRE_SEMAPHORE);
		}
	}

	RELEASE_SEM_LOCK(sSems[slot]);
	restore_interrupts(state);

	return error;
}


status_t
deselect_sem(int32 id, struct select_info* info, bool kernel)
{
	cpu_status state;
	int32 slot;

	if (id < 0)
		return B_BAD_SEM_ID;

	if (info->selected_events == 0)
		return B_OK;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id == id) {
		select_info** infoLocation = &sSems[slot].u.used.select_infos;
		while (*infoLocation != NULL && *infoLocation != info)
			infoLocation = &(*infoLocation)->next;

		if (*infoLocation == info)
			*infoLocation = info->next;
	}

	RELEASE_SEM_LOCK(sSems[slot]);
	restore_interrupts(state);

	return B_OK;
}


/*! Wake up a thread that's blocked on a semaphore
	this function must be entered with interrupts disabled and THREADLOCK held
*/
status_t
sem_interrupt_thread(struct thread *thread)
{
	struct thread_queue wakeupQueue;
	int32 slot;

	TRACE(("sem_interrupt_thread: called on thread %p (%ld), blocked on sem %ld\n",
		thread, thread->id, thread->sem.blocking));

	if (thread->state != B_THREAD_WAITING || thread->sem.blocking < 0)
		return B_BAD_VALUE;
	if ((thread->sem.flags & B_CAN_INTERRUPT) == 0
		&& ((thread->sem.flags & B_KILL_CAN_INTERRUPT) == 0
			|| (thread->sig_pending & KILL_SIGNALS) == 0)) {
		return B_NOT_ALLOWED;
	}

	slot = thread->sem.blocking % sMaxSems;

	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != thread->sem.blocking) {
		panic("sem_interrupt_thread: thread %ld blocks on sem %ld, but that "
			"sem doesn't exist!\n", thread->id, thread->sem.blocking);
	}

	clear_thread_queue(&wakeupQueue);
	if (remove_thread_from_sem(thread, &sSems[slot], &wakeupQueue,
			B_INTERRUPTED, true) != B_OK) {
		panic("sem_interrupt_thread: thread %ld not found in sem %ld's wait "
			"queue\n", thread->id, thread->sem.blocking);
	}

	RELEASE_SEM_LOCK(sSems[slot]);

	while ((thread = thread_dequeue(&wakeupQueue)) != NULL) {
		scheduler_enqueue_in_run_queue(thread);
	}

	return B_NO_ERROR;
}


/*!	Forcibly removes a thread from a semaphores wait queue. May have to wake up
	other threads in the process. All threads that need to be woken up are added
	to the passed in thread_queue.
	Must be called with semaphore lock held.
*/
static int
remove_thread_from_sem(struct thread *thread, struct sem_entry *sem,
	struct thread_queue *queue, status_t acquireStatus, bool hasThreadLock)
{
	// remove the thread from the queue and place it in the supplied queue
	if (thread_dequeue_id(&sem->u.used.queue, thread->id) != thread)
		return B_ENTRY_NOT_FOUND;

	sem->u.used.count += thread->sem.acquire_count;
	thread->state = thread->next_state = B_THREAD_READY;
	thread->sem.acquire_status = acquireStatus;
	thread_enqueue(thread, queue);

	// now see if more threads need to be woken up
	while (sem->u.used.count > 0
		   && thread_lookat_queue(&sem->u.used.queue) != NULL) {
		int32 delta = min_c(thread->sem.count, sem->u.used.count);

		thread->sem.count -= delta;
		if (thread->sem.count <= 0) {
			thread = thread_dequeue(&sem->u.used.queue);
			thread->state = thread->next_state = B_THREAD_READY;
			thread_enqueue(thread, queue);
		}
		sem->u.used.count -= delta;
	}

	if (sem->u.used.count > 0 && sem->u.used.select_infos != NULL) {
		if (hasThreadLock)
			RELEASE_THREAD_LOCK();

		notify_sem_select_events(sem, B_EVENT_ACQUIRE_SEMAPHORE);

		if (hasThreadLock)
			GRAB_THREAD_LOCK();
	}

	return B_OK;
}


/*!	This function cycles through the sem table, deleting all the sems
	that are owned by the specified team.
*/
int
sem_delete_owned_sems(team_id owner)
{
	int state;
	int i;
	int count = 0;

	// ToDo: that looks horribly inefficient - maybe it would be better
	//	to have them in a list in the team

	if (owner < 0)
		return B_BAD_TEAM_ID;

	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	for (i = 0; i < sMaxSems; i++) {
		if (sSems[i].id != -1 && sSems[i].u.used.owner == owner) {
			sem_id id = sSems[i].id;

			RELEASE_SEM_LIST_LOCK();
			restore_interrupts(state);

			delete_sem(id);
			count++;

			state = disable_interrupts();
			GRAB_SEM_LIST_LOCK();
		}
	}

	RELEASE_SEM_LIST_LOCK();
	restore_interrupts(state);

	return count;
}


int32
sem_max_sems(void)
{
	return sMaxSems;
}


int32
sem_used_sems(void)
{
	return sUsedSems;
}


//	#pragma mark - Public Kernel API


sem_id
create_sem(int32 count, const char *name)
{
	return create_sem_etc(count, name, team_get_kernel_team_id());
}


status_t
delete_sem(sem_id id)
{
	struct thread_queue releaseQueue;
	int32 releasedThreads;
	struct thread *thread;
	cpu_status state;
	int32 slot;
	char *name;

	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != id) {
		RELEASE_SEM_LOCK(sSems[slot]);
		restore_interrupts(state);
		TRACE(("delete_sem: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	notify_sem_select_events(&sSems[slot], B_EVENT_INVALID);
	sSems[slot].u.used.select_infos = NULL;

	releasedThreads = 0;
	clear_thread_queue(&releaseQueue);

	// free any threads waiting for this semaphore
	while ((thread = thread_dequeue(&sSems[slot].u.used.queue)) != NULL) {
		thread->state = B_THREAD_READY;
		thread->sem.acquire_status = B_BAD_SEM_ID;
		thread->sem.count = 0;
		thread_enqueue(thread, &releaseQueue);
		releasedThreads++;
	}

	sSems[slot].id = -1;
	name = sSems[slot].u.used.name;
	sSems[slot].u.used.name = NULL;

	RELEASE_SEM_LOCK(sSems[slot]);

	// append slot to the free list
	GRAB_SEM_LIST_LOCK();
	free_sem_slot(slot, id + sMaxSems);
	atomic_add(&sUsedSems, -1);
	RELEASE_SEM_LIST_LOCK();

	if (releasedThreads > 0) {
		GRAB_THREAD_LOCK();
		while ((thread = thread_dequeue(&releaseQueue)) != NULL) {
			scheduler_enqueue_in_run_queue(thread);
		}
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();
	}

	restore_interrupts(state);

	free(name);

	return B_OK;
}


status_t
acquire_sem(sem_id id)
{
	return switch_sem_etc(-1, id, 1, 0, 0);
}


status_t
acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return switch_sem_etc(-1, id, count, flags, timeout);
}


status_t
switch_sem(sem_id toBeReleased, sem_id toBeAcquired)
{
	return switch_sem_etc(toBeReleased, toBeAcquired, 1, 0, 0);
}


status_t
switch_sem_etc(sem_id semToBeReleased, sem_id id, int32 count,
	uint32 flags, bigtime_t timeout)
{
	int slot = id % sMaxSems;
	int state;
	status_t status = B_OK;

	if (kernel_startup)
		return B_OK;
	if (sSemsActive == false)
		return B_NO_MORE_SEMS;

	if (!are_interrupts_enabled()) {
		panic("switch_sem_etc: called with interrupts disabled for sem %ld\n",
			id);
	}

	if (id < 0)
		return B_BAD_SEM_ID;
	if (count <= 0
		|| (flags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) == (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) {
		return B_BAD_VALUE;
	}

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != id) {
		TRACE(("switch_sem_etc: bad sem %ld\n", id));
		status = B_BAD_SEM_ID;
		goto err;
	}

	// ToDo: the B_CHECK_PERMISSION flag should be made private, as it
	//	doesn't have any use outside the kernel
	if ((flags & B_CHECK_PERMISSION) != 0
		&& sSems[slot].u.used.owner == team_get_kernel_team_id()) {
		dprintf("thread %ld tried to acquire kernel semaphore %ld.\n",
			thread_get_current_thread_id(), id);
		status = B_NOT_ALLOWED;
		goto err;
	}

	if (sSems[slot].u.used.count - count < 0 && (flags & B_RELATIVE_TIMEOUT) != 0
		&& timeout <= 0) {
		// immediate timeout
		status = B_WOULD_BLOCK;
		goto err;
	}

	if ((sSems[slot].u.used.count -= count) < 0) {
		// we need to block
		struct thread *thread = thread_get_current_thread();
		timer timeout_timer; // stick it on the stack, since we may be blocking here
		struct sem_timeout_args args;

		TRACE(("switch_sem_etc(id = %ld): block name = %s, thread = %p,"
			" name = %s\n", id, sSems[slot].u.used.name, thread, thread->name));

		// do a quick check to see if the thread has any pending signals
		// this should catch most of the cases where the thread had a signal
		if (((flags & B_CAN_INTERRUPT) && (thread->sig_pending & ~thread->sig_block_mask) != 0)
			|| ((flags & B_KILL_CAN_INTERRUPT)
				&& (thread->sig_pending & KILL_SIGNALS))) {
			sSems[slot].u.used.count += count;
			status = B_INTERRUPTED;
			goto err;
		}

		if ((flags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) == 0)
			timeout = B_INFINITE_TIMEOUT;

		thread->next_state = B_THREAD_WAITING;
		thread->sem.flags = flags;
		thread->sem.blocking = id;
		thread->sem.acquire_count = count;
		thread->sem.count = min_c(-sSems[slot].u.used.count, count);
			// store the count we need to restore upon release
		thread->sem.acquire_status = B_NO_ERROR;
		thread_enqueue(thread, &sSems[slot].u.used.queue);

		if (timeout != B_INFINITE_TIMEOUT) {
			TRACE(("switch_sem_etc: setting timeout sem for %Ld usecs, sem %ld, thread %ld\n",
				timeout, id, thread->id));

			// set up an event to go off with the thread struct as the data
			args.blocked_sem_id = id;
			args.blocked_thread = thread->id;
			args.sem_count = count;

			// ToDo: another evil hack: pass the args into timer->entry.prev
			timeout_timer.entry.prev = (qent *)&args;
			add_timer(&timeout_timer, &sem_timeout, timeout,
				flags & B_RELATIVE_TIMEOUT ?
					B_ONE_SHOT_RELATIVE_TIMER : B_ONE_SHOT_ABSOLUTE_TIMER);			
		}

		RELEASE_SEM_LOCK(sSems[slot]);

		if (semToBeReleased >= B_OK)
			release_sem_etc(semToBeReleased, 1, B_DO_NOT_RESCHEDULE);

		GRAB_THREAD_LOCK();
		// check again to see if a signal is pending.
		// it may have been delivered while setting up the sem, though it's pretty unlikely
		if (((flags & B_CAN_INTERRUPT)
				&& (thread->sig_pending & ~thread->sig_block_mask) != 0)
			|| ((flags & B_KILL_CAN_INTERRUPT)
				&& (thread->sig_pending & KILL_SIGNALS))) {
			struct thread_queue wakeupQueue;
			// ok, so a tiny race happened where a signal was delivered to this thread while
			// it was setting up the sem. We can only be sure a signal wasn't delivered
			// here, since the threadlock is held. The previous check would have found most
			// instances, but there was a race, so we have to handle it. It'll be more messy...
			clear_thread_queue(&wakeupQueue);
			GRAB_SEM_LOCK(sSems[slot]);
			if (sSems[slot].id == id) {
				remove_thread_from_sem(thread, &sSems[slot], &wakeupQueue,
					B_INTERRUPTED, true);
			}
			RELEASE_SEM_LOCK(sSems[slot]);
			while ((thread = thread_dequeue(&wakeupQueue)) != NULL) {
				scheduler_enqueue_in_run_queue(thread);
			}
			// fall through and reschedule since another thread with a higher priority may have been woken up
		}
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();

		if (timeout != B_INFINITE_TIMEOUT) {
			if (thread->sem.acquire_status != B_TIMED_OUT) {
				// cancel the timer event, the sem may have been deleted or interrupted
				// with the timer still active
				cancel_timer(&timeout_timer);
			}
		}

#ifdef DEBUG_LAST_ACQUIRER
		if (thread->sem.acquire_status >= B_OK) {
			sSems[slot].u.used.last_acquirer = thread_get_current_thread_id();
			sSems[slot].u.used.last_acquire_count = count;
		}
#endif

		restore_interrupts(state);

		TRACE(("switch_sem_etc(sem %ld): exit block name %s, "
			"thread %ld (%s)\n", id, sSems[slot].u.used.name, thread->id,
			thread->name));
		return thread->sem.acquire_status;
	} else {
#ifdef DEBUG_LAST_ACQUIRER
		sSems[slot].u.used.last_acquirer = thread_get_current_thread_id();
		sSems[slot].u.used.last_acquire_count = count;
#endif
	}

err:
	RELEASE_SEM_LOCK(sSems[slot]);
	restore_interrupts(state);

#if 0
	if (status == B_NOT_ALLOWED)
	_user_debugger("Thread tried to acquire kernel semaphore.");
#endif

	return status;
}


status_t
release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}


status_t
release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	struct thread_queue releaseQueue;
	int32 slot = id % sMaxSems;
	cpu_status state;
	status_t status = B_OK;

	if (kernel_startup)
		return B_OK;
	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (count <= 0 && (flags & B_RELEASE_ALL) == 0)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != id) {
		TRACE(("sem_release_etc: invalid sem_id %ld\n", id));
		status = B_BAD_SEM_ID;
		goto err;
	}

	// ToDo: the B_CHECK_PERMISSION flag should be made private, as it
	//	doesn't have any use outside the kernel
	if ((flags & B_CHECK_PERMISSION) != 0
		&& sSems[slot].u.used.owner == team_get_kernel_team_id()) {
		dprintf("thread %ld tried to release kernel semaphore.\n",
			thread_get_current_thread_id());
		status = B_NOT_ALLOWED;
		goto err;
	}

#ifdef DEBUG_LAST_ACQUIRER
	sSems[slot].u.used.last_acquirer = -sSems[slot].u.used.last_acquirer;
	sSems[slot].u.used.last_releaser = thread_get_current_thread_id();
	sSems[slot].u.used.last_release_count = count;
#endif

	// clear out a queue we will use to hold all of the threads that we will have to
	// put back into the run list. This is done so the thread lock wont be held
	// while this sems lock is held since the two locks are grabbed in the other
	// order in sem_interrupt_thread.
	clear_thread_queue(&releaseQueue);

	if (flags & B_RELEASE_ALL) {
		count = -sSems[slot].u.used.count;

		// is there anything to do for us at all?
		if (count == 0)
			goto err;
	}

	while (count > 0) {
		int delta = count;
		if (sSems[slot].u.used.count < 0) {
			struct thread *thread = thread_lookat_queue(&sSems[slot].u.used.queue);

			delta = min_c(count, thread->sem.count);
			thread->sem.count -= delta;
			if (thread->sem.count <= 0) {
				// release this thread
				thread = thread_dequeue(&sSems[slot].u.used.queue);
				thread_enqueue(thread, &releaseQueue);
				thread->state = B_THREAD_READY;
				thread->sem.count = 0;
			}
		} else if (flags & B_RELEASE_IF_WAITING_ONLY)
			break;

		sSems[slot].u.used.count += delta;
		count -= delta;
	}

	if (sSems[slot].u.used.count > 0)
		notify_sem_select_events(&sSems[slot], B_EVENT_ACQUIRE_SEMAPHORE);

	RELEASE_SEM_LOCK(sSems[slot]);

	// pull off any items in the release queue and put them in the run queue
	if (releaseQueue.head != NULL) {
		struct thread *thread;

		GRAB_THREAD_LOCK();
		while ((thread = thread_dequeue(&releaseQueue)) != NULL) {
#if 0
			// temporarily place thread in a run queue with a higher priority to boost it up
			thread->next_priority = thread->priority >= B_FIRST_REAL_TIME_PRIORITY ?
				thread->priority : thread->priority + 1;
#endif
			scheduler_enqueue_in_run_queue(thread);
		}
		if ((flags & B_DO_NOT_RESCHEDULE) == 0)
			scheduler_reschedule();

		RELEASE_THREAD_LOCK();
	}
	goto outnolock;

err:
	RELEASE_SEM_LOCK(sSems[slot]);
outnolock:
	restore_interrupts(state);

	return status;
}


status_t
get_sem_count(sem_id id, int32 *_count)
{
	int slot;
	int state;

	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (_count == NULL)
		return B_BAD_VALUE;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != id) {
		RELEASE_SEM_LOCK(sSems[slot]);
		restore_interrupts(state);
		TRACE(("sem_get_count: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	*_count = sSems[slot].u.used.count;

	RELEASE_SEM_LOCK(sSems[slot]);
	restore_interrupts(state);

	return B_OK;
}


/*!	Called by the get_sem_info() macro. */
status_t
_get_sem_info(sem_id id, struct sem_info *info, size_t size)
{
	status_t status = B_OK;
	int state;
	int slot;

	if (!sSemsActive)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (info == NULL || size != sizeof(sem_info))
		return B_BAD_VALUE;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != id) {
		status = B_BAD_SEM_ID;
		TRACE(("get_sem_info: invalid sem_id %ld\n", id));
	} else
		fill_sem_info(&sSems[slot], info, size);

	RELEASE_SEM_LOCK(sSems[slot]);
	restore_interrupts(state);

	return status;
}


/*!	Called by the get_next_sem_info() macro. */
status_t
_get_next_sem_info(team_id team, int32 *_cookie, struct sem_info *info,
	size_t size)
{
	int state;
	int slot;
	bool found = false;

	if (!sSemsActive)
		return B_NO_MORE_SEMS;
	if (_cookie == NULL || info == NULL || size != sizeof(sem_info))
		return B_BAD_VALUE;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();
	/* prevents sSems[].owner == -1 >= means owned by a port */
	if (team < 0 || !team_is_valid(team))
		return B_BAD_TEAM_ID; 

	slot = *_cookie;
	if (slot >= sMaxSems)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	while (slot < sMaxSems) {
		if (sSems[slot].id != -1 && sSems[slot].u.used.owner == team) {
			GRAB_SEM_LOCK(sSems[slot]);
			if (sSems[slot].id != -1 && sSems[slot].u.used.owner == team) {
				// found one!
				fill_sem_info(&sSems[slot], info, size);

				RELEASE_SEM_LOCK(sSems[slot]);
				slot++;
				found = true;
				break;
			}
			RELEASE_SEM_LOCK(sSems[slot]);
		}
		slot++;
	}
	RELEASE_SEM_LIST_LOCK();
	restore_interrupts(state);

	if (!found)
		return B_BAD_VALUE;

	*_cookie = slot;
	return B_OK;
}


status_t
set_sem_owner(sem_id id, team_id team)
{
	int state;
	int slot;

	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (team < 0 || !team_is_valid(team))
		return B_BAD_TEAM_ID;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(sSems[slot]);

	if (sSems[slot].id != id) {
		RELEASE_SEM_LOCK(sSems[slot]);
		restore_interrupts(state);
		TRACE(("set_sem_owner: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	// ToDo: this is a small race condition: the team ID could already
	// be invalid at this point - we would lose one semaphore slot in
	// this case!
	// The only safe way to do this is to prevent either team (the new
	// or the old owner) from dying until we leave the spinlock.
	sSems[slot].u.used.owner = team;

	RELEASE_SEM_LOCK(sSems[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
}


//	#pragma mark - Syscalls


sem_id
_user_create_sem(int32 count, const char *userName)
{
	char name[B_OS_NAME_LENGTH];

	if (userName == NULL)
		return create_sem_etc(count, NULL, team_get_current_team_id());

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return create_sem_etc(count, name, team_get_current_team_id());
}


status_t
_user_delete_sem(sem_id id)
{
	return delete_sem(id);
}


status_t
_user_acquire_sem(sem_id id)
{
	return switch_sem_etc(-1, id, 1, B_CAN_INTERRUPT | B_CHECK_PERMISSION, 0);
}


status_t
_user_acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return switch_sem_etc(-1, id, count, flags | B_CAN_INTERRUPT | B_CHECK_PERMISSION, timeout);
}


status_t
_user_switch_sem(sem_id releaseSem, sem_id id)
{
	return switch_sem_etc(releaseSem, id, 1, B_CAN_INTERRUPT | B_CHECK_PERMISSION, 0);
}


status_t
_user_switch_sem_etc(sem_id releaseSem, sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return switch_sem_etc(releaseSem, id, count, flags | B_CAN_INTERRUPT | B_CHECK_PERMISSION, timeout);
}


status_t
_user_release_sem(sem_id id)
{
	return release_sem_etc(id, 1, B_CHECK_PERMISSION);
}


status_t
_user_release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	return release_sem_etc(id, count, flags | B_CHECK_PERMISSION);
}


status_t
_user_get_sem_count(sem_id id, int32 *userCount)
{
	status_t status;
	int32 count;

	if (userCount == NULL || !IS_USER_ADDRESS(userCount))
		return B_BAD_ADDRESS;

	status = get_sem_count(id, &count);
	if (status == B_OK && user_memcpy(userCount, &count, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_sem_info(sem_id id, struct sem_info *userInfo, size_t size)
{
	struct sem_info info;
	status_t status;

	if (userInfo == NULL || !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_sem_info(id, &info, size);
	if (status == B_OK && user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_sem_info(team_id team, int32 *userCookie, struct sem_info *userInfo,
	size_t size)
{
	struct sem_info info;
	int32 cookie;
	status_t status;

	if (userCookie == NULL || userInfo == NULL
		|| !IS_USER_ADDRESS(userCookie) || !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	status = _get_next_sem_info(team, &cookie, &info, size);

	if (status == B_OK) {
		if (user_memcpy(userInfo, &info, size) < B_OK
			|| user_memcpy(userCookie, &cookie, sizeof(int32)) < B_OK)
			return B_BAD_ADDRESS;
	}

	return status;
}


status_t
_user_set_sem_owner(sem_id id, team_id team)
{
	return set_sem_owner(id, team);
}
