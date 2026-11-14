#pragma once

#include "IDataStructure.hpp"
#include <utility>
#include <algorithm>
#include <vector>

namespace adaptive_ds {

template<typename T, 
         typename Compare = std::less<T>, 
         typename Hash = std::hash<T>, 
         typename KeyEqual = std::equal_to<T>>
class AVLTree : public IDataStructure<T, Compare, Hash, KeyEqual> {
public:
    using Base = IDataStructure<T, Compare, Hash, KeyEqual>;

    struct AVLNode {
        T value;
        int height{1};
        AVLNode* left{nullptr};
        AVLNode* right{nullptr};

        template<typename... Args>
        explicit AVLNode(Args&&... args)
            : value(std::forward<Args>(args)...), height(1), left(nullptr), right(nullptr) {}
    };

    AVLTree() : root_(nullptr), size_(0) {}

    ~AVLTree() override {
        clear_tree(root_);
        root_ = nullptr;
        size_ = 0;
    }

    AVLTree(const AVLTree& other) : root_(nullptr), size_(other.size_) {
        root_ = clone_tree(other.root_);
    }

    AVLTree& operator=(const AVLTree& other) {
        if (this != &other) {
            AVLTree temp(other);
            swap(temp);
        }
        return *this;
    }

    AVLTree(AVLTree&& other) noexcept
        : root_(other.root_), size_(other.size_) {
        other.root_ = nullptr;
        other.size_ = 0;
    }

    AVLTree& operator=(AVLTree&& other) noexcept {
        if (this != &other) {
            clear_tree(root_);
            root_ = other.root_;
            size_ = other.size_;
            other.root_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void swap(AVLTree& other) noexcept {
        std::swap(root_, other.root_);
        std::swap(size_, other.size_);
    }

    bool insert(const T& val) override {
        bool inserted = false;
        root_ = insert_recursive(root_, val, inserted);
        if (inserted) {
            ++size_;
        }
        return inserted;
    }

    bool insert(T&& val) override {
        bool inserted = false;
        root_ = insert_recursive_move(root_, std::move(val), inserted);
        if (inserted) {
            ++size_;
        }
        return inserted;
    }

    bool erase(const T& val) override {
        bool deleted = false;
        root_ = erase_recursive(root_, val, deleted);
        if (deleted) {
            --size_;
        }
        return deleted;
    }

    bool contains(const T& val) const override {
        return find(val) != nullptr;
    }

    const T* find(const T& val) const override {
        Compare comp;
        AVLNode* curr = root_;
        while (curr != nullptr) {
            if (comp(val, curr->value)) {
                curr = curr->left;
            } else if (comp(curr->value, val)) {
                curr = curr->right;
            } else {
                return &(curr->value);
            }
        }
        return nullptr;
    }

    std::vector<T> range(const T& low, const T& high) const override {
        std::vector<T> results;
        Compare comp;
        range_recursive(root_, low, high, comp, results);
        return results;
    }

    size_t size() const noexcept override {
        return size_;
    }

    bool empty() const noexcept override {
        return size_ == 0;
    }

    void clear() override {
        clear_tree(root_);
        root_ = nullptr;
        size_ = 0;
    }

    std::vector<T> to_vector() const override {
        std::vector<T> results;
        results.reserve(size_);
        inorder_collect(root_, results);
        return results;
    }

    StructureType type() const noexcept override {
        return StructureType::AVLTree;
    }

    const char* type_name() const noexcept override {
        return "AVLTree";
    }

    size_t memory_bytes() const noexcept override {
        return sizeof(*this) + size_ * sizeof(AVLNode);
    }

    std::unique_ptr<Base> clone() const override {
        return std::make_unique<AVLTree>(*this);
    }

    int tree_height() const noexcept {
        return get_height(root_);
    }

    bool is_balanced() const noexcept {
        return check_balanced(root_);
    }

private:
    int get_height(AVLNode* node) const noexcept {
        return node ? node->height : 0;
    }

    int get_balance_factor(AVLNode* node) const noexcept {
        return node ? (get_height(node->left) - get_height(node->right)) : 0;
    }

    void update_height(AVLNode* node) noexcept {
        if (node) {
            node->height = 1 + std::max(get_height(node->left), get_height(node->right));
        }
    }

    AVLNode* rotate_right(AVLNode* y) {
        AVLNode* x = y->left;
        AVLNode* T2 = x->right;

        x->right = y;
        y->left = T2;

        update_height(y);
        update_height(x);

        return x;
    }

    AVLNode* rotate_left(AVLNode* x) {
        AVLNode* y = x->right;
        AVLNode* T2 = y->left;

        y->left = x;
        x->right = T2;

        update_height(x);
        update_height(y);

        return y;
    }

    AVLNode* balance(AVLNode* node) {
        if (!node) return nullptr;
        update_height(node);

        int bf = get_balance_factor(node);

        // Left heavy
        if (bf > 1) {
            if (get_balance_factor(node->left) < 0) {
                node->left = rotate_left(node->left); // Left-Right case
            }
            return rotate_right(node); // Left-Left case
        }

        // Right heavy
        if (bf < -1) {
            if (get_balance_factor(node->right) > 0) {
                node->right = rotate_right(node->right); // Right-Left case
            }
            return rotate_left(node); // Right-Right case
        }

        return node;
    }

    AVLNode* insert_recursive(AVLNode* node, const T& val, bool& inserted) {
        if (!node) {
            inserted = true;
            return new AVLNode(val);
        }

        Compare comp;
        if (comp(val, node->value)) {
            node->left = insert_recursive(node->left, val, inserted);
        } else if (comp(node->value, val)) {
            node->right = insert_recursive(node->right, val, inserted);
        } else {
            inserted = false;
            return node; // Duplicate
        }

        return balance(node);
    }

    AVLNode* insert_recursive_move(AVLNode* node, T&& val, bool& inserted) {
        if (!node) {
            inserted = true;
            return new AVLNode(std::move(val));
        }

        Compare comp;
        if (comp(val, node->value)) {
            node->left = insert_recursive_move(node->left, std::move(val), inserted);
        } else if (comp(node->value, val)) {
            node->right = insert_recursive_move(node->right, std::move(val), inserted);
        } else {
            inserted = false;
            return node; // Duplicate
        }

        return balance(node);
    }

    AVLNode* find_min(AVLNode* node) const noexcept {
        AVLNode* curr = node;
        while (curr && curr->left != nullptr) {
            curr = curr->left;
        }
        return curr;
    }

    AVLNode* erase_recursive(AVLNode* node, const T& val, bool& deleted) {
        if (!node) {
            deleted = false;
            return nullptr;
        }

        Compare comp;
        if (comp(val, node->value)) {
            node->left = erase_recursive(node->left, val, deleted);
        } else if (comp(node->value, val)) {
            node->right = erase_recursive(node->right, val, deleted);
        } else {
            deleted = true;
            // Node found
            if (!node->left || !node->right) {
                AVLNode* temp = node->left ? node->left : node->right;
                if (!temp) {
                    // No children
                    delete node;
                    return nullptr;
                } else {
                    // One child
                    *node = *temp;
                    delete temp;
                }
            } else {
                // Two children: get in-order successor
                AVLNode* successor = find_min(node->right);
                node->value = successor->value;
                bool dummy_deleted = false;
                node->right = erase_recursive(node->right, successor->value, dummy_deleted);
            }
        }

        return balance(node);
    }

    void range_recursive(AVLNode* node, const T& low, const T& high, 
                         const Compare& comp, std::vector<T>& results) const {
        if (!node) return;

        // If low < node->value, search left
        if (comp(low, node->value)) {
            range_recursive(node->left, low, high, comp, results);
        }

        // If low <= node->value && node->value <= high
        if (!comp(node->value, low) && !comp(high, node->value)) {
            results.push_back(node->value);
        }

        // If node->value < high, search right
        if (comp(node->value, high)) {
            range_recursive(node->right, low, high, comp, results);
        }
    }

    void inorder_collect(AVLNode* node, std::vector<T>& results) const {
        if (!node) return;
        inorder_collect(node->left, results);
        results.push_back(node->value);
        inorder_collect(node->right, results);
    }

    void clear_tree(AVLNode* node) {
        if (!node) return;
        clear_tree(node->left);
        clear_tree(node->right);
        delete node;
    }

    AVLNode* clone_tree(AVLNode* node) const {
        if (!node) return nullptr;
        AVLNode* copy_node = new AVLNode(node->value);
        copy_node->height = node->height;
        copy_node->left = clone_tree(node->left);
        copy_node->right = clone_tree(node->right);
        return copy_node;
    }

    bool check_balanced(AVLNode* node) const noexcept {
        if (!node) return true;
        int bf = get_balance_factor(node);
        if (bf < -1 || bf > 1) return false;
        return check_balanced(node->left) && check_balanced(node->right);
    }

    AVLNode* root_{nullptr};
    size_t size_{0};
};

} // namespace adaptive_ds
