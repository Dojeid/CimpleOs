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

    // Normalize path tokens
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

int vfs_get_path(vfs_node_t* node, char* buffer, size_t max_len) {
    if (!node || !buffer || max_len == 0) return -1;
    if (node == &root_node || !node->parent) {
        strncpy(buffer, "/", max_len - 1);
        buffer[max_len - 1] = 0;
        return 0;
    }

    char stack[16][VFS_MAX_FILENAME];
    int depth = 0;

    vfs_node_t* curr = node;
    while (curr && curr != &root_node && depth < 16) {
        strncpy(stack[depth++], curr->name, VFS_MAX_FILENAME - 1);
        curr = curr->parent;
    }

    buffer[0] = '\0';
    for (int i = depth - 1; i >= 0; i--) {
        strcat(buffer, "/");
        strcat(buffer, stack[i]);
    }
    return 0;
}

vfs_node_t* vfs_lookup(vfs_node_t* relative_to, const char* path) {
    char resolved[256];
    char cwd_str[128] = "/";

    if (relative_to) {
        vfs_get_path(relative_to, cwd_str, sizeof(cwd_str));
    }

    vfs_resolve_path(cwd_str, path, resolved, sizeof(resolved));

    if (strcmp(resolved, "/") == 0) return &root_node;

    vfs_node_t* curr = &root_node;
    int idx = 1;
    int len = strlen(resolved);

    while (idx < len && curr) {
        char token[64];
        int t_idx = 0;
        while (idx < len && resolved[idx] != '/') {
            if (t_idx < 63) token[t_idx++] = resolved[idx];
            idx++;
        }
        token[t_idx] = '\0';
        if (resolved[idx] == '/') idx++;

        vfs_node_t* found = 0;
        for (uint32_t i = 0; i < curr->child_count; i++) {
            if (strcmp(curr->children[i]->name, token) == 0) {
                found = curr->children[i];
                break;
            }
        }
        curr = found;
    }

    return curr;
}

int vfs_remove(vfs_node_t* parent, const char* name) {
    if (!parent || parent->type != VFS_DIRECTORY || !name) return -1;

    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            vfs_node_t* child = parent->children[i];

            if (child->data) {
                kfree(child->data);
            }
            kfree(child);

            for (uint32_t j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;
            return 0;
        }
    }
    return -1;
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
