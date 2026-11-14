#pragma once

#include "IDataStructure.hpp"
#include <new>
#include <utility>
#include <algorithm>
#include <stdexcept>

namespace adaptive_ds {

template<typename T, 
         typename Compare = std::less<T>, 
         typename Hash = std::hash<T>, 
         typename KeyEqual = std::equal_to<T>>
class DynamicArray : public IDataStructure<T, Compare, Hash, KeyEqual> {
public:
    using Base = IDataStructure<T, Compare, Hash, KeyEqual>;

    explicit DynamicArray(size_t initial_capacity = 8)
        : capacity_(initial_capacity > 0 ? initial_capacity : 8),
          size_(0),
          buffer_(nullptr) {
        allocate_buffer(capacity_);
    }

    ~DynamicArray() override {
        destroy_all();
        deallocate_buffer();
    }

    DynamicArray(const DynamicArray& other)
        : capacity_(other.capacity_), size_(other.size_), buffer_(nullptr) {
        allocate_buffer(capacity_);
        for (size_t i = 0; i < size_; ++i) {
            new (buffer_ + i) T(other.buffer_[i]);
        }
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray temp(other);
            swap(temp);
        }
        return *this;
    }

    DynamicArray(DynamicArray&& other) noexcept
        : capacity_(other.capacity_), size_(other.size_), buffer_(other.buffer_) {
        other.capacity_ = 0;
        other.size_ = 0;
        other.buffer_ = nullptr;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            destroy_all();
            deallocate_buffer();
            buffer_ = other.buffer_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.buffer_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void swap(DynamicArray& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    bool insert(const T& val) override {
        if (contains(val)) {
            return false;
        }
        ensure_capacity(size_ + 1);
        new (buffer_ + size_) T(val);
        ++size_;
        return true;
    }

    bool insert(T&& val) override {
        if (contains(val)) {
            return false;
        }
        ensure_capacity(size_ + 1);
        new (buffer_ + size_) T(std::move(val));
        ++size_;
        return true;
    }

    bool erase(const T& val) override {
        KeyEqual eq;
        for (size_t i = 0; i < size_; ++i) {
            if (eq(buffer_[i], val)) {
                buffer_[i].~T();
                for (size_t j = i; j + 1 < size_; ++j) {
                    new (buffer_ + j) T(std::move(buffer_[j + 1]));
                    buffer_[j + 1].~T();
                }
                --size_;
                shrink_if_needed();
                return true;
            }
        }
        return false;
    }

    bool contains(const T& val) const override {
        return find(val) != nullptr;
    }

    const T* find(const T& val) const override {
        KeyEqual eq;
        for (size_t i = 0; i < size_; ++i) {
            if (eq(buffer_[i], val)) {
                return buffer_ + i;
            }
        }
        return nullptr;
    }

    std::vector<T> range(const T& low, const T& high) const override {
        Compare comp;
        std::vector<T> results;
        for (size_t i = 0; i < size_; ++i) {
            const T& item = buffer_[i];
            // item >= low && item <= high => !comp(item, low) && !comp(high, item)
            if (!comp(item, low) && !comp(high, item)) {
                results.push_back(item);
            }
        }
        std::sort(results.begin(), results.end(), comp);
        return results;
    }

    size_t size() const noexcept override {
        return size_;
    }

    size_t capacity() const noexcept {
        return capacity_;
    }

    bool empty() const noexcept override {
        return size_ == 0;
    }

    void clear() override {
        destroy_all();
        size_ = 0;
    }

    std::vector<T> to_vector() const override {
        std::vector<T> vec;
        vec.reserve(size_);
        for (size_t i = 0; i < size_; ++i) {
            vec.push_back(buffer_[i]);
        }
        return vec;
    }

    StructureType type() const noexcept override {
        return StructureType::DynamicArray;
    }

    const char* type_name() const noexcept override {
        return "DynamicArray";
    }

    size_t memory_bytes() const noexcept override {
        return sizeof(*this) + capacity_ * sizeof(T);
    }

    std::unique_ptr<Base> clone() const override {
        return std::make_unique<DynamicArray>(*this);
    }

    // Direct element access for benchmarks/diagnostics
    const T& operator[](size_t index) const {
        return buffer_[index];
    }

    T& operator[](size_t index) {
        return buffer_[index];
    }

private:
    void allocate_buffer(size_t cap) {
        if (cap == 0) {
            buffer_ = nullptr;
            return;
        }
        buffer_ = static_cast<T*>(::operator new(cap * sizeof(T)));
    }

    void deallocate_buffer() {
        if (buffer_) {
            ::operator delete(static_cast<void*>(buffer_));
            buffer_ = nullptr;
        }
    }

    void destroy_all() {
        for (size_t i = 0; i < size_; ++i) {
            buffer_[i].~T();
        }
    }

    void ensure_capacity(size_t min_cap) {
        if (min_cap <= capacity_) return;
        size_t new_cap = capacity_ * 2;
        if (new_cap < min_cap) new_cap = min_cap;
        reallocate(new_cap);
    }

    void shrink_if_needed() {
        if (capacity_ > 16 && size_ <= capacity_ / 4) {
            size_t new_cap = capacity_ / 2;
            if (new_cap < 8) new_cap = 8;
            reallocate(new_cap);
        }
    }

    void reallocate(size_t new_cap) {
        T* new_buffer = static_cast<T*>(::operator new(new_cap * sizeof(T)));
        for (size_t i = 0; i < size_; ++i) {
            new (new_buffer + i) T(std::move(buffer_[i]));
            buffer_[i].~T();
        }
        deallocate_buffer();
        buffer_ = new_buffer;
        capacity_ = new_cap;
    }

    size_t capacity_;
    size_t size_;
    T* buffer_;
};

} // namespace adaptive_ds
