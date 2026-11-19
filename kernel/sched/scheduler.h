// Scheduler -- MaXOS, 16/11/26.
// Simple O(N) scheduler (how linux 0.11)
// Author: github.com/lev445
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../types.h"

// XXX: Add VMM here
// WIP: VMM is not created yet
#define MAX_TASKS 8
#define TASK_STACK_SIZE 4096

// Task state
#define TASK_RUNNING  0
#define TASK_SLEEPING 1
#define TASK_ZOMBIE   2

// task structure
typedef struct task {
	struct task *next;
	uint64_t rsp; // stack ptr to task
	uint8_t stack[TASK_STACK_SIZE];
	int state; // task status
	int counter; // counter
	int priority; // Nice+
	int pid; // Process Id
	int active; // is active
	uint8_t *kstack;
	uint64_t kstack_top; // -> TSS.RSP0
	uint64_t user_rsp; // ring3 RSP
	uint64_t user_entry; // ring3 RIP
	int is_user;
	int exit_code;
	char name[16];
	int  wait_pid; // -1 - nothing wait, other return child pid
} task_t;

void scheduler_init(void);
void scheduler_tick(void);
int spawn_task(void (*func)(void), int priority);
void switch_context(uint64_t *old_rsp_ptr, uint64_t new_rsp);

extern task_t tasks[MAX_TASKS];
extern int current_task;
extern int task_count;
extern void task_trampoline(void);
extern void user_trampoline(void);
int spawn_user_task(uint64_t entry, uint64_t user_stack_top, int priority, const char *name);
void task_exit(int code);
int task_wait(int pid, int *exit_code);
int task_alive(int pid);

#endif
