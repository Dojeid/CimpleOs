#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_MAX_FILENAME 64
#define VFS_MAX_CHILDREN 32
#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02

typedef struct vfs_node {
    char name[VFS_MAX_FILENAME];
    uint32_t type;
    uint32_t size;
    uint8_t* data;
    uint32_t child_count;
    struct vfs_node* children[VFS_MAX_CHILDREN];
    struct vfs_node* parent;
} vfs_node_t;

void vfs_init(void);
vfs_node_t* vfs_get_root(void);
vfs_node_t* vfs_lookup(vfs_node_t* relative_to, const char* path);
vfs_node_t* vfs_create_file(vfs_node_t* parent, const char* name, const uint8_t* data, uint32_t size);
vfs_node_t* vfs_mkdir(vfs_node_t* parent, const char* name);
int vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
int vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, const uint8_t* buffer);
int vfs_remove(vfs_node_t* parent, const char* name);
int vfs_get_path(vfs_node_t* node, char* buffer, size_t max_len);
void vfs_resolve_path(const char* cwd, const char* path, char* out_buf, size_t buf_size);

#endif // VFS_H
