#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <common/hash.h>

using namespace hash;

// Helper: generate all valid strings of a given length
static std::vector<std::string> all_strings_of_length(size_t len) {
    std::vector<std::string> result;
    if (len == 0) {
        result.push_back("");
        return result;
    }
    std::string s(len, ' ');
    std::function<void(size_t)> gen = [&](size_t pos) {
        if (pos == len) {
            result.push_back(s);
            return;
        }
        for (char c : kCharset) {
            s[pos] = c;
            gen(pos + 1);
        }
    };
    gen(0);
    return result;
}

// Test suite for EncodeString & DecodeString

TEST(EncodeDecodeTest, EmptyString) {
    auto code = EncodeString("");
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(*code, 0ULL);
    auto decoded = DecodeString(*code);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, "");
}

TEST(EncodeDecodeTest, SingleChars) {
    for (char c : kCharset) {
        std::string s(1, c);
        auto code = EncodeString(s);
        ASSERT_TRUE(code.has_value());
        auto decoded = DecodeString(*code);
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(*decoded, s);
    }
}

TEST(EncodeDecodeTest, RoundTripShortStrings) {
    // Exhaustively test lengths 0..3 (safe)
    for (size_t len = 0; len <= 3; ++len) {
        auto strings = all_strings_of_length(len);
        for (const auto& s : strings) {
            auto code = EncodeString(s);
            ASSERT_TRUE(code.has_value()) << "Failed for: \"" << s << "\"";
            auto decoded = DecodeString(*code);
            ASSERT_TRUE(decoded.has_value());
            EXPECT_EQ(*decoded, s);
        }
    }
}

TEST(EncodeDecodeTest, RoundTripLongerStrings) {
    // Test a fixed set of valid strings for lengths 4..10
    std::vector<std::string> samples = {"abcd",
                                        "ABCD",
                                        "0123",
                                        "aBcD",
                                        "Z9y8",
                                        "abcdef",
                                        "ABCDEF",
                                        "012345",
                                        "aBcDeF",
                                        "abcdefgh",
                                        "ABCDEFGH",
                                        "01234567",
                                        "abcdefghij",
                                        "ABCDEFGHIJ",
                                        "0123456789",
                                        "aZbYcXdWeV",
                                        "A1B2C3D4E5"};
    for (const auto& s : samples) {
        auto code = EncodeString(s);
        ASSERT_TRUE(code.has_value()) << "Failed for: \"" << s << "\"";
        auto decoded = DecodeString(*code);
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(*decoded, s);
    }
}

TEST(EncodeDecodeTest, UniqueCodesForDistinctStrings) {
    std::vector<std::string> samples = {"a", "b", "aa", "ab", "ba", "zz", "aA", "Aa", "0", "9"};
    for (size_t i = 0; i < samples.size(); ++i) {
        for (size_t j = i + 1; j < samples.size(); ++j) {
            auto c1 = EncodeString(samples[i]);
            auto c2 = EncodeString(samples[j]);
            ASSERT_TRUE(c1.has_value());
            ASSERT_TRUE(c2.has_value());
            EXPECT_NE(*c1, *c2) << "Collision between \"" << samples[i] << "\" and \"" << samples[j] << "\"";
        }
    }
}

TEST(EncodeDecodeTest, InvalidInputTooLong) {
    std::string long_str(11, 'a');
    auto code = EncodeString(long_str);
    EXPECT_FALSE(code.has_value());
}

TEST(EncodeDecodeTest, InvalidInputIllegalChar) {
    EXPECT_FALSE(EncodeString("hello!").has_value());
    EXPECT_FALSE(EncodeString(" ").has_value());
    EXPECT_FALSE(EncodeString("\n").has_value());
}

TEST(DecodeTest, MalformedCodes) {
    uint64_t bad_len = 15ULL << kLengthShift;
    EXPECT_FALSE(DecodeString(bad_len).has_value());

    uint64_t bad_idx = (62ULL << 0) | (1ULL << kLengthShift);
    EXPECT_FALSE(DecodeString(bad_idx).has_value());

    uint64_t stray = (0ULL << kLengthShift) | (0ULL << 0) | (1ULL << 6);
    EXPECT_FALSE(DecodeString(stray).has_value());

    uint64_t stray2 = (2ULL << kLengthShift) | (0ULL << 0) | (0ULL << 6) | (1ULL << 12);
    EXPECT_FALSE(DecodeString(stray2).has_value());
}

TEST(DecodeTest, ValidCodesWithMaxLength) {
    std::string s(10, 'z');
    auto code = EncodeString(s);
    ASSERT_TRUE(code.has_value());
    auto decoded = DecodeString(*code);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, s);

    s = "0123456789";
    code = EncodeString(s);
    ASSERT_TRUE(code.has_value());
    decoded = DecodeString(*code);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, s);
}

// Test suite for hash::Int64Hasher (splitmix64)

TEST(Int64HasherTest, SameValueYieldsSameHash) {
    hash::Int64Hasher hasher;
    const uint64_t kValue = 0xdeadbeefcafebabeULL;
    const std::size_t kHash1 = hasher(kValue);
    const std::size_t kHash2 = hasher(kValue);
    EXPECT_EQ(kHash1, kHash2);
}

TEST(Int64HasherTest, DifferentValuesYieldDifferentHashes) {
    hash::Int64Hasher hasher;
    const std::size_t kHash0 = hasher(0);
    const std::size_t kHash1 = hasher(1);
    const std::size_t kHashMax = hasher(std::numeric_limits<uint64_t>::max());

    EXPECT_NE(kHash0, kHash1);
    EXPECT_NE(kHash0, kHashMax);
    EXPECT_NE(kHash1, kHashMax);
}

TEST(Int64HasherTest, ZeroInputIsProcessed) {
    hash::Int64Hasher hasher;
    const std::size_t kHashZero = hasher(0);
    // No specific value expected, just ensure it's callable and deterministic.
    EXPECT_EQ(kHashZero, hasher(0));
}

TEST(Int64HasherTest, LargeInputIsHandled) {
    hash::Int64Hasher hasher;
    const uint64_t kLarge = 0xFFFFFFFFFFFFFFFFULL;
    const std::size_t kHashLarge = hasher(kLarge);
    EXPECT_EQ(kHashLarge, hasher(kLarge));
}

TEST(Int64HasherTest, ConsecutiveValuesAreWellDistributed) {
    hash::Int64Hasher hasher;
    const std::size_t kPrev = hasher(1000);
    const std::size_t kCurr = hasher(1001);
    // Not a strict requirement, but high probability they differ.
    EXPECT_NE(kPrev, kCurr);
}
