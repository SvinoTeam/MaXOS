#include "users.h"
#include "printf.h"
#include "string.h"
#include "heap.h"
#include "vfs.h"

user_t users[MAX_USERS];
int    current_user_idx = -1;

const char *current_username(void) {
	if (current_user_idx >= 0 && users[current_user_idx].active) {
		return users[current_user_idx].name;
	}
	return "?";
}

int current_is_admin(void) {
	return (current_user_idx >= 0 && users[current_user_idx].is_admin);	
}

int user_add(const char *name, const char *pass, int is_admin) {
	for (int i = 0; i < MAX_USERS; i++) {
		if (users[i].active && strcmp(users[i].name, name) == 0) {
			return -1; // duplicate
		}
	}
	for (int i = 0; i < MAX_USERS; i++) {
		if (!users[i].active) {
			strncpy(users[i].name, name, MAX_USERNAME - 1);
			users[i].name[MAX_USERNAME - 1] = '\0';
			strncpy(users[i].password, pass, MAX_PASS_LEN - 1);
			users[i].password[MAX_PASS_LEN - 1] = '\0';
			users[i].uid = (uint32_t)i;
			users[i].gid = is_admin ? 0 : 100;
			users[i].is_admin = is_admin;
			users[i].active = 1;
			return i;
		}
	}
	return -1; // if MAX_USERS ==0
}

// name:pass:uid:gid:admin
static void parse_line(char *s) {
	if (*s == '\0' || *s == '#'){
		return;
	}
	char *name = s;
	char *p = s;
	while (*p && *p != ':') p++; if (*p) *p++ = '\0';
	char *pass = p;
	while (*p && *p != ':') p++; if (*p) *p++ = '\0';
	char *uid_s = p;
	while (*p && *p != ':') p++; if (*p) *p++ = '\0';
	char *gid_s = p;
	while (*p && *p != ':') p++; if (*p) *p++ = '\0';
	char *adm_s = p;
	int idx = user_add(name,pass,0);
	if (idx < 0) {
		return;
	}
	users[idx].uid = (uint32_t)atoi(uid_s);
	users[idx].gid  (uint32_t)atoi(gid_s);
	users[idx].is_admin = atoi(adm_s) ? 1 : 0;
}

void users_init(void) {
	for (int i = 0; i < MAX_USERS; i++) {
		users[i].active = 0;
	}
	static char buf[4096];
	int n = vfs_read(PASSWD_PATH, buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] == '\0';
		kprintf("Loaded /etc/passwd (%d bytes)\n", n);
		char *line = buf;
		for (int i = 0; i <= n; i++) {
			if (buf[i] == '\n' || buf[i] == '\0') {
				buf[i] == '\0';
				parse_line(line);
				line = &buf[i+1];
			}
		}
	} else {
		// root:root:0:0:1
		user_add("root", "root", 1);
	}
	for (int i = 0; i < MAX_USERS; i++) {
		if (users[i].active && users[i].is_admin) {
			current_user_idx = i;
			break;
		}
	}
}

void users_save(void) {
    char buf[4096];
    int off = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        if (!users[i].active) continue;
        off += snprintf(buf + off, sizeof(buf) - off,
                        "%s:%s:%u:%u:%d\n",
                        users[i].name, users[i].password,
                        users[i].uid, users[i].gid, users[i].is_admin);
        if (off >= (int)sizeof(buf) - 1) break;
    }
    int w = vfs_write(PASSWD_PATH, buf, (uint32_t)off);
    if (w > 0) {
    	kprintf("Created new user (+%d bytes)",w);	
    }
    else {
    	kprintf("Save failed\n");	
    }
}

int users_auth(const char *name, const char *pass) {
	for (int i = 0; i < MAX_USERS; i++) {
		if (users[i].active && strcmp(users[i].name, name) == 0) {
			if (users[i].active && strcmp(users[i].name,name) == 0) {
				return (strcmp(users[i].password, pass) == 0) ? i : -1;
			}
		}
		return -1;
	}
}
