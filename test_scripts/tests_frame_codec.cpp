#include <boost/asio.hpp>
#include <common/frame_codec.h>
#include <cstring>
#include <gtest/gtest.h>

using namespace network;

// Helper: fill a boost::asio::streambuf with binary data
static void fillStreambuf(boost::asio::streambuf& sb, const std::vector<uint8_t>& data) {
    auto mutable_buffer = sb.prepare(data.size());
    boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(data));
    sb.commit(data.size());
}

// Helper: extract content of a streambuf into a vector (for verification)
static std::vector<uint8_t> streambufToVector(const boost::asio::streambuf& sb) {
    const auto& data = sb.data();
    std::vector<uint8_t> vec(boost::asio::buffer_size(data));
    boost::asio::buffer_copy(boost::asio::buffer(vec), data);
    return vec;
}

TEST(FrameCodecTest, EncodeDecodeSingleFrame) {
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
    auto frame = FrameCodec::encodeFrame(payload);
    EXPECT_FALSE(frame.empty());

    // Verify magic
    uint32_t magic;
    std::memcpy(&magic, frame.data(), FrameCodec::kMagicLength);
    EXPECT_EQ(ntohl(magic), FrameCodec::kMagic);

    // Verify length
    uint32_t len;
    std::memcpy(&len, frame.data() + FrameCodec::kMagicLength, FrameCodec::kSizePrefixLength);
    EXPECT_EQ(ntohl(len), payload.size());

    // Decode
    boost::asio::streambuf sb;
    fillStreambuf(sb, frame);
    auto messages = FrameCodec::extractFrames(sb);
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0], payload);
    EXPECT_EQ(sb.size(), 0);
}

TEST(FrameCodecTest, EmptyPayload) {
    std::vector<uint8_t> payload;
    auto frame = FrameCodec::encodeFrame(payload);
    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.size(), FrameCodec::kMagicLength + FrameCodec::kSizePrefixLength);

    uint32_t len;
    std::memcpy(&len, frame.data() + FrameCodec::kMagicLength, FrameCodec::kSizePrefixLength);
    EXPECT_EQ(ntohl(len), 0);

    boost::asio::streambuf sb;
    fillStreambuf(sb, frame);
    auto messages = FrameCodec::extractFrames(sb);
    ASSERT_EQ(messages.size(), 1);
    EXPECT_TRUE(messages[0].empty());
    EXPECT_EQ(sb.size(), 0);
}

TEST(FrameCodecTest, MultipleFrames) {
    std::vector<uint8_t> p1 = {1, 2};
    std::vector<uint8_t> p2 = {3, 4, 5, 6};
    auto f1 = FrameCodec::encodeFrame(p1);
    auto f2 = FrameCodec::encodeFrame(p2);

    std::vector<uint8_t> combined;
    combined.insert(combined.end(), f1.begin(), f1.end());
    combined.insert(combined.end(), f2.begin(), f2.end());

    boost::asio::streambuf sb;
    fillStreambuf(sb, combined);
    auto messages = FrameCodec::extractFrames(sb);
    ASSERT_EQ(messages.size(), 2);
    EXPECT_EQ(messages[0], p1);
    EXPECT_EQ(messages[1], p2);
    EXPECT_EQ(sb.size(), 0);
}

TEST(FrameCodecTest, IncompleteFrame) {
    std::vector<uint8_t> payload = {1, 2, 3};
    auto frame = FrameCodec::encodeFrame(payload);
    // Remove the last byte (part of payload)
    frame.pop_back();

    boost::asio::streambuf sb;
    fillStreambuf(sb, frame);
    auto messages = FrameCodec::extractFrames(sb);
    EXPECT_TRUE(messages.empty());

    // The incomplete frame (including its magic+length) must be preserved
    EXPECT_EQ(sb.size(), frame.size());
    EXPECT_EQ(streambufToVector(sb), frame);
}

TEST(FrameCodecTest, OneCompleteOneIncomplete) {
    std::vector<uint8_t> p1 = {1, 2, 3};
    std::vector<uint8_t> p2 = {4, 5, 6, 7, 8};
    auto f1 = FrameCodec::encodeFrame(p1);
    auto f2 = FrameCodec::encodeFrame(p2);
    // Truncate f2 by 2 bytes
    f2.resize(f2.size() - 2);

    std::vector<uint8_t> combined;
    combined.insert(combined.end(), f1.begin(), f1.end());
    combined.insert(combined.end(), f2.begin(), f2.end());

    boost::asio::streambuf sb;
    fillStreambuf(sb, combined);
    auto messages = FrameCodec::extractFrames(sb);
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0], p1);

    // The incomplete frame (f2) must remain intact
    EXPECT_EQ(sb.size(), f2.size());
    EXPECT_EQ(streambufToVector(sb), f2);
}

TEST(FrameCodecTest, OversizedFrameSkippedAndResync) {
    // Build a frame with magic + oversize length + some data
    std::vector<uint8_t> buffer;
    buffer.resize(FrameCodec::kMagicLength + FrameCodec::kSizePrefixLength + 5);
    uint32_t net_magic = htonl(FrameCodec::kMagic);
    std::memcpy(buffer.data(), &net_magic, FrameCodec::kMagicLength);
    uint32_t huge_len = htonl(100 * 1024 * 1024);  // > kMaxMessageSize
    std::memcpy(buffer.data() + FrameCodec::kMagicLength, &huge_len, FrameCodec::kSizePrefixLength);
    // Fill the rest with arbitrary data
    std::fill(buffer.begin() + FrameCodec::kMagicLength + FrameCodec::kSizePrefixLength, buffer.end(), 0x55);

    // Append a valid frame afterwards
    std::vector<uint8_t> valid_payload = {0xAA, 0xBB};
    auto valid_frame = FrameCodec::encodeFrame(valid_payload);
    buffer.insert(buffer.end(), valid_frame.begin(), valid_frame.end());

    boost::asio::streambuf sb;
    fillStreambuf(sb, buffer);
    auto messages = FrameCodec::extractFrames(sb);

    // Only the valid frame should be extracted
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0], valid_payload);

    // All bytes (including the oversized frame and the valid frame) are consumed
    EXPECT_EQ(sb.size(), 0);
}

TEST(FrameCodecTest, MaxSizeAllowed) {
    std::vector<uint8_t> payload(FrameCodec::kMaxMessageSize, 0xAA);
    auto frame = FrameCodec::encodeFrame(payload);
    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.size(), FrameCodec::kMagicLength + FrameCodec::kSizePrefixLength + FrameCodec::kMaxMessageSize);

    boost::asio::streambuf sb;
    fillStreambuf(sb, frame);
    auto messages = FrameCodec::extractFrames(sb);
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].size(), FrameCodec::kMaxMessageSize);
    EXPECT_EQ(sb.size(), 0);
}

TEST(FrameCodecTest, ExceedsMaxSize) {
    std::vector<uint8_t> payload(FrameCodec::kMaxMessageSize + 1, 0xAA);
    auto frame = FrameCodec::encodeFrame(payload);
    EXPECT_TRUE(frame.empty());
}

TEST(FrameCodecTest, JunkDataBeforeFrame) {
    // Random junk before a valid frame
    std::vector<uint8_t> junk = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<uint8_t> payload = {0x10, 0x20};
    auto valid_frame = FrameCodec::encodeFrame(payload);

    std::vector<uint8_t> combined;
    combined.insert(combined.end(), junk.begin(), junk.end());
    combined.insert(combined.end(), valid_frame.begin(), valid_frame.end());

    boost::asio::streambuf sb;
    fillStreambuf(sb, combined);
    auto messages = FrameCodec::extractFrames(sb);
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0], payload);
    // Junk should be consumed, only the frame remains (but we consumed it all)
    EXPECT_EQ(sb.size(), 0);
}

TEST(FrameCodecTest, PartialMagicAtEnd) {
    // Buffer ends with first 3 bytes of magic: 0xDE, 0xAD, 0xBE
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE};
    boost::asio::streambuf sb;
    fillStreambuf(sb, data);
    auto messages = FrameCodec::extractFrames(sb);
    EXPECT_TRUE(messages.empty());
    // Should keep the last 3 bytes (potential partial magic)
    EXPECT_EQ(sb.size(), 3);
    EXPECT_EQ(streambufToVector(sb), data);
}

TEST(FrameCodecTest, IncompleteMagicAtEnd) {
    // Only 2 bytes of magic at end
    std::vector<uint8_t> data = {0xDE, 0xAD};
    boost::asio::streambuf sb;
    fillStreambuf(sb, data);
    auto messages = FrameCodec::extractFrames(sb);
    EXPECT_TRUE(messages.empty());
    // Buffer size < kMagicLength, so nothing is consumed (keeps all)
    EXPECT_EQ(sb.size(), 2);
    EXPECT_EQ(streambufToVector(sb), data);
}
