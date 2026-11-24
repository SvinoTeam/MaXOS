/* MaXOS -- user accounts.
 * Database format (/etc/passwd), one account per line:
 *     name:password:uid:gid:is_admin
 * Password is stored as "sha512$<128 hex chars>". Legacy plaintext
 * entries are still accepted by user_auth() so old files keep working.
 */
#include "users.h"
#include "printf.h"
#include "string.h"
#include "vfs.h"
#include "initramfs.h"
#include "sha512.h"

user_t users[MAX_USERS];
int    current_user_idx = -1;

const char *current_username(void) {
	if (current_user_idx >= 0 && users[current_user_idx].active) {
		return users[current_user_idx].name;
	}
	return "?";
}

int current_is_admin(void) {
	return (current_user_idx >= 0 && users[current_user_idx].active &&
	        users[current_user_idx].is_admin);
}

uint32_t current_uid(void) {
	if (current_user_idx >= 0 && users[current_user_idx].active) {
		return users[current_user_idx].uid;
	}
	return (uint32_t)-1;
}

int user_count(void) {
	int n = 0;
	for (int i = 0; i < MAX_USERS; i++)
		if (users[i].active) n++;
	return n;
}

int user_find(const char *name) {
	for (int i = 0; i < MAX_USERS; i++)
		if (users[i].active && strcmp(users[i].name, name) == 0)
			return i;
	return -1;
}

static void hash_password(const char *plain, char *out, uint32_t outsz) {
	char hex[SHA512_HEX_LEN + 1];
	sha512_hex(plain, strlen(plain), hex);
	snprintf(out, outsz, "sha512$%s", hex);
}

static int password_ok(const char *stored, const char *plain) {
	if (strncmp(stored, "sha512$", 7) == 0) {
		char hex[SHA512_HEX_LEN + 1];
		sha512_hex(plain, strlen(plain), hex);
		return strcmp(stored + 7, hex) == 0;
	}
	return strcmp(stored, plain) == 0; /* legacy plaintext entry */
}

static int valid_name(const char *name) {
	size_t n = strlen(name);
	if (n == 0 || n >= MAX_USERNAME) return 0;
	for (size_t i = 0; i < n; i++) {
		char c = name[i];
		int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		         (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
		if (!ok) return 0;
	}
	return 1;
}

static uint32_t alloc_uid(int is_admin) {
	uint32_t uid = is_admin ? 0 : 1000;
	for (;;) {
		int taken = 0;
		for (int i = 0; i < MAX_USERS; i++)
			if (users[i].active && users[i].uid == uid) { taken = 1; break; }
		if (!taken) return uid;
		uid++;
	}
}

/* internal: store the password field exactly as given (already hashed) */
static int user_add_raw(const char *name, const char *pass_field, int is_admin) {
	if (!valid_name(name) || user_find(name) >= 0)
		return -1; /* bad name or duplicate */
	for (int i = 0; i < MAX_USERS; i++) {
		if (!users[i].active) {
			strncpy(users[i].name, name, MAX_USERNAME - 1);
			users[i].name[MAX_USERNAME - 1] = '\0';
			strncpy(users[i].password, pass_field, MAX_PASS_LEN - 1);
			users[i].password[MAX_PASS_LEN - 1] = '\0';
			users[i].uid = alloc_uid(is_admin);
			users[i].gid = is_admin ? 0 : 100;
			users[i].is_admin = is_admin ? 1 : 0;
			users[i].active = 1;
			return i;
		}
	}
	return -1; /* user table full */
}

int user_add(const char *name, const char *pass, int is_admin) {
	char field[MAX_PASS_LEN];
	hash_password(pass, field, sizeof(field));
	return user_add_raw(name, field, is_admin);
}

int user_auth(const char *name, const char *pass) {
	int idx = user_find(name);
	if (idx < 0)
		return -1;
	return password_ok(users[idx].password, pass) ? idx : -1;
}

int user_del(const char *name) {
	int idx = user_find(name);
	if (idx < 0) return -1;
	if (idx == current_user_idx) return -2; /* cannot delete yourself */
	if (users[idx].is_admin) {
		int admins = 0;
		for (int i = 0; i < MAX_USERS; i++)
			if (users[i].active && users[i].is_admin) admins++;
		if (admins <= 1) return -3; /* keep at least one admin */
	}
	memset(&users[idx], 0, sizeof(user_t));
	return 0;
}

int user_set_password(const char *name, const char *newpass) {
	int idx = user_find(name);
	if (idx < 0) return -1;
	hash_password(newpass, users[idx].password, MAX_PASS_LEN);
	return 0;
}

/* name:pass:uid:gid:admin */
static void parse_line(char *s) {
	if (*s == '\0' || *s == '#')
		return;
	char *name = s;
	char *p = s;
	while (*p && *p != ':') p++;
	if (!*p) return;
	*p++ = '\0';
	char *pass = p;
	while (*p && *p != ':') p++;
	if (!*p) return;
	*p++ = '\0';
	char *uid_s = p;
	while (*p && *p != ':') p++;
	if (!*p) return;
	*p++ = '\0';
	char *gid_s = p;
	while (*p && *p != ':') p++;
	if (!*p) return;
	*p++ = '\0';
	char *adm_s = p;
	int idx = user_add_raw(name, pass, 0);
	if (idx < 0)
		return;
	users[idx].uid = (uint32_t)atoi(uid_s);
	users[idx].gid = (uint32_t)atoi(gid_s);
	users[idx].is_admin = atoi(adm_s) ? 1 : 0;
}

void users_init(void) {
	memset(users, 0, sizeof(users));
	current_user_idx = -1; /* nobody is logged in until the login prompt */

	static char buf[4096];
	int n = vfs_read(PASSWD_PATH, buf, sizeof(buf) - 1);
	if (n <= 0) {
		fs_node_t *etc = find_child(&root_node, "etc");
		fs_node_t *pw = etc ? find_child(etc, "passwd") : NULL;
		if (pw && pw->data && pw->size > 0) {
			n = (pw->size < sizeof(buf) - 1) ? (int)pw->size : (int)sizeof(buf) - 1;
			memcpy(buf, pw->data, (size_t)n);
		}
	}
	if (n > 0) {
		buf[n] = '\0';
		char *line = buf;
		for (int i = 0; i <= n; i++) {
			if (buf[i] == '\n' || buf[i] == '\0') {
				buf[i] = '\0';
				parse_line(line);
				line = &buf[i + 1];
			}
		}
	}
	if (user_count() == 0) {
		/* no database -- create the default root account */
		user_add("root", "root", 1);
	}
}

void users_save(void) {
	static char buf[4096];
	int off = 0;
	for (int i = 0; i < MAX_USERS; i++) {
		if (!users[i].active) continue;
		int left = (int)sizeof(buf) - off;
		if (left <= 1) break;
		int w = snprintf(buf + off, (size_t)left,
		                 "%s:%s:%u:%u:%d\n",
		                 users[i].name, users[i].password,
		                 users[i].uid, users[i].gid, users[i].is_admin);
		if (w < 0 || w >= left) break;
		off += w;
	}

	int ok = 0;
	if (vfs_write(PASSWD_PATH, buf, (uint32_t)off) > 0)
		ok = 1;

	/* keep the initramfs tree in sync so `cd etc; cat passwd` shows the DB */
	fs_node_t *etc = find_child(&root_node, "etc");
	if (!etc) etc = create_dir(&root_node, "etc");
	if (etc) {
		fs_node_t *pw = find_child(etc, "passwd");
		if (!pw) pw = create_file(etc, "passwd");
		if (pw && write_file(pw, (const uint8_t *)buf, (uint32_t)off) == 0)
			ok = 1;
	}

	if (ok) {
		// only for debug
		//kprintf("users: saved %d account(s) to /%s\n", user_count(), PASSWD_PATH);	
	}
	else
		kprintf("failed, saving '/etc/passwd' failure\n");
}
