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

// Test basic functionality without project dependencies
TEST(BasicFunctionalityTest, MathOperations) {
    // Test basic math operations
    EXPECT_EQ(10 / 2, 5);
    EXPECT_EQ(10 % 3, 1);
    EXPECT_GT(3.14, 3.0);
    EXPECT_LT(2.71, 3.0);
}

TEST(BasicFunctionalityTest, StringOperations) {
    std::string test = "Smart Monitor";
    EXPECT_EQ(test.length(), 13);
    EXPECT_TRUE(test.find("Smart") != std::string::npos);
    EXPECT_FALSE(test.find("Absent") != std::string::npos);
}

TEST(BasicFunctionalityTest, ArrayOperations) {
    int array[] = {1, 2, 3, 4, 5};
    int sum = 0;
    
    for (int i = 0; i < 5; i++) {
        sum += array[i];
    }
    
    EXPECT_EQ(sum, 15);
    EXPECT_EQ(array[0], 1);
    EXPECT_EQ(array[4], 5);
}

// Test environment setup
class EnvironmentTest : public ::testing::Environment {
public:
    void SetUp() override {
        // Test environment setup
        setenv("SMART_MONITOR_TEST_MODE", "1", 1);
    }
    
    void TearDown() override {
        // Test environment cleanup
        unsetenv("SMART_MONITOR_TEST_MODE");
    }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add global test environment
    ::testing::AddGlobalTestEnvironment(new EnvironmentTest);
    
    return RUN_ALL_TESTS();
}
