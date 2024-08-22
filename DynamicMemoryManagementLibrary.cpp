#include <iostream>
#include <cstdlib>
#include <cstring>

// Memory block structure for the linked list
struct Block {
    size_t size;        // Size of the block
    bool free;          // Is this block free?
    Block* next;        // Pointer to the next block
    Block* prev;        // Pointer to the previous block
};

// Binary tree node structure for managing blocks by size
struct TreeNode {
    Block* block;
    TreeNode* left;
    TreeNode* right;
};

// Memory Manager class
class MemoryManager {
private:
    Block* freeList;           // Head of the free list
    TreeNode* treeRoot;        // Root of the binary tree
    Block* memoryPool;         // Start of the memory pool

    void* pool;                // The actual memory pool
    size_t poolSize;

    // Private methods
    TreeNode* insertBlock(TreeNode* node, Block* block) {
        if (!node) {
            node = new TreeNode();
            node->block = block;
            node->left = node->right = nullptr;
        } else if (block->size < node->block->size) {
            node->left = insertBlock(node->left, block);
        } else {
            node->right = insertBlock(node->right, block);
        }
        return node;
    }

    TreeNode* findBestFit(TreeNode* node, size_t size) {
        if (!node) return nullptr;
        if (node->block->size >= size && node->block->free) {
            // Check the left subtree for a better fit
            TreeNode* leftBest = findBestFit(node->left, size);
            return (leftBest && leftBest->block->size < node->block->size) ? leftBest : node;
        }
        return findBestFit(node->right, size);
    }

    TreeNode* removeBlock(TreeNode* root, Block* block) {
        if (!root) return root;

        if (block->size < root->block->size) {
            root->left = removeBlock(root->left, block);
        } else if (block->size > root->block->size) {
            root->right = removeBlock(root->right, block);
        } else {
            if (!root->left) {
                TreeNode* temp = root->right;
                delete root;
                return temp;
            } else if (!root->right) {
                TreeNode* temp = root->left;
                delete root;
                return temp;
            }

            TreeNode* temp = findMin(root->right);
            root->block = temp->block;
            root->right = removeBlock(root->right, temp->block);
        }
        return root;
    }

    TreeNode* findMin(TreeNode* node) {
        TreeNode* current = node;
        while (current && current->left != nullptr)
            current = current->left;
        return current;
    }

    void coalesce(Block* block) {
        if (block->next && block->next->free) {
            block->size += block->next->size + sizeof(Block);
            block->next = block->next->next;
            if (block->next) {
                block->next->prev = block;
            }
        }
        if (block->prev && block->prev->free) {
            block->prev->size += block->size + sizeof(Block);
            block->prev->next = block->next;
            if (block->next) {
                block->next->prev = block->prev;
            }
        }
    }

public:
    MemoryManager(size_t size) : poolSize(size), freeList(nullptr), treeRoot(nullptr) {
        pool = malloc(size);
        if (!pool) {
            std::cerr << "Memory allocation failed!" << std::endl;
            exit(1);
        }

        memoryPool = static_cast<Block*>(pool);
        memoryPool->size = size - sizeof(Block);
        memoryPool->free = true;
        memoryPool->next = memoryPool->prev = nullptr;

        freeList = memoryPool;
        treeRoot = insertBlock(treeRoot, memoryPool);
    }

    ~MemoryManager() {
        free(pool);
        // Recursively delete binary tree nodes
        deleteTree(treeRoot);
    }

    void deleteTree(TreeNode* node) {
        if (!node) return;
        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

    void* mymalloc(size_t size) {
        TreeNode* bestFitNode = findBestFit(treeRoot, size);
        if (!bestFitNode) {
            std::cerr << "Memory allocation failed: insufficient memory!" << std::endl;
            return nullptr;
        }

        Block* block = bestFitNode->block;
        removeBlock(treeRoot, block);

        if (block->size > size + sizeof(Block)) {
            Block* newBlock = reinterpret_cast<Block*>(reinterpret_cast<char*>(block) + sizeof(Block) + size);
            newBlock->size = block->size - size - sizeof(Block);
            newBlock->free = true;
            newBlock->next = block->next;
            newBlock->prev = block;

            if (block->next) {
                block->next->prev = newBlock;
            }

            block->size = size;
            block->next = newBlock;

            insertBlock(treeRoot, newBlock);
        }

        block->free = false;
        return reinterpret_cast<void*>(reinterpret_cast<char*>(block) + sizeof(Block));
    }

    void myfree(void* ptr) {
        if (!ptr) return;

        Block* block = reinterpret_cast<Block*>(reinterpret_cast<char*>(ptr) - sizeof(Block));
        block->free = true;

        coalesce(block);

        insertBlock(treeRoot, block);
    }
};

// Test the MemoryManager
int main() {
    size_t poolSize = 2048; // 2 KB memory pool
    MemoryManager memoryManager(poolSize);

    // Allocate memory
    void* ptr1 = memoryManager.mymalloc(200);
    void* ptr2 = memoryManager.mymalloc(300);
    void* ptr3 = memoryManager.mymalloc(100);

    // Use the allocated memory
    std::memset(ptr1, 0, 200);
    std::memset(ptr2, 0, 300);
    std::memset(ptr3, 0, 100);

    // Free memory
    memoryManager.myfree(ptr2);
    memoryManager.myfree(ptr1);
    memoryManager.myfree(ptr3);

    return 0;
}
