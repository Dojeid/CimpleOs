#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_MAX_FILENAME 256
#define MAX_MOUNTPOINTS 16

struct inode;
struct dentry;
struct super_block;
struct file;
struct file_system_type;

// File operations (read, write, ioctl, etc)
typedef struct file_operations {
    int (*read)(struct file *file, uint32_t offset, uint32_t size, uint8_t *buffer);
    int (*write)(struct file *file, uint32_t offset, uint32_t size, const uint8_t *buffer);
    int (*open)(struct inode *inode, struct file *file);
    int (*release)(struct inode *inode, struct file *file);
} file_operations_t;

// Inode operations (lookup, mkdir, create, etc)
typedef struct inode_operations {
    struct dentry* (*lookup)(struct inode *dir, const char *name);
    int (*mkdir)(struct inode *dir, const char *name);
    int (*create)(struct inode *dir, const char *name);
} inode_operations_t;

// Superblock operations (read_inode, write_inode, etc)
typedef struct super_operations {
    int (*read_inode)(struct inode *inode);
    int (*write_inode)(struct inode *inode);
} super_operations_t;

// Superblock structure (represents a mounted filesystem)
typedef struct super_block {
    struct file_system_type *s_type;
    struct dentry *s_root; // Root directory dentry
    super_operations_t *s_op;
    void *s_fs_info; // Private filesystem data (e.g. ext4 superblock)
} super_block_t;

// Inode structure (represents a file or directory on disk)
typedef struct inode {
    uint32_t i_ino;       // Inode number
    uint32_t i_mode;      // File type (dir, file, etc) and permissions
    uint32_t i_size;      // Size in bytes
    uint32_t i_blocks;    // Number of blocks
    super_block_t *i_sb;  // Associated superblock
    inode_operations_t *i_op;
    file_operations_t *i_fop;
    void *i_private;      // Private data (e.g. ext4 inode cache)
} inode_t;

// Dentry structure (represents a path component in RAM)
typedef struct dentry {
    char d_name[VFS_MAX_FILENAME];
    struct inode *d_inode;    // Associated inode
    struct dentry *d_parent;  // Parent directory
    struct dentry *d_subdirs[32]; // Simplistic child array for now
    uint32_t d_child_count;
    super_block_t *d_sb;
    
    // Legacy compatibility fields
    uint32_t type;
    uint32_t size;
    uint8_t* data;
} dentry_t;

typedef dentry_t vfs_node_t;
#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02

// File structure (represents an open file descriptor in a process)
typedef struct file {
    dentry_t *f_dentry;
    inode_t *f_inode;
    uint32_t f_pos;       // Current file offset
    uint32_t f_mode;      // Read/Write flags
    file_operations_t *f_op;
    void *f_private;      // Private data for the open instance
} file_t;

// Filesystem type definition
typedef struct file_system_type {
    const char *name;
    super_block_t* (*mount)(struct file_system_type *fs_type, const char *dev_name);
    struct file_system_type *next;
} file_system_type_t;

// VFS Core API
void vfs_init(void);
int register_filesystem(file_system_type_t *fs);
int vfs_mount(const char *dev_name, const char *dir_name, const char *fs_type);
dentry_t* vfs_lookup(const char *path);

// Process-level VFS API (returning pointers, syscalls will manage FDs)
file_t* vfs_open(const char *path, uint32_t flags);
int vfs_read(file_t *file, uint32_t size, uint8_t *buffer);
int vfs_write(file_t *file, uint32_t size, const uint8_t *buffer);
int vfs_close(file_t *file);

// Legacy wrappers for GUI & Shell apps transition
dentry_t* vfs_get_root(void);
dentry_t* vfs_mkdir(dentry_t* parent, const char* name);
dentry_t* vfs_create_file(dentry_t* parent, const char* name, const uint8_t* data, uint32_t size);
int vfs_remove(dentry_t* parent, const char* name);
void vfs_resolve_path(const char* cwd, const char* path, char* out_buf, size_t buf_size);

#endif // VFS_H
