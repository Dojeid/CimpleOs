#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

static vfs_node_t root_node;

void vfs_init(void) {
    memset(&root_node, 0, sizeof(vfs_node_t));
    strcpy(root_node.name, "/");
    root_node.type = VFS_DIRECTORY;
    vga_print("[VFS] Root File System initialized.\n");
}

vfs_node_t* vfs_get_root(void) {
    return &root_node;
}

vfs_node_t* vfs_mkdir(vfs_node_t* parent, const char* name) {
    if (!parent || parent->type != VFS_DIRECTORY) return 0;
    if (parent->child_count >= VFS_MAX_CHILDREN) return 0;

    vfs_node_t* dir = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!dir) return 0;

    memset(dir, 0, sizeof(vfs_node_t));
    strncpy(dir->name, name, VFS_MAX_FILENAME - 1);
    dir->type = VFS_DIRECTORY;
    dir->parent = parent;

    parent->children[parent->child_count++] = dir;
    return dir;
}

vfs_node_t* vfs_create_file(vfs_node_t* parent, const char* name, const uint8_t* data, uint32_t size) {
    if (!parent || parent->type != VFS_DIRECTORY) return 0;
    if (parent->child_count >= VFS_MAX_CHILDREN) return 0;

    vfs_node_t* file = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!file) return 0;

    memset(file, 0, sizeof(vfs_node_t));
    strncpy(file->name, name, VFS_MAX_FILENAME - 1);
    file->type = VFS_FILE;
    file->size = size;
    file->parent = parent;

    if (size > 0 && data) {
        file->data = (uint8_t*)kmalloc(size + 1);
        if (file->data) {
            memcpy(file->data, data, size);
            file->data[size] = 0;
        }
    }

    parent->children[parent->child_count++] = file;
    return file;
}

vfs_node_t* vfs_lookup(vfs_node_t* relative_to, const char* path) {
    if (!path || strlen(path) == 0) return relative_to;
    vfs_node_t* curr = (path[0] == '/') ? &root_node : (relative_to ? relative_to : &root_node);

    char tmp[128];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    char* token = strtok(tmp, "/");
    while (token && curr) {
        if (strcmp(token, ".") == 0) {
            token = strtok(0, "/");
            continue;
        }
        if (strcmp(token, "..") == 0) {
            curr = curr->parent ? curr->parent : &root_node;
            token = strtok(0, "/");
            continue;
        }

        vfs_node_t* found = 0;
        for (uint32_t i = 0; i < curr->child_count; i++) {
            if (strcmp(curr->children[i]->name, token) == 0) {
                found = curr->children[i];
                break;
            }
        }
        curr = found;
        token = strtok(0, "/");
    }

    return curr;
}

int vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (!node || node->type != VFS_FILE || !buffer) return -1;
    if (offset >= node->size) return 0;

    uint32_t read_bytes = size;
    if (offset + read_bytes > node->size) {
        read_bytes = node->size - offset;
    }

    if (node->data) {
        memcpy(buffer, node->data + offset, read_bytes);
    }
    return read_bytes;
}

int vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, const uint8_t* buffer) {
    if (!node || node->type != VFS_FILE || !buffer) return -1;

    uint32_t new_size = offset + size;
    if (new_size > node->size) {
        uint8_t* new_data = (uint8_t*)kmalloc(new_size + 1);
        if (!new_data) return -1;

        if (node->data) {
            memcpy(new_data, node->data, node->size);
            kfree(node->data);
        }
        node->data = new_data;
        node->size = new_size;
        node->data[new_size] = 0;
    }

    memcpy(node->data + offset, buffer, size);
    return size;
}
