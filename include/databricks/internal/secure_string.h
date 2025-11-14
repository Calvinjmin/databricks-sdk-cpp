// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#ifndef DATABRICKS_INTERNAL_SECURE_STRING_H
#define DATABRICKS_INTERNAL_SECURE_STRING_H

#include <cstring>
#include <memory>
#include <string>
#include <sys/mman.h>

namespace databricks {
namespace internal {

/**
 * @brief Custom allocator that provides secure memory handling for sensitive data
 *
 * This allocator:
 * - Locks allocated memory pages to prevent swapping to disk
 * - Securely zeros memory before deallocation
 * - Uses volatile writes to prevent compiler optimization
 *
 * @tparam T The type of object to allocate
 */
template <typename T> class SecureAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    SecureAllocator() noexcept = default;

    template <typename U> SecureAllocator(const SecureAllocator<U>&) noexcept {}

    template <typename U> struct rebind {
        using other = SecureAllocator<U>;
    };

    /**
     * @brief Allocate memory and lock it to prevent swapping
     */
    T* allocate(size_type n) {
        if (n == 0) {
            return nullptr;
        }

        // Check for overflow
        if (n > static_cast<size_type>(-1) / sizeof(T)) {
            throw std::bad_alloc();
        }

        void* ptr = std::malloc(n * sizeof(T));
        if (!ptr) {
            throw std::bad_alloc();
        }

        // Lock memory to prevent swapping to disk
        lock_memory(ptr, n * sizeof(T));

        return static_cast<T*>(ptr);
    }

    /**
     * @brief Securely zero memory and deallocate
     */
    void deallocate(T* ptr, size_type n) noexcept {
        if (!ptr || n == 0) {
            return;
        }

        // Securely zero memory using volatile to prevent optimization
        secure_zero(ptr, n * sizeof(T));

        // Unlock memory
        unlock_memory(ptr, n * sizeof(T));

        std::free(ptr);
    }

private:
    /**
     * @brief Lock memory pages to prevent swapping to disk
     */
    static void lock_memory(void* ptr, size_type size) noexcept {
        // Use mlock on POSIX systems
        // Ignore failures - locking is best-effort
        mlock(ptr, size);
    }

    /**
     * @brief Unlock memory pages
     */
    static void unlock_memory(void* ptr, size_type size) noexcept {
        munlock(ptr, size);
    }

    /**
     * @brief Securely zero memory using volatile writes
     *
     * Uses volatile pointer to prevent compiler from optimizing away the writes.
     * This is critical for security as standard memset() can be optimized away.
     */
    static void secure_zero(void* ptr, size_type size) noexcept {
        // Use volatile to prevent compiler optimization
        volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
        for (size_type i = 0; i < size; ++i) {
            p[i] = 0;
        }
    }
};

template <typename T, typename U> bool operator==(const SecureAllocator<T>&, const SecureAllocator<U>&) noexcept {
    return true;
}

template <typename T, typename U> bool operator!=(const SecureAllocator<T>&, const SecureAllocator<U>&) noexcept {
    return false;
}

/**
 * @brief A secure string type for storing sensitive data like tokens
 *
 * SecureString uses a custom allocator that:
 * - Locks memory to prevent swapping to disk
 * - Securely zeros memory when the string is destroyed
 * - Prevents compiler optimizations that might leave sensitive data in memory
 *
 * Use this type for storing passwords, tokens, and other sensitive credentials.
 *
 * Example:
 * @code
 * SecureString token = "dapi1234567890abcdef";
 * // ... use token ...
 * // Memory is securely zeroed when token goes out of scope
 * @endcode
 */
using SecureString = std::basic_string<char, std::char_traits<char>, SecureAllocator<char>>;

/**
 * @brief Securely zero a standard string
 *
 * This is a utility function for clearing sensitive data from std::string objects.
 * Use this when you must use std::string for compatibility but need to clear
 * sensitive data afterward.
 *
 * @param str The string to zero
 */
inline void secure_zero_string(std::string& str) noexcept {
    if (str.empty()) {
        return;
    }

    // Use volatile to prevent compiler optimization
    volatile char* p = const_cast<volatile char*>(str.data());
    for (size_t i = 0; i < str.size(); ++i) {
        p[i] = 0;
    }

    // Clear the string to free any allocated memory
    str.clear();
    str.shrink_to_fit();
}

/**
 * @brief Convert std::string to SecureString
 *
 * Creates a SecureString from a std::string. Note that this does not
 * securely clear the source string - use secure_zero_string() afterward
 * if needed.
 *
 * @param str Source string
 * @return SecureString with copied content
 */
inline SecureString to_secure_string(const std::string& str) {
    return SecureString(str.begin(), str.end());
}

/**
 * @brief Convert SecureString to std::string
 *
 * WARNING: This creates a copy in regular memory that is NOT securely managed.
 * Use this only when necessary for interop with APIs that require std::string.
 * Consider using c_str() or data() directly when possible.
 *
 * @param secure_str Source secure string
 * @return std::string with copied content
 */
inline std::string from_secure_string(const SecureString& secure_str) {
    return std::string(secure_str.begin(), secure_str.end());
}

} // namespace internal
} // namespace databricks

#endif // DATABRICKS_INTERNAL_SECURE_STRING_H
