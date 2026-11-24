/* 4/11/26 MaXOS -- VFS (Virtual File System)*/
/* fully re-writed at 24/11/26*/
#include "vfs.h"
#include "printf.h"
#include "string.h"
#include "heap.h"

static vfs_file_t *root = NULL;

/* soo we just locate `root` var to root_node */
void vfs_init(void) {
	root = &root_node;
}

vfs_file_t *vfs_get_root(void) {
	return root;
}

/* split_path (static,return void)
* this function just parse path like `etc/passwd` to parent="etc" and name="passwd"
*/

static void split_path(const char *path, char *parent, char *name) {
	const char *last_splash = NULL;
	const char *p = path;
	while (*p){
		if (*p == '/') {
			last_splash=p;
		}
		p++;
	}
	if (last_splash) {
		int len = last_splash - path;
		if (len > 0 && len < VFS_MAX_PATH) {
			strncpy(parent, path,len);
			parent[len] = '\0';
		} else {
			strcpy(parent, "/");
		}
		strcpy(name,last_splash+1);
	}
	else {
		strcpy(parent,"/");
		strcpy(name,path);
	}
}

/* line (like `etc/passwd`) -> structure (in fs_node_t*)*/
vfs_file_t *vfs_lookup(const char *path) {
    
    if (!path || !*path) return NULL;
    
    vfs_file_t *node;
    if (*path == '/') {
    	node = root;
    	path++;
    }
    else {
    	node = current_dir;
    }
    
    if (!node) {
        return NULL;
    }
    
    if (*path == '/') path++;
    
    while (*path) {
        char comp[64];
        int i = 0;
        while (*path && *path != '/' && i < 63) comp[i++] = *path++;
        comp[i] = '\0';
        
        if (i == 0) {
            if (*path == '/') { path++; continue; }
        }
        
        node = find_child(node, comp);
        if (!node) {
            return NULL;
        }
        
        if (*path == '/') {
			path++;
       	}
        	
    }
    return node;
}

/* this is alias for backward compatibility*/
vfs_file_t *vfs_find(const char *path) {
	return vfs_lookup(path);
}

int vfs_read(const char *path, void *buffer, uint32_t size) {
    vfs_file_t *file = vfs_lookup(path);
    if (!file || file->is_directory) return -1;
    uint32_t bytes = (size < file->size) ? size : file->size;
    if (file->data) {
        memcpy(buffer, file->data, bytes);
        return bytes;
    }
    return 0;
}

int vfs_write(const char *path, const void *buffer, uint32_t size) {
    if (!path || !buffer || size == 0) {
    	return -1;	
    }
    vfs_file_t *file = vfs_lookup(path);
    if (!file) {
        file = vfs_create_file(path, 0644, 0, 0);
        if (!file) {
        	return -1;	
        }
    }
    if (file->data) kfree(file->data);
    file->data = (uint8_t *)kmalloc(size);
    if (!file->data) {
    	return -1;
    }
    memcpy(file->data, buffer, size);
    file->size = size;
    return size;
}

vfs_file_t *vfs_create_file(const char *path, uint32_t flags, uint32_t uid, uint32_t gid) {
	char parent_path[VFS_MAX_PATH];
	char name[64];
	split_path(path, parent_path, name);
	vfs_file_t *parent = vfs_lookup(parent_path);
	if (!parent) {
		return NULL;	
	}
	return create_file(parent, name); 
}
