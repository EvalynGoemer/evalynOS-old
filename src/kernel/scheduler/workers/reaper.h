#pragma once

#include <scheduler/scheduler.h>

extern struct thread* threads_to_reap;

extern void reaper_thread();
