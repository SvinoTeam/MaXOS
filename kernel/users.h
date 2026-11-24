#ifndef USERS_H
#define USERS_H

#include "types.h"

#define MAX_USERS 16
#define MAX_USERNAME 32
#define MAX_PASS_LEN 160 /* "sha512$" + 128 hex chars + NUL */
#define PASSWD_PATH   "etc/passwd"

typedef struct {
	char name[MAX_USERNAME];
	char password[MAX_PASS_LEN]; /* "sha512$<hex>" (legacy plaintext also accepted) */
	uint32_t uid;
	uint32_t gid;
	int is_admin; // 1 - ROOT, 0 - DEFAULT USER
	int active;
} user_t;

extern user_t users[MAX_USERS];
extern int current_user_idx; /* -1 = nobody is logged in */

void users_init(void); /* load /etc/passwd (VFS or initramfs), create default root if empty */
void users_save(void); /* persist account database to /etc/passwd (VFS + initramfs tree) */

int user_add(const char *name, const char *pass, int is_admin); /* plaintext pass, hashed inside; returns slot or -1 */
int user_auth(const char *name, const char *pass); /* returns slot on success, -1 on failure */
int user_find(const char *name); /* returns slot or -1 */
int user_del(const char *name); /* 0 ok, -1 no user, -2 self, -3 last admin */
int user_set_password(const char *name, const char *newpass); /* 0 ok, -1 no user */
int user_count(void);

const char *current_username(void);
int current_is_admin(void);
uint32_t current_uid(void);
#endif
