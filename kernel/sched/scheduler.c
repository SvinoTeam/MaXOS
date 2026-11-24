// heart of O(N) scheduler
#include "scheduler.h"
#include "../printf.h"
#include "../panic.h"
#include "../arch/x86_64/tss.h"
#include "../string.h"
#include "../heap.h"

#define UKSTACK_SIZE 16384

task_t tasks[MAX_TASKS];
int current_task = 0;
int task_count = 0;

static void task_clear(task_t *t) {
    uint8_t *p = (uint8_t *)t;
    for (uint64_t i = 0; i < sizeof(task_t); i++) {
        p[i] = 0;
    }
}

static void reap_zombies(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i].active || tasks[i].state != TASK_ZOMBIE) continue;
        if (i == current_task) continue;

        int waited = 0;
        for (int j = 0; j < MAX_TASKS; j++) {
            if (tasks[j].active && tasks[j].wait_pid == tasks[i].pid) { waited = 1; break; }
        }
        if (waited) continue;

        if (tasks[i].kstack) {
            kfree(tasks[i].kstack);
        }
        task_clear(&tasks[i]);
        task_count--;
    }
}

void scheduler_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        task_clear(&tasks[i]);
        tasks[i].wait_pid = -1;
    }
    current_task = 0;
    task_count = 0;
}

int spawn_task(void (*func)(void), int priority, uint32_t uid) {
    reap_zombies();

    if (task_count >= MAX_TASKS) return -1;

    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i].active) { slot = i; break; }
    }
    if (slot == -1) return -1;

    task_clear(&tasks[slot]);
    tasks[slot].wait_pid = -1;

    tasks[slot].active     = 1;
    tasks[slot].state      = TASK_RUNNING;
    tasks[slot].priority   = priority;
    tasks[slot].counter    = priority;   // Initial quantum = priority
    tasks[slot].pid        = slot;
    tasks[slot].kstack_top = (uint64_t)(tasks[slot].stack + TASK_STACK_SIZE);
    tasks[slot].is_user    = 0;
    tasks[slot].uid        = uid;
	
	
	
    // build stack: function address as the "return address"
    uintptr_t base = (uintptr_t)(tasks[slot].stack + TASK_STACK_SIZE);
    uint64_t *top = (uint64_t *)(base & ~0xFULL);
    *(--top) = 0;                        // padding
    *(--top) = (uint64_t)task_trampoline; // ret: bro is mine
    *(--top) = 0;                        // rbp
    *(--top) = (uint64_t)func;           // rbx
    *(--top) = 0;                        // r12
    *(--top) = 0;                        // r13
    *(--top) = 0;                        // r14
    *(--top) = 0;                        // r15
    tasks[slot].rsp = (uint64_t)top;

    task_count++;
    return slot;
}

// O(N) main function
void schedule(void) {
    int i, next, c;

    while (1) {
        c = -1;
        next = 0;
        int runnable = 0;

        // find task with max counter
        for (i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].active && tasks[i].state == TASK_RUNNING) {
                runnable++;
                if (tasks[i].counter > c) {
                    c = tasks[i].counter;
                    next = i;
                }
            }
        }
        if (!runnable) {
            panic("scheduler: no runnable tasks");
        }

        if (c > 0) break;

        for (i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].active) {
                tasks[i].counter = (tasks[i].counter >> 1) + tasks[i].priority;
            }
        }
    }

    if (next != current_task) {
        int prev = current_task;
        current_task = next;
        tss_set_rsp0(tasks[next].kstack_top);
        switch_context(&tasks[prev].rsp, tasks[next].rsp);
    }
}

// PIT, please, come here ;O
void scheduler_tick(void) {
    if (task_count <= 1) return;

    if (tasks[current_task].state == TASK_RUNNING) {
        tasks[current_task].counter--;
    }
    if (tasks[current_task].counter <= 0) {
        schedule();
    }
}

/*
 * Spawn user task (*.max files, ring3)
 */
int spawn_user_task(uint64_t entry, uint64_t user_stack_top, int priority, const char *name, uint32_t uid) {
    reap_zombies();

    if (task_count >= MAX_TASKS) return -1;
    if (!entry || !user_stack_top) return -1;   // привет, kctx_rsp = 0 :)

    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i].active) { slot = i; break; }
    }
    if (slot == -1) return -1;

    task_clear(&tasks[slot]);
    task_t *t = &tasks[slot];
    t->wait_pid = -1;

    // allocate kernel stack for this task
    t->kstack = (uint8_t *)kmalloc(UKSTACK_SIZE);
    if (!t->kstack) {
        task_clear(t);
        return -1;
    }
    t->kstack_top = (uint64_t)(t->kstack + UKSTACK_SIZE);

    // metadata
    t->is_user    = 1;
    t->user_entry = entry;
    t->user_rsp   = user_stack_top;
    t->state      = TASK_RUNNING;
    t->priority   = priority;
    t->counter    = priority;
    t->pid        = slot;
    t->active     = 1;
    t->uid        = uid;
	t->fd_table[0]= NULL; // stdin
	t->fd_table[1]= NULL; // stdout
	t->fd_table[2]= NULL; // stderr
	for(int i=3;i<10;i++) {
		t->fd_table[i] = NULL;
	}
	
    // copy name
    if (name) {
        int i;
        for (i = 0; i < 15 && name[i] != '\0'; i++) t->name[i] = name[i];
        t->name[i] = '\0';
    }

    // uhmm.. Can I skip explaining how this works? :)
    uintptr_t base = t->kstack_top;
    uint64_t *top = (uint64_t *)(base & ~0xFULL);   // align 16 byte
    *(--top) = 0;                          // padding
    *(--top) = (uint64_t)user_trampoline;  // ret: jump to trampoline
    *(--top) = 0;                          // rbp
    *(--top) = entry;                      // rbx
    *(--top) = user_stack_top;             // r12
    *(--top) = 0;                          // r13
    *(--top) = 0;                          // r14
    *(--top) = 0;                          // r15
    t->rsp = (uint64_t)top;

    task_count++;
    return slot;
}

int task_alive(int pid) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].active && tasks[i].pid == pid && tasks[i].state != TASK_ZOMBIE)
            return 1;
    }
    return 0;
}

/* Wait child! 0 = done, -1 = task not found. */
int task_wait(int pid, int *exit_code) {
    for (;;) {
        int slot = -1;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].active && tasks[i].pid == pid) { slot = i; break; }
        }
        if (slot < 0) return -1;

        if (tasks[slot].state == TASK_ZOMBIE) {
            if (exit_code) *exit_code = tasks[slot].exit_code;
            if (tasks[slot].kstack) {
                kfree(tasks[slot].kstack); 
            }
            task_clear(&tasks[slot]);
            task_count--;
            return 0;
        }

        __asm__ volatile("cli");
        if (tasks[slot].state != TASK_ZOMBIE) {
            tasks[current_task].wait_pid = pid;
            tasks[current_task].state    = TASK_SLEEPING;
            schedule();
        }
        __asm__ volatile("sti");
    }
}

/*
 * Correct stop of program (*.max files, ring3)
 */
void task_exit(int code) {
    __asm__ volatile("cli");

    task_t *t = &tasks[current_task];
    t->state     = TASK_ZOMBIE;
    t->exit_code = code;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].active &&
            tasks[i].state == TASK_SLEEPING &&
            tasks[i].wait_pid == t->pid) {
            tasks[i].wait_pid = -1;
            tasks[i].state    = TASK_RUNNING;
            tasks[i].counter  = tasks[i].priority; 
        }
    }

    schedule();

    while (1) { __asm__ volatile("hlt"); }
}
