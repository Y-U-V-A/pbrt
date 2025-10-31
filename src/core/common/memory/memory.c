#include "memory.h"
#undef malloc
#undef free
#undef realloc

#include "logger.h"
#include <stdlib.h>
#include "zmutex.h"



//    ███    ███ ███████ ███    ███  ██████  ██████  ██    ██
//    ████  ████ ██      ████  ████ ██    ██ ██   ██  ██  ██
//    ██ ████ ██ █████   ██ ████ ██ ██    ██ ██████    ████
//    ██  ██  ██ ██      ██  ██  ██ ██    ██ ██   ██    ██
//    ██      ██ ███████ ██      ██  ██████  ██   ██    ██
//
//

/**
 * this implementation of memory tracks no of memory allocations using red black trees to detect memory leaks
 */

typedef enum memory_node_color {
    RED,
    BLACK,
} memory_node_color;

typedef struct memory_node {
    void* addr;
    u64 size;
    const char* file;
    i32 line;
    memory_node_color color;
    struct memory_node* parent;
    struct memory_node* left;
    struct memory_node* right;
} memory_node;

typedef struct memory_state {
    memory_node* root;
    u64 allocated_memory;
    zmutex mutex;
} memory_state;

static memory_state* ptr_state;
// when auto_free is set it free's all unfreed memory only during memory_shudown
static bool auto_free;

memory_node* memory_node_create(u64 size, const char* file, i32 line);
void memory_node_destroy(memory_node* node);
void memory_node_right_rotate(memory_node* node);
void memory_node_left_rotate(memory_node* node);
void memory_node_insert_fixup(memory_node* node);
void memory_node_delete_fixup(memory_node* node);

void memory_init(bool auto_free_memory) {
    ASSERT(ptr_state == 0);
    ptr_state = malloc(sizeof(memory_state));
    ptr_state->root = 0;
    ptr_state->allocated_memory = 0;
    zmutex_create(&ptr_state->mutex);
    auto_free = auto_free_memory;
    LOGT("memory_init");
}

void memory_shutdown() {
    ASSERT(ptr_state != 0);
    if (ptr_state->allocated_memory != 0) {
        LOGE("memory_leaks");
        // using morris tree traversal to print memory leaks
        memory_node* root = ptr_state->root;
        while (root) {
            if (root->left) {
                memory_node* temp = root->left;
                while (temp->right && temp->right != root) {
                    temp = temp->right;
                }
                if (temp->right == 0) {
                    LOGE("%llu bytes %s:%i", root->size, root->file, root->line);
                    temp->right = root;
                    root = root->left;
                } else {
                    temp->right = 0;
                    root = root->right;
                }
            } else {
                LOGE("%llu bytes %s:%i", root->size, root->file, root->line);
                root = root->right;
            }
        }
        if (auto_free) {
            memory_node_destroy(ptr_state->root);
        }
    }
    zmutex_destroy(&ptr_state->mutex);
    free(ptr_state);
    LOGT("memory_shutdown");
}

void* _memory_allocate(u32 size, const char* file, i32 line) {
    ASSERT(ptr_state != 0 && size != 0);
    zmutex_lock(&ptr_state->mutex);
    memory_node* node = memory_node_create(size, file, line);
    if (ptr_state->root == 0) {
        ptr_state->root = node;
        ptr_state->root->color = BLACK;
    } else {
        memory_node* root = ptr_state->root;
        while (TRUE) {
            if ((u64)root->addr > (u64)node->addr) {
                if (root->left == 0) {
                    root->left = node;
                    node->parent = root;
                    break;
                }
                root = root->left;
            } else {
                if (root->right == 0) {
                    root->right = node;
                    node->parent = root;
                    break;
                }
                root = root->right;
            }
        }
        memory_node_insert_fixup(node);
    }
    zmutex_unlock(&ptr_state->mutex);
    return node->addr;
}

void memory_free(const void* addr) {
    ASSERT(ptr_state != 0 && addr != 0);
    zmutex_lock(&ptr_state->mutex);
    memory_node* root = ptr_state->root;
    bool found = FALSE;
    while (root) {
        if ((u64)root->addr == (u64)addr) {
            if (root->left == 0 && root->right == 0) {
                if (root->color == BLACK) {
                    memory_node_delete_fixup(root);
                }
                if (root->parent == 0) {
                    ptr_state->root = 0;
                } else {
                    if (root->parent->left == root) {
                        root->parent->left = 0;
                    } else {
                        root->parent->right = 0;
                    }
                }
                memory_node_destroy(root);
                found = TRUE;
                break;
            }
            memory_node* node;
            if (root->left) {
                node = root->left;
                while (node->right) {
                    node = node->right;
                }
            } else {
                node = root->right;
                while (node->left) {
                    node = node->left;
                }
            }
            // swap addr
            void* temp_addr = root->addr;
            root->addr = node->addr;
            node->addr = temp_addr;
            // swap size
            u64 temp_size = root->size;
            root->size = node->size;
            node->size = temp_size;

            root->file = node->file;
            root->line = node->line;

            // update root
            root = node;
        } else {
            root = ((u64)root->addr > (u64)addr) ? root->left : root->right;
        }
    }
    ASSERT(found != FALSE);
    zmutex_unlock(&ptr_state->mutex);
}

void* memory_reallocate(const void* addr, u64 size) {
    ASSERT(ptr_state != 0 && size != 0);
    zmutex_lock(&ptr_state->mutex);
    memory_node* node = ptr_state->root;
    bool found = FALSE;
    while (node) {
        if ((u64)node->addr == (u64)addr) {
            found = TRUE;
            break;
        } else {
            node = ((u64)node->addr > (u64)addr) ? node->left : node->right;
        }
    }
    ASSERT(found != FALSE);

    void* realloc_addr = realloc(node->addr, size);
    ptr_state->allocated_memory -= node->size;
    ptr_state->allocated_memory += size;

    if ((u64)node->addr != (u64)realloc_addr) {
        memory_node* realloc_node = malloc(sizeof(memory_node));
        realloc_node->addr = realloc_addr;
        realloc_node->size = size;
        realloc_node->file = node->file;
        realloc_node->line = node->line;
        realloc_node->color = RED;
        realloc_node->parent = 0;
        realloc_node->left = 0;
        realloc_node->right = 0;

        memory_node* root = node;
        while (root) {
            if ((u64)root->addr == (u64)addr) {
                if (root->left == 0 && root->right == 0) {
                    if (root->color == BLACK) {
                        memory_node_delete_fixup(root);
                    }
                    if (root->parent == 0) {
                        ptr_state->root = 0;
                    } else {
                        if (root->parent->left == root) {
                            root->parent->left = 0;
                        } else {
                            root->parent->right = 0;
                        }
                    }
                    free(root);
                    break;
                }
                memory_node* node;
                if (root->left) {
                    node = root->left;
                    while (node->right) {
                        node = node->right;
                    }
                } else {
                    node = root->right;
                    while (node->left) {
                        node = node->left;
                    }
                }
                // swap addr
                void* temp_addr = root->addr;
                root->addr = node->addr;
                node->addr = temp_addr;
                // swap size
                u64 temp_size = root->size;
                root->size = node->size;
                node->size = temp_size;

                root->file = node->file;
                root->line = node->line;

                // update root
                root = node;
            } else {
                root = ((u64)root->addr > (u64)addr) ? root->left : root->right;
            }
        }
        root = ptr_state->root;
        if (root == 0) {
            ptr_state->root = realloc_node;
            ptr_state->root->color = BLACK;
        } else {
            while (TRUE) {
                if ((u64)root->addr > (u64)realloc_node->addr) {
                    if (root->left == 0) {
                        root->left = realloc_node;
                        realloc_node->parent = root;
                        break;
                    }
                    root = root->left;
                } else {
                    if (root->right == 0) {
                        root->right = realloc_node;
                        realloc_node->parent = root;
                        break;
                    }
                    root = root->right;
                }
            }
            memory_node_insert_fixup(realloc_node);
        }
    } else {
        node->size = size;
    }
    zmutex_unlock(&ptr_state->mutex);
    return realloc_addr;
}

//    ██   ██ ███████ ██      ██████  ███████ ██████  ███████
//    ██   ██ ██      ██      ██   ██ ██      ██   ██ ██
//    ███████ █████   ██      ██████  █████   ██████  ███████
//    ██   ██ ██      ██      ██      ██      ██   ██      ██
//    ██   ██ ███████ ███████ ██      ███████ ██   ██ ███████
//
//

memory_node* memory_node_create(u64 size, const char* file, i32 line) {
    memory_node* node = malloc(sizeof(memory_node));
    node->addr = malloc(size);
    node->size = size;
    node->file = file;
    node->line = line;
    node->color = RED;
    node->parent = 0;
    node->left = 0;
    node->right = 0;
    ptr_state->allocated_memory += size;
    return node;
}

void memory_node_destroy(memory_node* node) {
    if (!node)
        return;
    memory_node_destroy(node->left);
    memory_node_destroy(node->right);
    ptr_state->allocated_memory -= node->size;
    free(node->addr);
    free(node);
}

void memory_node_right_rotate(memory_node* node) {
    if (node == 0 || node->left == 0) {
        return;
    }
    memory_node* parent = node->parent;
    memory_node* left_node = node->left;

    if (parent) {
        if (parent->left == node) {
            parent->left = left_node;
        } else {
            parent->right = left_node;
        }
    } else {
        ptr_state->root = left_node;
    }
    left_node->parent = parent;

    node->left = left_node->right;
    if (left_node->right) {
        left_node->right->parent = node;
    }

    left_node->right = node;
    node->parent = left_node;
}

void memory_node_left_rotate(memory_node* node) {
    if (node == 0 || node->right == 0) {
        return;
    }
    memory_node* parent = node->parent;
    memory_node* right_node = node->right;

    if (parent) {
        if (parent->left == node) {
            parent->left = right_node;
        } else {
            parent->right = right_node;
        }
    } else {
        ptr_state->root = right_node;
    }
    right_node->parent = parent;

    node->right = right_node->left;
    if (right_node->left) {
        right_node->left->parent = node;
    }

    right_node->left = node;
    node->parent = right_node;
}

void memory_node_insert_fixup(memory_node* node) {
    while (node->parent && node->parent->color == RED) {
        // get the uncle node
        memory_node* uncle;
        if (node->parent->parent->left == node->parent) {
            uncle = node->parent->parent->right;
        } else {
            uncle = node->parent->parent->left;
        }
        // if uncle node is red
        if (uncle && uncle->color == RED) {
            node->parent->color = BLACK;
            uncle->color = BLACK;
            node->parent->parent->color = RED;
            node = node->parent->parent;
        } else {
            // if uncle node is null or black
            if (node->parent->parent->left == node->parent) {
                if (node->parent->right == node) {
                    node = node->parent;
                    memory_node_left_rotate(node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                memory_node_right_rotate(node->parent->parent);
            } else {
                if (node->parent->left == node) {
                    node = node->parent;
                    memory_node_right_rotate(node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                memory_node_left_rotate(node->parent->parent);
            }
            break;
        }
    }
    if (node->parent == 0) {
        node->color = BLACK;
    }
}

void memory_node_delete_fixup(memory_node* node) {
    while (node->parent && node->color == BLACK) {
        if (node->parent->left == node) {
            memory_node* sibling = node->parent->right;
            if (sibling && sibling->color == RED) {
                sibling->color = BLACK;
                node->parent->color = RED;
                memory_node_left_rotate(node->parent);
                sibling = node->parent->right;
            }
            if (sibling == 0 || ((sibling->left == 0 || sibling->left->color == BLACK) && (sibling->right == 0 || sibling->right->color == BLACK))) {
                if (sibling) {
                    sibling->color = RED;
                }
                node = node->parent;
            } else {
                if (sibling->right == 0 || sibling->right->color == BLACK) {
                    sibling->left->color = BLACK;
                    sibling->color = RED;
                    memory_node_right_rotate(sibling);
                    sibling = node->parent->right;
                }
                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                sibling->right->color = BLACK;
                memory_node_left_rotate(node->parent);
                node = ptr_state->root;
            }
        } else {
            memory_node* sibling = node->parent->left;
            if (sibling && sibling->color == RED) {
                sibling->color = BLACK;
                node->parent->color = RED;
                memory_node_right_rotate(node->parent);
                sibling = node->parent->left;
            }
            if (sibling == 0 || ((sibling->left == 0 || sibling->left->color == BLACK) && (sibling->right == 0 || sibling->right->color == BLACK))) {
                if (sibling) {
                    sibling->color = RED;
                }
                node = node->parent;
            } else {
                if (sibling->left == 0 || sibling->left->color == BLACK) {
                    sibling->right->color = BLACK;
                    sibling->color = RED;
                    memory_node_left_rotate(sibling);
                    sibling = node->parent->left;
                }
                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                sibling->left->color = BLACK;
                memory_node_right_rotate(node->parent);
                node = ptr_state->root;
            }
        }
    }
    node->color = BLACK;
}