#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

typedef _Bool bool;

#define TRUE 1
#define FALSE 0

#if defined(_WIN64) || defined(_WIN32)
#    define PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(linux)
#    define PLATFORM_LINUX 1
#else
#    error platform not suported
#endif

#define DEBUG_BREAK __builtin_trap()

//    ██       ██████   ██████   ██████  ███████ ██████
//    ██      ██    ██ ██       ██       ██      ██   ██
//    ██      ██    ██ ██   ███ ██   ███ █████   ██████
//    ██      ██    ██ ██    ██ ██    ██ ██      ██   ██
//    ███████  ██████   ██████   ██████  ███████ ██   ██
//
//

#define LOGE(msg_fmt, ...) log_stdout("\033[31m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#define LOGW(msg_fmt, ...) log_stdout("\033[33m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#define LOGI(msg_fmt, ...) log_stdout("\033[37m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#define LOGD(msg_fmt, ...) log_stdout("\033[34m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#define LOGT(msg_fmt, ...) log_stdout("\033[32m" msg_fmt "\033[0m\n", ##__VA_ARGS__)

void log_stdout(const char* msg_fmt, ...) {
    va_list args;
    va_start(args, msg_fmt);
    vfprintf(stdout, msg_fmt, args);
    va_end(args);
}

//     █████  ███████ ███████ ███████ ██████  ████████ ███████
//    ██   ██ ██      ██      ██      ██   ██    ██    ██
//    ███████ ███████ ███████ █████   ██████     ██    ███████
//    ██   ██      ██      ██ ██      ██   ██    ██         ██
//    ██   ██ ███████ ███████ ███████ ██   ██    ██    ███████
//
//

#define ASSERT(exp)                                               \
    do {                                                          \
        if (exp) {                                                \
        } else {                                                  \
            LOGE("assert:%s -> %s:%i", #exp, __FILE__, __LINE__); \
            DEBUG_BREAK;                                          \
        }                                                         \
    } while (0)

static i32 total = 0;

void* zmalloc(u32 size) {
    void* ptr = malloc(size);
    total += size;
    return ptr;
}
void* zrealloc(void* addr, u32 size, u32 new_size) {
    void* ptr = realloc(addr, new_size);
    total -= size;
    total += new_size;
    return ptr;
}
void zfree(void* addr, u32 size) {
    free(addr);
    total -= size;
}

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

#define memory_allocate(size) _memory_allocate(size, __FILE__, __LINE__)

void memory_init(bool auto_free_memory) {
    ASSERT(ptr_state == 0);
    ptr_state = zmalloc(sizeof(memory_state));
    ptr_state->root = 0;
    ptr_state->allocated_memory = 0;
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
    zfree(ptr_state, sizeof(memory_state));
    LOGT("memory_shutdown");
}

void* _memory_allocate(u32 size, const char* file, i32 line) {
    ASSERT(ptr_state != 0 && size != 0);
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
    return node->addr;
}

void memory_free(void* addr) {
    ASSERT(ptr_state != 0 && addr != 0);
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
            //swap addr
                void*temp_addr=root->addr;
                root->addr = node->addr;
                node->addr=temp_addr;
                //swap size
                u64 temp_size=root->size;
                root->size = node->size;
                node->size=temp_size;

                root->file = node->file;
                root->line = node->line;

                //update root
                root = node;
        } else {
            root = ((u64)root->addr > (u64)addr) ? root->left : root->right;
        }
    }
    ASSERT(found != FALSE);
}

void* memory_reallocate(void* addr, u64 size) {
    ASSERT(ptr_state != 0 && size != 0);
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

    void* realloc_addr = zrealloc(node->addr, node->size,size);
    ptr_state->allocated_memory -= node->size;
    ptr_state->allocated_memory += size;

    if ((u64)node->addr != (u64)realloc_addr) {
        memory_node *realloc_node=zmalloc(sizeof(memory_node));
        realloc_node->addr = realloc_addr;
        realloc_node->size=size;
        realloc_node->file=node->file;
        realloc_node->line=node->line;
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
                    zfree(root,sizeof(memory_node));
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
                //swap addr
                void*temp_addr=root->addr;
                root->addr = node->addr;
                node->addr=temp_addr;
                //swap size
                u64 temp_size=root->size;
                root->size = node->size;
                node->size=temp_size;

                root->file = node->file;
                root->line = node->line;

                //update root
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
    }
    else{
        node->size=size;
    }
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
    memory_node* node = zmalloc(sizeof(memory_node));
    node->addr = zmalloc(size);
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
    zfree(node->addr, node->size);
    zfree(node,sizeof(memory_node));
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

void check_red_black_tree(memory_node* node, int blacks) {
    if (node == 0)
        return;
    if (node->color == BLACK) {
        blacks += 1;
    }
    if (node->left == 0 && node->right == 0) {
        printf("%d,", blacks);
        return;
    }
    if (node->color == RED && ((node->left && node->left->color == RED) || (node->right && node->right->color == RED))) {
        LOGE("RED_RED error");
    }
    check_red_black_tree(node->right, blacks);
    check_red_black_tree(node->left, blacks);
}

void print_red_black_tree(memory_node* node, int level, int left) {
    if (level) {
        int k = level;
        printf("|");
        while (k) {
            printf("  |");
            k--;
        }
        printf("--");
        if (!node) {
            if (left) {
                LOGD("L_NULL");
            } else {
                LOGD("R_NULL");
            }
            return;
        }
        if (node->color == RED) {
            if (left) {
                LOGE("L_%llu_%llu_RED", (u64)node->addr, node->size);
            } else {
                LOGE("R_%llu_%llu_RED", (u64)node->addr, node->size);
            }

        } else {
            if (left) {
                LOGD("L_%llu_%llu_BLACK", (u64)node->addr, node->size);
            } else {
                LOGD("R_%llu_%llu_BLACK", (u64)node->addr, node->size);
            }
        }

    } else {
        if(!node)return;
        LOGD("%llu_%llu_BLACK ROOT", (u64)node->addr, node->size);
    }
    print_red_black_tree(node->right, level + 1, 0);
    print_red_black_tree(node->left, level + 1, 1);
}

#define RB                                        \
    print_red_black_tree(ptr_state->root, 0, -1); \
    check_red_black_tree(ptr_state->root, 0);     \
    printf("\n")

#define VAL 100

static void* ptr[VAL];

int main() {
    memory_init(TRUE);
    for (int i = 0; i < VAL; ++i) {
        ptr[i] = memory_allocate(2*(i+1));
        RB;
        LOGT("%i", total);
    }
    LOGT("REALLOCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
    for(int i=0;i<VAL;++i){
        if(i&1){
            ptr[i]=memory_reallocate(ptr[i],16*i);
            RB;
        }
    }
    LOGT("FREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
    for(int i=0;i<VAL;++i){
        memory_free(ptr[i]);
        RB;
        LOGT("%i", total);
    }
    memory_shutdown();
    LOGT("%i", total);
}