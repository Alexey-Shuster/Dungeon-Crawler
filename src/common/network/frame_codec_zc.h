#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/endian/conversion.hpp>
#include <cstddef>
#include <cstdint>  // IWYU pragma: keep // uint
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace network {

/**
 * @brief Non-owning view over payload bytes.
 *
 * Valid only inside the callback that received it.
 * Do not store, capture, or use after the callback returns.
 */
using PayloadView = std::span<const std::uint8_t>;

/**
 * @brief Zero-copy framing codec for stream transports (TCP, TLS, pipes).
 *
 * @par Wire format
 * @code
 *   [magic : 4 bytes]  [length : 4 bytes]  [payload : length bytes]
 *   network byte order  network byte order  raw bytes
 * @endcode
 *
 * Both header fields are 32-bit unsigned integers transmitted in
 * **network byte order** (big-endian), independent of host endianness.
 * On the sender side they are produced with @c boost::endian::native_to_big;
 * on the receiver side they are read with @c boost::endian::big_to_native.
 * The payload is a raw byte sequence and is never byte-swapped.
 *
 * @par Encode contract
 * @ref encode takes a payload vector by rvalue reference, moves it into a
 * @ref Frame, and prepends an 8-byte header. No payload byte is ever
 * copied. The returned @c Frame owns the header and the payload and
 * yields a two-buffer sequence via @ref Frame::buffers for
 * @c boost::asio::async_write.
 *
 * @par Decode contract
 * @ref decode walks a @c flat_buffer, dispatches every complete frame
 * through the supplied callback, and **consumes from the buffer every
 * byte it has processed** — including bytes skipped during
 * resynchronization. This is required: without consuming, the next
 * call would re-parse the same frames indefinitely.
 *
 * Bytes that do not yet form a complete frame are left in the buffer
 * for the next call. "Not enough data yet" is a normal outcome, not an
 * error; @ref decode does not throw for incomplete input.
 *
 * The @c PayloadView passed to the callback points directly into the
 * buffer and is valid **only for the duration of that callback**.
 * The callback must not modify or consume the buffer, and
 * must not rely on the buffer's data outside the duration of the call.
 * Any exception thrown by the callback propagates out of @ref decode
 * unchanged.
 *
 * @par Oversized-frame protection
 * The payload length is validated against @ref kMaxMessageSize as soon
 * as the header is parsed, **before** the codec ever waits for
 * payload bytes. A candidate with an excessive length is rejected and
 * the parser resynchronizes from the next byte, so a valid frame
 * following a malformed one is still recovered. This prevents a peer
 * from forcing the buffer to grow via a single 8-byte header.
 *
 * @par Thread safety
 * Stateless and thread-safe. A single @c flat_buffer must not be
 * touched concurrently.
 *
 * @par Example
 * @code
 * // Send
 * auto frame = std::make_shared<Frame>(FrameCodec::encode(std::move(bytes)));
 * auto buffers = frame->buffers();
 * boost::asio::async_write(sock, buffers,
 *  [frame](auto ec, std::size_t) { ... });
 *
 * // Receive (after every async_read)
 * FrameCodec::decode(read_buffer_, [](PayloadView p) {
 *     handle(p);   // use p directly; do not store it
 * });
 * @endcode
 */
class FrameCodec {
private:
    // Wire-format constants
    static constexpr std::uint32_t kMagic = 0xDEADBEEF;
    static constexpr std::size_t kMagicLength = 4;
    static constexpr std::size_t kSizeLength = 4;
    static constexpr std::size_t kHeaderLength = kMagicLength + kSizeLength;

    static_assert(sizeof(std::uint32_t) == 4, "FrameCodec wire format assumes a 32-bit uint32_t.");

    /// Scan @p data and dispatch every complete frame.
    /// @return Number of frames dispatched; @p offset advanced past them.
    template <typename FrameHandler>
    static std::size_t scan(const std::uint8_t* data, std::size_t size, std::size_t& offset, FrameHandler&& onFrame) {
        std::size_t count = 0;

        // Scan while a full header still fits
        while (offset + kHeaderLength <= size) {
            // Not a magic start -> this byte is junk, resync one step
            if (boost::endian::load_big_u32(data + offset) != kMagic) {
                ++offset;
                continue;
            }

            // Magic matched; read the declared payload length
            const std::uint32_t length_v = boost::endian::load_big_u32(data + offset + kMagicLength);

            // Oversized -> treat as corruption, resync from next byte
            if (length_v > kMaxMessageSize) {
                ++offset;
                continue;
            }

            // Full frame fits? If not, stop here and wait for more data
            const std::size_t total = kHeaderLength + length_v;
            if (offset + total > size)
                break;

            // Complete frame: hand a view into the buffer to the caller.
            // The view dies when this call returns.
            onFrame(PayloadView{data + offset + kHeaderLength, length_v});
            ++count;
            offset += total;  // skip the whole frame
        }

        return count;
    }

public:
    /// Maximum allowed payload size. Applies to encode and decode.
    static constexpr std::size_t kMaxMessageSize = 16 * 1024 * 1024;

    /**
     * @brief Outbound frame: header + payload, ready to send.
     *
     * Owns its bytes. Move-only. The only public operation is
     * @ref buffers.
     *
     * @warning The frame must outlive any asynchronous write that
     *          references its buffers. Keep it in a queue and pop it
     *          from the write completion handler.
     */
    class Frame {
    public:
        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;
        Frame(Frame&&) noexcept = default;
        Frame& operator=(Frame&&) noexcept = default;
        ~Frame() = default;

        /// Two-buffer sequence for async_write: [header, payload].
        [[nodiscard]] std::array<boost::asio::const_buffer, 2> buffers() const noexcept {
            return {boost::asio::buffer(header_), boost::asio::buffer(payload_)};
        }

    private:
        friend class FrameCodec;

        Frame(std::array<std::uint8_t, kHeaderLength> h, std::vector<std::uint8_t> p) noexcept :
            header_(h), payload_(std::move(p)) {}

        std::array<std::uint8_t, kHeaderLength> header_{};
        std::vector<std::uint8_t> payload_;
    };

    /**
     * @brief Wrap @p payload in a frame ready to send.
     *
     * The header (magic + length) is serialized in network byte order
     * via @c boost::endian::native_to_big. The payload is moved, not copied.
     *
     * @param payload Payload bytes. Moved into the frame.
     * @return Frame owning header and payload.
     * @throws std::length_error If @p payload.size() >
     *         @ref kMaxMessageSize.
     */
    [[nodiscard]] static Frame encode(std::vector<std::uint8_t>&& payload) {
        if (payload.size() > kMaxMessageSize) {
            throw std::length_error("FrameCodec::encode: payload exceeds kMaxMessageSize");
        }

        const std::uint32_t magic_be = boost::endian::native_to_big(kMagic);
        const std::uint32_t length_be = boost::endian::native_to_big(static_cast<std::uint32_t>(payload.size()));

        const auto magic_bytes = std::bit_cast<std::array<std::uint8_t, kMagicLength>>(magic_be);
        const auto length_bytes = std::bit_cast<std::array<std::uint8_t, kSizeLength>>(length_be);

        std::array<std::uint8_t, kHeaderLength> header{};
        std::ranges::copy(magic_bytes, header.begin());
        std::ranges::copy(length_bytes, header.begin() + kMagicLength);

        return Frame{header, std::move(payload)};
    }

    /**
     * @brief Dispatch every complete frame found in @p buffer.
     *
     * Consumes every byte that belongs to a parsed frame, plus every
     * byte skipped during resynchronization. Leaves trailing bytes that
     * do not yet form a complete frame for the next call.
     *
     * Rejects candidates whose length exceeds @ref kMaxMessageSize
     * before waiting for payload bytes, and continues scanning from
     * the next byte.
     *
     * @param buffer  Receive buffer. Must be contiguous
     *                (@c boost::beast::flat_buffer). Modified: parsed
     *                bytes are consumed from it.
     * @param onFrame Called once per complete payload, in arrival
     *                order. Must not read from, write to, or consume
     *                @p buffer. The @ref PayloadView it receives is
     *                valid only for the duration of the call.
     * @return Number of frames dispatched.
     * @throws Any exception thrown by @p onFrame (propagated
     *         unchanged). Never thrown for incomplete input.
     */
    template <typename FrameHandler>
    [[nodiscard]] static std::size_t decode(boost::beast::flat_buffer& buffer, FrameHandler&& onFrame) {
        // Snapshot the buffer once; the codec never re-queries it while parsing
        const auto seq = buffer.data();
        const auto* data = static_cast<const std::uint8_t*>(seq.data());
        const auto size = seq.size();

        // Fewer bytes than one header: nothing to do, keep them for next time
        if (size < kHeaderLength)
            return 0;

        std::size_t offset = 0;  // first byte not yet resolved

        // Consume whatever the scanner resolved, on every exit path:
        // normal return, break, or exception from onFrame.
        struct ConsumeOnExit {
            boost::beast::flat_buffer& buf;
            const std::size_t& off;

            ~ConsumeOnExit() {
                if (off > 0)
                    buf.consume(off);
            }
        } guard{buffer, offset};

        return scan(data, size, offset, std::forward<FrameHandler>(onFrame));
    }
};

}  // namespace network
