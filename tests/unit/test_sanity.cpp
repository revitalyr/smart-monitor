#include <gtest/gtest.h>

// Simple test to verify test framework is working
TEST(SanityTest, BasicAssertions) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST(SanityTest, StringAssertions) {
    const char* test_string = "Hello, Smart Monitor!";
    EXPECT_STREQ(test_string, "Hello, Smart Monitor!");
    EXPECT_STRNE(test_string, "Goodbye");
}

TEST(SanityTest, FloatingPointAssertions) {
    EXPECT_FLOAT_EQ(1.0f, 1.0f);
    EXPECT_NEAR(3.14159f, 3.14f, 0.01f);
}

// Test environment setup
class EnvironmentTest : public ::testing::Environment {
public:
    void SetUp() override {
        // Test environment setup
    }
    
    void TearDown() override {
        // Test environment cleanup
    }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add global test environment
    ::testing::AddGlobalTestEnvironment(new EnvironmentTest);
    
    return RUN_ALL_TESTS();
}
