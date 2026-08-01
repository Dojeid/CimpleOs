#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

static file_system_type_t *file_systems = NULL;
static dentry_t *vfs_root = NULL;

void vfs_init(void) {
    vfs_root = (dentry_t*)kmalloc(sizeof(dentry_t));
    memset(vfs_root, 0, sizeof(dentry_t));
    strcpy(vfs_root->d_name, "/");
    vga_print("[VFS] Virtual File System core initialized.\n");
}

int register_filesystem(file_system_type_t *fs) {
    if (!fs) return -1;
    fs->next = file_systems;
    file_systems = fs;
    vga_print("[VFS] Registered filesystem: ");
    vga_print(fs->name);
    vga_print("\n");
    return 0;
}

int vfs_mount(const char *dev_name, const char *dir_name, const char *fs_type) {
    // Basic mount: currently only supports mounting at root "/"
    if (strcmp(dir_name, "/") != 0) {
        vga_print("[VFS] Currently only root mounts are supported.\n");
        return -1;
    }

    file_system_type_t *fs = file_systems;
    while (fs) {
        if (strcmp(fs->name, fs_type) == 0) {
            super_block_t *sb = fs->mount(fs, dev_name);
            if (!sb) {
                vga_print("[VFS] Mount failed.\n");
                return -1;
            }
            vfs_root->d_sb = sb;
            vfs_root->d_inode = sb->s_root->d_inode;
            vga_print("[VFS] Successfully mounted ");
            vga_print(fs_type);
            vga_print(" on ");
            vga_print(dir_name);
            vga_print("\n");
            return 0;
        }
        fs = fs->next;
    }
    vga_print("[VFS] Filesystem type not found.\n");
    return -1;
}

dentry_t* vfs_lookup(const char *path) {
    if (!vfs_root || !vfs_root->d_inode) return NULL;
    if (strcmp(path, "/") == 0) return vfs_root;

    // Simple one-level lookup for now
    const char *name = path;
    if (name[0] == '/') name++;
    
    if (vfs_root->d_inode->i_op && vfs_root->d_inode->i_op->lookup) {
        return vfs_root->d_inode->i_op->lookup(vfs_root->d_inode, name);
    }
    return NULL;
}

file_t* vfs_open(const char *path, uint32_t flags) {
    dentry_t *dentry = vfs_lookup(path);
    if (!dentry || !dentry->d_inode) return NULL;

    file_t *f = (file_t*)kmalloc(sizeof(file_t));
    if (!f) return NULL;
    memset(f, 0, sizeof(file_t));
    f->f_dentry = dentry;
    f->f_inode = dentry->d_inode;
    f->f_pos = 0;
    f->f_mode = flags;
    f->f_op = dentry->d_inode->i_fop;
    
    if (f->f_op && f->f_op->open) {
        if (f->f_op->open(f->f_inode, f) != 0) {
            kfree(f);
            return NULL;
        }
    }
    return f;
}

int vfs_read(file_t *file, uint32_t size, uint8_t *buffer) {
    if (!file || !file->f_op || !file->f_op->read) return -1;
    int bytes = file->f_op->read(file, file->f_pos, size, buffer);
    if (bytes > 0) file->f_pos += bytes;
    return bytes;
}

int vfs_write(file_t *file, uint32_t size, const uint8_t *buffer) {
    if (!file || !file->f_op || !file->f_op->write) return -1;
    int bytes = file->f_op->write(file, file->f_pos, size, buffer);
    if (bytes > 0) file->f_pos += bytes;
    return bytes;
}

int vfs_close(file_t *file) {
    if (!file) return -1;
    if (file->f_op && file->f_op->release) {
        file->f_op->release(file->f_inode, file);
    }
    kfree(file);
    return 0;
}

// =========================================================================
// LEGACY TRANSITION HELPERS (Ramdisk / GUI / Shell)
// =========================================================================

dentry_t* vfs_get_root(void) {
    return vfs_root;
}

dentry_t* vfs_mkdir(dentry_t* parent, const char* name) {
    if (!parent) return 0;
    dentry_t* dir = (dentry_t*)kmalloc(sizeof(dentry_t));
    memset(dir, 0, sizeof(dentry_t));
    strncpy(dir->d_name, name, VFS_MAX_FILENAME - 1);
    dir->d_parent = parent;
    
    inode_t* ino = (inode_t*)kmalloc(sizeof(inode_t));
    memset(ino, 0, sizeof(inode_t));
    ino->i_mode = 0x4000; // DIR
    dir->d_inode = ino;
    
    parent->d_subdirs[parent->d_child_count++] = dir;
    return dir;
}

static int ram_read(file_t *file, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (!file || !file->f_inode || !buffer) return -1;
    if (offset >= file->f_inode->i_size) return 0;
    uint32_t read_bytes = size;
    if (offset + read_bytes > file->f_inode->i_size) {
        read_bytes = file->f_inode->i_size - offset;
    }
    if (file->f_inode->i_private) {
        memcpy(buffer, (uint8_t*)file->f_inode->i_private + offset, read_bytes);
    }
    return read_bytes;
}

static file_operations_t ram_fops = {
    .read = ram_read,
    .write = 0,
    .open = 0,
    .release = 0
};

dentry_t* vfs_create_file(dentry_t* parent, const char* name, const uint8_t* data, uint32_t size) {
    if (!parent) return 0;
    dentry_t* file = (dentry_t*)kmalloc(sizeof(dentry_t));
    memset(file, 0, sizeof(dentry_t));
    strncpy(file->d_name, name, VFS_MAX_FILENAME - 1);
    file->d_parent = parent;
    
    inode_t* ino = (inode_t*)kmalloc(sizeof(inode_t));
    memset(ino, 0, sizeof(inode_t));
    ino->i_mode = 0x8000; // FILE
    ino->i_size = size;
    ino->i_fop = &ram_fops;
    
    if (size > 0 && data) {
        ino->i_private = kmalloc(size + 1);
        memcpy(ino->i_private, data, size);
        ((uint8_t*)ino->i_private)[size] = 0;
    }
    file->d_inode = ino;
    
    parent->d_subdirs[parent->d_child_count++] = file;
    return file;
}

int vfs_remove(dentry_t* parent, const char* name) {
    if (!parent) return -1;
    for (uint32_t i = 0; i < parent->d_child_count; i++) {
        if (strcmp(parent->d_subdirs[i]->d_name, name) == 0) {
            dentry_t* child = parent->d_subdirs[i];
            if (child->d_inode && child->d_inode->i_private) {
                kfree(child->d_inode->i_private);
            }
            if (child->d_inode) kfree(child->d_inode);
            kfree(child);
            for (uint32_t j = i; j < parent->d_child_count - 1; j++) {
                parent->d_subdirs[j] = parent->d_subdirs[j + 1];
            }
            parent->d_child_count--;
            return 0;
        }
    }
    return -1;
}

void vfs_resolve_path(const char* cwd, const char* path, char* out_buf, size_t buf_size) {
    if (!out_buf || buf_size == 0) return;
    out_buf[0] = '\0';

    char raw[256];
    if (!path || strlen(path) == 0) {
        strncpy(raw, cwd ? cwd : "/", sizeof(raw) - 1);
    } else if (path[0] == '/') {
        strncpy(raw, path, sizeof(raw) - 1);
    } else {
        if (!cwd || strcmp(cwd, "/") == 0) {
            raw[0] = '/';
            raw[1] = '\0';
            strcat(raw, path);
        } else {
            strncpy(raw, cwd, sizeof(raw) - 1);
            strcat(raw, "/");
            strcat(raw, path);
        }
    }
    raw[sizeof(raw) - 1] = '\0';

    char segments[16][64];
    int seg_count = 0;

    int idx = 0;
    int len = strlen(raw);
    while (idx < len) {
        while (idx < len && raw[idx] == '/') idx++;
        if (idx >= len) break;

        char token[64];
        int t_idx = 0;
        while (idx < len && raw[idx] != '/') {
            if (t_idx < 63) token[t_idx++] = raw[idx];
            idx++;
        }
        token[t_idx] = '\0';

        if (strcmp(token, ".") == 0) continue;
        if (strcmp(token, "..") == 0) {
            if (seg_count > 0) seg_count--;
            continue;
        }

        if (seg_count < 16) {
            strncpy(segments[seg_count++], token, 63);
        }
    }

    if (seg_count == 0) {
        strcpy(out_buf, "/");
        return;
    }

    out_buf[0] = '\0';
    for (int i = 0; i < seg_count; i++) {
        strcat(out_buf, "/");
        strcat(out_buf, segments[i]);
    }
}
