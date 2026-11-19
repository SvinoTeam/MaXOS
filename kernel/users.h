#ifndef USERS_H
#define USERS_H

#include "types.h"

#define MAX_USERS 16
#define MAX_USERNAME 32
#define MAX_PASS_LEN 128
#define PASSWD_PATH   "etc/passwd"

typedef struct {
	char name[MAX_USERNAME];
	char password[MAX_PASS_LEN]; // WIP: SHA512 is getting ready...
	uint32_t uid;
	uint32_t gid;
	int is_admin; // 1 - ROOT, 0 - DEFAULT USER 
	int active;
} user_t;

extern user_t users[MAX_USERS];
extern int current_user_idx;
void users_init(void);
void users_save(void);

int user_add(const char *name, const char *pass, int is_admin);
int user_auth(const char *name, const char *pass);
#endif
