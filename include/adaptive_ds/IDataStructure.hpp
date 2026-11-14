#pragma once

#include "Types.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

namespace adaptive_ds {

template<typename T, 
         typename Compare = std::less<T>, 
         typename Hash = std::hash<T>, 
         typename KeyEqual = std::equal_to<T>>
class IDataStructure {
public:
    virtual ~IDataStructure() = default;

    // Core operations
    virtual bool insert(const T& val) = 0;
    virtual bool insert(T&& val) = 0;
    virtual bool erase(const T& val) = 0;
    virtual bool contains(const T& val) const = 0;
    virtual const T* find(const T& val) const = 0;
    virtual std::vector<T> range(const T& low, const T& high) const = 0;

    // Capacity & state
    virtual size_t size() const noexcept = 0;
    virtual bool empty() const noexcept = 0;
    virtual void clear() = 0;

    // Data extraction for migration & iteration
    virtual std::vector<T> to_vector() const = 0;

    // Introspection
    virtual StructureType type() const noexcept = 0;
    virtual const char* type_name() const noexcept = 0;
    virtual size_t memory_bytes() const noexcept = 0;

    // Deep copy clone
    virtual std::unique_ptr<IDataStructure<T, Compare, Hash, KeyEqual>> clone() const = 0;
};

} // namespace adaptive_ds
