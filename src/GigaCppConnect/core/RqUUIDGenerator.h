#pragma once

#include <string>
#include <random>
#include <regex>

namespace GigaCpp {

class RqUUIDGenerator {
public:
    static std::string GenerateRqUUID() {
        // Generates an RFC 4122 v4 compliant UUID
        thread_local std::random_device rd;
        thread_local std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;

        uint64_t part1 = dis(gen);
        uint64_t part2 = dis(gen);

        // Version 4: set bits 12-15 of time_hi_and_version to 0100 (4)
        part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;

        // Variant 1: set bits 6-7 of clock_seq_hi_and_reserved to 10
        part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

        char buffer[37];
        snprintf(buffer, sizeof(buffer),
                 "%08x-%04x-%04x-%02x%02x-%012llx",
                 static_cast<uint32_t>((part1 >> 32) & 0xFFFFFFFF),
                 static_cast<uint16_t>((part1 >> 16) & 0xFFFF),
                 static_cast<uint16_t>(part1 & 0xFFFF),
                 static_cast<uint8_t>((part2 >> 56) & 0xFF),
                 static_cast<uint8_t>((part2 >> 48) & 0xFF),
                 part2 & 0xFFFFFFFFFFFFULL);

        return std::string(buffer, 36);
    }
};

} // namespace GigaCpp

// Backwards compatibility at global namespace
using RqUUIDGenerator = GigaCpp::RqUUIDGenerator;
