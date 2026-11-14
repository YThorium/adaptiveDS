#pragma once

#include "IDataStructure.hpp"
#include <utility>
#include <algorithm>

namespace adaptive_ds {

template<typename T, 
         typename Compare = std::less<T>, 
         typename Hash = std::hash<T>, 
         typename KeyEqual = std::equal_to<T>>
class DoublyLinkedList : public IDataStructure<T, Compare, Hash, KeyEqual> {
public:
    using Base = IDataStructure<T, Compare, Hash, KeyEqual>;

    struct Node {
        T value;
        Node* prev{nullptr};
        Node* next{nullptr};

        template<typename... Args>
        explicit Node(Args&&... args) : value(std::forward<Args>(args)...) {}
    };

    DoublyLinkedList() : size_(0) {
        init_sentinels();
    }

    ~DoublyLinkedList() override {
        clear_internal();
        delete head_;
        delete tail_;
        head_ = nullptr;
        tail_ = nullptr;
    }

    DoublyLinkedList(const DoublyLinkedList& other) : size_(0) {
        init_sentinels();
        Node* curr = other.head_->next;
        while (curr != other.tail_) {
            push_back_internal(curr->value);
            curr = curr->next;
        }
    }

    DoublyLinkedList& operator=(const DoublyLinkedList& other) {
        if (this != &other) {
            DoublyLinkedList temp(other);
            swap(temp);
        }
        return *this;
    }

    DoublyLinkedList(DoublyLinkedList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.size_ = 0;
        other.init_sentinels();
    }

    DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept {
        if (this != &other) {
            clear_internal();
            delete head_;
            delete tail_;

            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;

            other.size_ = 0;
            other.init_sentinels();
        }
        return *this;
    }

    void swap(DoublyLinkedList& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }

    bool insert(const T& val) override {
        if (contains(val)) {
            return false;
        }
        push_back_internal(val);
        return true;
    }

    bool insert(T&& val) override {
        if (contains(val)) {
            return false;
        }
        push_back_internal(std::move(val));
        return true;
    }

    bool erase(const T& val) override {
        KeyEqual eq;
        Node* curr = head_->next;
        while (curr != tail_) {
            if (eq(curr->value, val)) {
                unlink_and_delete(curr);
                return true;
            }
            curr = curr->next;
        }
        return false;
    }

    bool contains(const T& val) const override {
        return find(val) != nullptr;
    }

    const T* find(const T& val) const override {
        KeyEqual eq;
        Node* curr = head_->next;
        while (curr != tail_) {
            if (eq(curr->value, val)) {
                return &(curr->value);
            }
            curr = curr->next;
        }
        return nullptr;
    }

    std::vector<T> range(const T& low, const T& high) const override {
        Compare comp;
        std::vector<T> results;
        Node* curr = head_->next;
        while (curr != tail_) {
            const T& item = curr->value;
            if (!comp(item, low) && !comp(high, item)) {
                results.push_back(item);
            }
            curr = curr->next;
        }
        std::sort(results.begin(), results.end(), comp);
        return results;
    }

    size_t size() const noexcept override {
        return size_;
    }

    bool empty() const noexcept override {
        return size_ == 0;
    }

    void clear() override {
        clear_internal();
    }

    std::vector<T> to_vector() const override {
        std::vector<T> vec;
        vec.reserve(size_);
        Node* curr = head_->next;
        while (curr != tail_) {
            vec.push_back(curr->value);
            curr = curr->next;
        }
        return vec;
    }

    StructureType type() const noexcept override {
        return StructureType::DoublyLinkedList;
    }

    const char* type_name() const noexcept override {
        return "DoublyLinkedList";
    }

    size_t memory_bytes() const noexcept override {
        return sizeof(*this) + (size_ + 2) * sizeof(Node);
    }

    std::unique_ptr<Base> clone() const override {
        return std::make_unique<DoublyLinkedList>(*this);
    }

private:
    void init_sentinels() {
        head_ = new Node();
        tail_ = new Node();
        head_->next = tail_;
        tail_->prev = head_;
    }

    void clear_internal() {
        Node* curr = head_->next;
        while (curr != tail_) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        head_->next = tail_;
        tail_->prev = head_;
        size_ = 0;
    }

    template<typename U>
    void push_back_internal(U&& val) {
        Node* new_node = new Node(std::forward<U>(val));
        Node* prev_node = tail_->prev;

        prev_node->next = new_node;
        new_node->prev = prev_node;
        new_node->next = tail_;
        tail_->prev = new_node;

        ++size_;
    }

    void unlink_and_delete(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
        --size_;
    }

    Node* head_{nullptr};
    Node* tail_{nullptr};
    size_t size_{0};
};

} // namespace adaptive_ds
