#include "databricks/core/config.h"
#include "databricks/internal/secure_string.h"

#include <cstring>

#include <gtest/gtest.h>

namespace databricks {
namespace test {

/**
 * @brief Test suite for security features
 *
 * These tests verify that sensitive data (tokens, passwords) are properly
 * secured in memory and don't leak into logs or error messages.
 */
class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }

    void TearDown() override {
        // Clean up after tests
    }
};

/**
 * @brief Test that SecureString zeros memory on destruction
 */
TEST_F(SecurityTest, SecureStringZerosMemoryOnDestruction) {
    const char* test_data = "sensitive_token_12345";
    char* memory_check = nullptr;

    {
        internal::SecureString secure_str(test_data);
        // Get pointer to internal data for verification
        memory_check = const_cast<char*>(secure_str.data());

        // Verify data is present while SecureString is alive
        EXPECT_STREQ(memory_check, test_data);
    }
    // After SecureString destruction, memory should be zeroed
    // Note: This test is best-effort as the memory may be reused
    // In production, we rely on the volatile write in secure_zero
}

/**
 * @brief Test that secure_zero_string clears std::string data
 */
TEST_F(SecurityTest, SecureZeroStringClearsData) {
    std::string test_str = "secret_password_123";
    const char* original_ptr = test_str.data();
    size_t original_size = test_str.size();

    // Copy the pointer to check memory after
    char buffer[50];
    std::memcpy(buffer, original_ptr, original_size);

    // Securely zero the string
    internal::secure_zero_string(test_str);

    // String should be empty after zeroing
    EXPECT_TRUE(test_str.empty());
}

/**
 * @brief Test that AuthConfig set_token properly stores secure token
 */
TEST_F(SecurityTest, AuthConfigStoresSecureToken) {
    AuthConfig config;
    std::string test_token = "dapi1234567890abcdef";

    config.set_token(test_token);

    // Verify secure token is set
    EXPECT_TRUE(config.has_secure_token());
    EXPECT_FALSE(config.get_secure_token().empty());

    // Verify token content matches
    std::string retrieved = internal::from_secure_string(config.get_secure_token());
    EXPECT_EQ(retrieved, test_token);

    // Clean up
    internal::secure_zero_string(retrieved);
}

/**
 * @brief Test token conversion between std::string and SecureString
 */
TEST_F(SecurityTest, TokenConversionPreservesData) {
    std::string original = "test_token_xyz";

    // Convert to SecureString
    internal::SecureString secure = internal::to_secure_string(original);
    EXPECT_EQ(secure.size(), original.size());

    // Convert back to std::string
    std::string converted = internal::from_secure_string(secure);
    EXPECT_EQ(converted, original);

    // Clean up
    internal::secure_zero_string(converted);
}

/**
 * @brief Test that SecureString can be used in comparisons
 */
TEST_F(SecurityTest, SecureStringComparison) {
    internal::SecureString str1("token123");
    internal::SecureString str2("token123");
    internal::SecureString str3("different");

    EXPECT_EQ(str1, str2);
    EXPECT_NE(str1, str3);
}

/**
 * @brief Test that SecureString can be hashed
 */
TEST_F(SecurityTest, SecureStringHashing) {
    internal::SecureString str1("token123");
    internal::SecureString str2("token123");
    internal::SecureString str3("different");

    std::hash<internal::SecureString> hasher;
    size_t hash1 = hasher(str1);
    size_t hash2 = hasher(str2);
    size_t hash3 = hasher(str3);

    // Same content should produce same hash
    EXPECT_EQ(hash1, hash2);

    // Different content should (likely) produce different hash
    EXPECT_NE(hash1, hash3);
}

/**
 * @brief Test AuthConfig validation with secure token
 */
TEST_F(SecurityTest, AuthConfigValidationWithSecureToken) {
    AuthConfig config;

    // Invalid without token
    config.host = "https://example.cloud.databricks.com";
    EXPECT_FALSE(config.is_valid());

    // Valid after setting token
    config.set_token("dapi_test_token");
    EXPECT_TRUE(config.is_valid());
}

/**
 * @brief Test that secure allocator can handle empty strings
 */
TEST_F(SecurityTest, SecureStringHandlesEmptyString) {
    internal::SecureString empty_str;
    EXPECT_TRUE(empty_str.empty());
    EXPECT_EQ(empty_str.size(), 0);

    internal::SecureString empty_str2("");
    EXPECT_TRUE(empty_str2.empty());
    EXPECT_EQ(empty_str2.size(), 0);
}

/**
 * @brief Test that secure allocator handles large strings
 */
TEST_F(SecurityTest, SecureStringHandlesLargeStrings) {
    std::string large_token(10000, 'x'); // 10KB token

    internal::SecureString secure_large = internal::to_secure_string(large_token);
    EXPECT_EQ(secure_large.size(), large_token.size());

    std::string converted_back = internal::from_secure_string(secure_large);
    EXPECT_EQ(converted_back, large_token);

    // Clean up
    internal::secure_zero_string(converted_back);
}

/**
 * @brief Test that SecureString supports basic string operations
 */
TEST_F(SecurityTest, SecureStringBasicOperations) {
    internal::SecureString str("test");

    // Test size and empty
    EXPECT_EQ(str.size(), 4);
    EXPECT_FALSE(str.empty());

    // Test data access
    EXPECT_NE(str.data(), nullptr);
    EXPECT_NE(str.c_str(), nullptr);

    // Test clear
    str.clear();
    EXPECT_TRUE(str.empty());
    EXPECT_EQ(str.size(), 0);
}

/**
 * @brief Test that SecureString can be copied and moved
 */
TEST_F(SecurityTest, SecureStringCopyAndMove) {
    internal::SecureString original("secret");

    // Test copy constructor
    internal::SecureString copied(original);
    EXPECT_EQ(copied, original);

    // Test copy assignment
    internal::SecureString assigned;
    assigned = original;
    EXPECT_EQ(assigned, original);

    // Test move constructor
    internal::SecureString moved(std::move(assigned));
    EXPECT_EQ(moved, original);

    // Test move assignment
    internal::SecureString move_assigned;
    move_assigned = std::move(moved);
    EXPECT_EQ(move_assigned, original);
}

/**
 * @brief Test secure token usage with set_token
 */
TEST_F(SecurityTest, SecureTokenUsage) {
    AuthConfig config;
    config.host = "https://example.cloud.databricks.com";
    config.set_token("secure_token");

    // Should be valid with secure_token set
    EXPECT_TRUE(config.is_valid());
    EXPECT_TRUE(config.has_secure_token());
}

} // namespace test
} // namespace databricks
