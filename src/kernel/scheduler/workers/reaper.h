#pragma once

#include <scheduler/scheduler.h>
#include <utils/spinlock.h>

extern spinlock_t reaper_spinlock;
extern struct thread* threads_to_reap;

extern void reaper_thread();
