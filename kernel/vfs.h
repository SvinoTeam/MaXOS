/* 4/11/26 MaXOS: VFS (virtual file system).
 * created for initramfs and inicpio.cpio.
*/

#ifndef VFS_H
#define VFS_H
#include "types.h"
#include "initramfs.h"

#define VFS_MAX_PATH 256
#define VFS_MAX_FILES 1024

// `fs_node_t` -> from initramfs.h
typedef fs_node_t vfs_file_t;

/* Initialization VFS*/
void vfs_init(void);
/* Mount file system*/
int vfs_mount(const char *path, const char *fs_type, void *device);
/* Read file*/
int vfs_read(const char *path, void *buffer, uint32_t size);
/* Write file*/
int vfs_write(const char *path, const void *buffer, uint32_t size);
/* File list in directory*/
int vfs_readdir(const char *path, char *buffer, uint32_t buffer_size);
vfs_file_t *vfs_create_file(const char *path, uint32_t flags, uint32_t uid, uint32_t gid);
vfs_file_t *vfs_get_root(void);
/* find file*/
vfs_file_t *vfs_find(const char *path);
/* get file via path (return `NULL` if file not found)*/
vfs_file_t *vfs_lookup(const char *path);
#endif
