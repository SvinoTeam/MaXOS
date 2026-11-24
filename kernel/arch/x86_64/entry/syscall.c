/* ебать я даун */
#include "syscall.h"
#include "../../../printf.h"
#include "../../../console.h"
#include "../../../pit.h"
#include "../../../ps2.h"
#include "../../../panic.h"
#include "../../../sched/scheduler.h"
#include "../lib/io.h"
#include "../../../vfs.h"
#include "../../../string.h"

extern uint64_t syscall_stack_fault;

#define USER_MEM_LO  0xA00000ULL
#define USER_MEM_HI  0xE00000ULL   /* exclusive */
#define WRITE_MAX    (1u << 20)

static int user_range_ok(uint64_t addr, uint64_t len) {
    if (len == 0)
        return 1;
    if (addr < USER_MEM_LO || addr > USER_MEM_HI)
        return 0;
    if (len > (USER_MEM_HI - USER_MEM_LO))
        return 0;
    return len <= (USER_MEM_HI - addr);
}

uint64_t syscall_dispatch(syscall_regs_t *r) {
    switch (r->rax) {

    case SYS_PUTC: /* putc(char c) */
        console_putc((char)r->rdi);
        return 0;

    case SYS_WRITE: { /* write(const char *buf, uint64_t n) */
        uint64_t addr = r->rdi;
        uint64_t len  = r->rsi;
        //kprintf("KernelDebug[write](addr=0x%lx, len=%lu)\n", addr, len);
        if (!user_range_ok(addr, len)) {
            kprintf("KernelError[write](Addr: %lx, LenSize: %lu, Syscall: %lu): ptr outside user memory\n",
                    (unsigned long)addr, (unsigned long)len, (unsigned long)r->rax);
            return SYSCALL_ERR;
        }
        if (len > (1u << 20)) { /* limit */
            kprintf("KernelError[write]: (Addr: %lx, LenSize: %lu, Syscall: %lu) size is too big!\n", (unsigned long)addr, (unsigned long)len, (unsigned long)r->rax);
            return SYSCALL_ERR;
        }
        const char *buf = (const char *)(uintptr_t)addr;

        for (uint64_t i = 0; i < len; i++)
            console_putc(buf[i]);
        return len;
    }

    case SYS_AI:  /* :) */
        kprintf("Sizran'\n");
        return 0;

    case SYS_UPTIME: /* uptime_ms() */
        return uptime_ms();
	case SYS_OPEN: {
		uint64_t u_path = r->rdi;
		char k_path[256];
		int i = 0;
		while (i < 255) {
			// We must ensure that the specific bit resides in userspace.
			if (!user_range_ok(u_path + i, 1)) {
				return -1; // efault: bad ptr
			}
			char c = *(char *)(uintptr_t)(u_path +i);
			k_path[i] = c;
			if (c == '\0') {
				break; // check for null terminator
			}
			i++;
		}
		k_path[255] = '\0'; // !!! need null terminator!!!
		vfs_file_t *file = vfs_lookup(k_path);
		if (!file) {
			return -1; // enoent: file not found
		}
		/* find for available slot in fd
		   0,1,2 :: stdin,stdout,stderr
		*/
		int fd = -1;
		for (int i = 3; i < 16; i++) {
			if (tasks[current_task].fd_table[i] == NULL) {
				fd = i;
				break;
			}
		}
		if (fd == -1) {
			return -1; // emfile: mooooooooreeeeeeeeeeeeeeeeee fd pleeeeeaseeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee stupidddddd userrrrrrrrrrrrrrrrrrrr 
		}
		tasks[current_task].fd_table[fd] = file;
		return fd;
	}
	case SYS_CLOSE: {
		int fd = (int)r->rdi;
		if (fd<3 || fd >= 16) {
			return -1;
		}
		tasks[current_task].fd_table[fd] = NULL;
		return 0;
	}
	case SYS_READ: {
		int fd = (int)r->rdi;
		uint64_t buf = r->rsi;
		uint64_t count = r->rdx;
		kprintf("sys_read: fd=%d, buf=x%lx, count=%lu\n", fd,(unsigned long)buf, (unsigned long)count);
		if (fd < 3 || fd >= 16) {
			return -1;
		}
		vfs_file_t *file = tasks[current_task].fd_table[fd];
		if (!file || file->is_directory) {
			return -1;
		}
		if (!user_range_ok(buf,count)) {
			return -1; // efault
		}
		uint64_t bytes = (count < file->size) ? count : file->size;
		if (file->data) {
		    memcpy((void *)(uintptr_t)buf, file->data, bytes);
		}
		kprintf("sys_read: -> bytes=%lu\n", (unsigned long)bytes);
		return bytes;
	}
    case SYS_GETCHAR: /* getchar(), -1 if empty */
        return kbd_avail() ? (uint64_t)(int64_t)kbd_getchar() : SYSCALL_ERR;
    case SYS_EXIT: { /* exit(int code) */
        task_exit((int)(int64_t)r->rdi);
        panic("task_exit() returned!!");
        while(1) {
        	__asm__ volatile("cli; hlt");
        }
    }

    default:
		kprintf("KernelError[Syscall]: syscall %lu not found", (unsigned long)r->rax);
        return SYSCALL_ERR;
    }
}
