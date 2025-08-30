#include <catch2/catch_test_macros.hpp>

#include <vortex/util/byte_ring.h>

TEST_CASE("ByteRing.BasicOperations", "[byte_ring]") {
    vortex::byte_ring ring(4); // Start with a small capacity
    // Test writing data
    constexpr std::string_view data_1 = "ABCD";
    std::span data1 = std::span{ data_1.data(), data_1.size() };
    REQUIRE(ring.write_as(data1) == std::errc{});
    REQUIRE(ring.size() == 4);
    REQUIRE(ring.capacity() == 8);
    REQUIRE(ring.available_space() != 0); // Should have space left after expansion

    // Test reading data
    std::array<char, 4> read_buffer1;
    REQUIRE(ring.read_as(std::span{ read_buffer1 }) == 4);
    REQUIRE(std::memcmp(read_buffer1.data(), "ABCD", 4) == 0);
    REQUIRE(ring.size() == 0);
    REQUIRE(ring.available_space() == 7);

    // Test wrap-around write and read
    constexpr std::string_view data_2 = "EFGHJK";
    std::span data2 = std::span{ data_2.data(), data_2.size() };
    REQUIRE(ring.write_as(data2) == std::errc{});
    REQUIRE(ring.size() == 6);
    REQUIRE(ring.available_space() == 1);
    std::array<char, data_2.size()> read_buffer2;
    REQUIRE(ring.read_as(std::span{ read_buffer2 }) == data_2.size());
    REQUIRE(std::memcmp(read_buffer2.data(), data_2.data(), data_2.size()) == 0);
    REQUIRE(ring.size() == 0);
    REQUIRE(ring.available_space() == 7);
}