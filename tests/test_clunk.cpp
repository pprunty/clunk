#include <gtest/gtest.h>
#include "clunk/clunk.hpp"

TEST(ClunkTest, TestSimple) {
    clunk::ClunkApp app;
    // Just a trivial check for now
    EXPECT_NO_THROW(app.run());
}

// Entry point for Google Test
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
