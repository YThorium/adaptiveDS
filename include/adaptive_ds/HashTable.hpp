#pragma once

#include "IDataStructure.hpp"
#include <utility>
#include <algorithm>
#include <cassert>

namespace adaptive_ds {

template<typename T, 
         typename Compare = std::less<T>, 
         typename Hash = std::hash<T>, 
         typename KeyEqual = std::equal_to<T>>
class HashTable : public IDataStructure<T, Compare, Hash, KeyEqual> {
public:
    using Base = IDataStructure<T, Compare, Hash, KeyEqual>;

    struct BucketNode {
        T value;
        BucketNode* next{nullptr};

        template<typename... Args>
        explicit BucketNode(BucketNode* nxt, Args&&... args)
            : value(std::forward<Args>(args)...), next(nxt) {}
    };

    explicit HashTable(size_t initial_buckets = 16, double max_load_factor = 0.75)
        : num_buckets_(initial_buckets < 8 ? 8 : initial_buckets),
          size_(0),
          max_load_factor_(max_load_factor),
          buckets_(nullptr) {
        allocate_buckets(num_buckets_);
    }

    ~HashTable() override {
        clear_buckets();
        deallocate_buckets();
    }

    HashTable(const HashTable& other)
        : num_buckets_(other.num_buckets_),
          size_(0),
          max_load_factor_(other.max_load_factor_),
          buckets_(nullptr) {
        allocate_buckets(num_buckets_);
        for (size_t i = 0; i < other.num_buckets_; ++i) {
            BucketNode* curr = other.buckets_[i];
            while (curr != nullptr) {
                insert(curr->value);
                curr = curr->next;
            }
        }
    }

    HashTable& operator=(const HashTable& other) {
        if (this != &other) {
            HashTable temp(other);
            swap(temp);
        }
        return *this;
    }

    HashTable(HashTable&& other) noexcept
        : num_buckets_(other.num_buckets_),
          size_(other.size_),
          max_load_factor_(other.max_load_factor_),
          buckets_(other.buckets_) {
        other.buckets_ = nullptr;
        other.size_ = 0;
        other.num_buckets_ = 0;
    }

    HashTable& operator=(HashTable&& other) noexcept {
        if (this != &other) {
            clear_buckets();
            deallocate_buckets();

            num_buckets_ = other.num_buckets_;
            size_ = other.size_;
            max_load_factor_ = other.max_load_factor_;
            buckets_ = other.buckets_;

            other.buckets_ = nullptr;
            other.size_ = 0;
            other.num_buckets_ = 0;
        }
        return *this;
    }

    void swap(HashTable& other) noexcept {
        std::swap(buckets_, other.buckets_);
        std::swap(num_buckets_, other.num_buckets_);
        std::swap(size_, other.size_);
        std::swap(max_load_factor_, other.max_load_factor_);
    }

    bool insert(const T& val) override {
        return insert_internal(val);
    }

    bool insert(T&& val) override {
        return insert_internal(std::move(val));
    }

    bool erase(const T& val) override {
        if (num_buckets_ == 0 || size_ == 0) return false;

        size_t idx = get_bucket_index(val);
        KeyEqual eq;
        BucketNode* curr = buckets_[idx];
        BucketNode* prev = nullptr;

        while (curr != nullptr) {
            if (eq(curr->value, val)) {
                if (prev == nullptr) {
                    buckets_[idx] = curr->next;
                } else {
                    prev->next = curr->next;
                }
                delete curr;
                --size_;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    bool contains(const T& val) const override {
        return find(val) != nullptr;
    }

    const T* find(const T& val) const override {
        if (num_buckets_ == 0 || size_ == 0) return nullptr;

        size_t idx = get_bucket_index(val);
        KeyEqual eq;
        BucketNode* curr = buckets_[idx];

        while (curr != nullptr) {
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
        results.reserve(size_ > 32 ? 32 : size_);

        for (size_t i = 0; i < num_buckets_; ++i) {
            BucketNode* curr = buckets_[i];
            while (curr != nullptr) {
                const T& item = curr->value;
                if (!comp(item, low) && !comp(high, item)) {
                    results.push_back(item);
                }
                curr = curr->next;
            }
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
        clear_buckets();
    }

    std::vector<T> to_vector() const override {
        std::vector<T> vec;
        vec.reserve(size_);
        for (size_t i = 0; i < num_buckets_; ++i) {
            BucketNode* curr = buckets_[i];
            while (curr != nullptr) {
                vec.push_back(curr->value);
                curr = curr->next;
            }
        }
        return vec;
    }

    StructureType type() const noexcept override {
        return StructureType::HashTable;
    }

    const char* type_name() const noexcept override {
        return "HashTable";
    }

    size_t memory_bytes() const noexcept override {
        return sizeof(*this) + num_buckets_ * sizeof(BucketNode*) + size_ * sizeof(BucketNode);
    }

    std::unique_ptr<Base> clone() const override {
        return std::make_unique<HashTable>(*this);
    }

    size_t bucket_count() const noexcept {
        return num_buckets_;
    }

    double load_factor() const noexcept {
        return num_buckets_ > 0 ? static_cast<double>(size_) / num_buckets_ : 0.0;
    }

private:
    void allocate_buckets(size_t count) {
        num_buckets_ = count;
        buckets_ = new BucketNode*[num_buckets_]();
    }

    void deallocate_buckets() {
        delete[] buckets_;
        buckets_ = nullptr;
    }

    void clear_buckets() {
        if (!buckets_) return;
        for (size_t i = 0; i < num_buckets_; ++i) {
            BucketNode* curr = buckets_[i];
            while (curr != nullptr) {
                BucketNode* next = curr->next;
                delete curr;
                curr = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    size_t get_bucket_index(const T& val) const {
        return Hash{}(val) % num_buckets_;
    }

    template<typename U>
    bool insert_internal(U&& val) {
        if (contains(val)) {
            return false;
        }

        if (static_cast<double>(size_ + 1) > max_load_factor_ * num_buckets_) {
            rehash(num_buckets_ * 2);
        }

        size_t idx = get_bucket_index(val);
        BucketNode* new_node = new BucketNode(buckets_[idx], std::forward<U>(val));
        buckets_[idx] = new_node;
        ++size_;
        return true;
    }

    void rehash(size_t new_bucket_count) {
        if (new_bucket_count < 8) new_bucket_count = 8;
        BucketNode** new_buckets = new BucketNode*[new_bucket_count]();

        for (size_t i = 0; i < num_buckets_; ++i) {
            BucketNode* curr = buckets_[i];
            while (curr != nullptr) {
                BucketNode* next = curr->next;
                size_t new_idx = Hash{}(curr->value) % new_bucket_count;
                curr->next = new_buckets[new_idx];
                new_buckets[new_idx] = curr;
                curr = next;
            }
        }

        delete[] buckets_;
        buckets_ = new_buckets;
        num_buckets_ = new_bucket_count;
    }

    size_t num_buckets_{16};
    size_t size_{0};
    double max_load_factor_{0.75};
    BucketNode** buckets_{nullptr};
};

} // namespace adaptive_ds
