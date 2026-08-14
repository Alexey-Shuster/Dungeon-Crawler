#pragma once

#include <boost/asio.hpp>  // asio::streambuf, asio::buffer, etc.
#include <cstring>         // std::memcpy
#include <vector>

#include "network/byte_buffer.h"

#ifdef _WIN32
    #include <winsock2.h>  // htonl / ntohl on Windows
#else
    #include <arpa/inet.h>  // htonl / ntohl on Unix/Linux
#endif

#include "utility/logger.h"

namespace dungeons::common::network {
/**
 * @brief Utility class for message framing over stream protocols (e.g., TCP).
 *
 * Adds a fixed 4‑byte magic number and a 4‑byte length prefix (both big‑endian)
 * before each message, allowing the receiver to:
 * - identify message boundaries,
 * - detect and skip invalid/oversized frames,
 * - resynchronize after corrupted data.
 *
 * @par Frame format
 *   [MAGIC (4 bytes)] [length (4 bytes, network order)] [payload]
 *
 * @par Receiver behaviour
 *   The `extractFrames` method scans the input buffer for the magic number.
 *   When a magic is found, it verifies the length and extracts the complete
 *   payload if available. Invalid frames (e.g., oversized length) are skipped,
 *   and the search continues from the next byte, so valid data after corruption
 *   is not lost.
 */
// TODO: zero-copy
/*
- asio::streambuf --> boost::beast::flat_buffer
- rework without buffer_copy()
- return std::span instead of vector
*/
class FrameCodec {
public:
    /// Maximum allowed payload size
    static constexpr size_t kMaxMessageSize = 16 * 1024 * 1024;

    /// Magic number used for frame synchronization (network‑order when sent)
    // TODO: 8 bytes for magic
    static constexpr uint32_t kMagic = 0xDEADBEEF;

    /// Length of the magic prefix
    static constexpr size_t kMagicLength = 4;

    /// Length of the size prefix
    static constexpr size_t kSizePrefixLength = 4;

    /**
     * @brief Builds a frame ready for transmission.
     *
     * @param payload The original data to send.
     * @return std::vector<uint8_t> Frame with magic + length prefix + payload.
     *         Returns an empty vector if payload exceeds kMaxMessageSize.
     */
    static ByteBuffer encodeFrame(const ByteBuffer& payload) {
        if (payload.size() > kMaxMessageSize) {
            LOG_INFO("[FrameCodec::encodeFrame] payload.size() > kMaxMessageSize");
            return {};
        }

        const size_t total_len = kMagicLength + kSizePrefixLength + payload.size();
        std::vector<uint8_t> frame(total_len);

        // Write magic number in network byte order
        uint32_t net_magic = htonl(kMagic);
        std::memcpy(frame.data(), &net_magic, kMagicLength);

        // Write payload length in network byte order
        uint32_t net_len = htonl(static_cast<uint32_t>(payload.size()));
        std::memcpy(frame.data() + kMagicLength, &net_len, kSizePrefixLength);

        // Copy payload
        if (!payload.empty()) {
            std::memcpy(frame.data() + kMagicLength + kSizePrefixLength, payload.data(), payload.size());
        }

        return frame;
    }

    /**
     * @brief Extracts all complete frames from an Asio streambuf.
     *
     * The method scans the buffer for the magic number, verifies frame integrity,
     * and extracts valid payloads. Processed bytes (including any junk data)
     * are consumed from the streambuf. Incomplete or invalid frames are handled
     * as follows:
     * - If a valid frame is found, all bytes up to its end are consumed.
     * - If no valid frame is found, the method safely discards bytes that
     *   cannot be part of a magic sequence (keeping the last 3 bytes for
     *   potential partial magic).
     *
     * @param buffer Receive buffer (in/out). Data is accumulated between calls.
     * @return std::vector<std::vector<uint8_t>> Vector of extracted payloads
     *         (without magic or length prefixes). Each element is a complete message.
     */
    static std::vector<ByteBuffer> extractFrames(boost::asio::streambuf& buffer) {
        std::vector<std::vector<uint8_t>> messages;

        // Get current buffer size and copy data into a contiguous vector
        const size_t buf_size = buffer.size();
        if (buf_size < kMagicLength) {
            return messages;  // not enough data to even attempt sync
        }

        // copy into std::vector<uint8_t> for simple index‑based scanning
        // avoids dealing with Boost.Asio’s scatter‑gather buffer sequence.
        std::vector<uint8_t> data(buf_size);
        boost::asio::buffer_copy(boost::asio::buffer(data), buffer.data());

        size_t offset = 0;
        size_t first_incomplete_magic_offset = buf_size;

        // TODO: optimize (std::search?)
        while (offset + kMagicLength <= data.size()) {
            // Check for magic number at current offset
            uint32_t net_magic;
            std::memcpy(&net_magic, data.data() + offset, kMagicLength);
            if (ntohl(net_magic) != kMagic) {
                ++offset;
                continue;
            }

            // Magic found – ensure we have enough data for the length field
            if (offset + kMagicLength + kSizePrefixLength > data.size()) {
                // Not enough data for length – cannot decide if this is a valid frame.
                // Keep everything from this magic onward, discard before it.
                first_incomplete_magic_offset = offset;
                break;  // need more data
            }

            // Read the payload length
            uint32_t net_len;
            std::memcpy(&net_len, data.data() + offset + kMagicLength, kSizePrefixLength);
            const uint32_t payload_len = ntohl(net_len);

            if (payload_len > kMaxMessageSize) {
                // Invalid length – skip this magic and continue searching
                ++offset;
                continue;
            }

            const size_t total_len = kMagicLength + kSizePrefixLength + payload_len;
            if (offset + total_len > data.size()) {
                // Valid magic+length, but payload is incomplete.
                // Keep everything from this magic onward, discard before it.
                first_incomplete_magic_offset = offset;
                break;  // incomplete frame – wait for more data
            }

            // Complete frame - extract the payload (skip magic and length)
            messages.emplace_back(data.begin() + offset + kMagicLength + kSizePrefixLength,
                                  data.begin() + offset + total_len);

            // Move past this frame and continue searching
            offset += total_len;
        }

        // Determine how many bytes to consume from the streambuf
        size_t bytes_to_consume = 0;

        if (first_incomplete_magic_offset != buf_size) {
            // We found an incomplete frame – consume everything before its magic
            bytes_to_consume = first_incomplete_magic_offset;
        } else {
            // Either we consumed some complete frames (offset > 0) or we scanned
            // the whole buffer and found no magic. In both cases, consuming 'offset'
            // bytes is safe. If no magic was found, offset = data.size() - 3,
            // meaning we keep exactly the last 3 bytes for a potential partial magic.
            bytes_to_consume = offset;
        }

        // TODO: rework with consume until
        buffer.consume(bytes_to_consume);
        return messages;
    }
};
}  // namespace dungeons::common::network
