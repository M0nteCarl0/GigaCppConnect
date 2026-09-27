#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace GigaCpp {

class Base64 {
public:
    static std::string Encode(std::string_view input) {
        static constexpr char kTable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string output;
        const size_t in_len = input.size();
        output.reserve(((in_len + 2) / 3) * 4);

        size_t i = 0;
        while (i + 2 < in_len) {
            uint32_t triple = (static_cast<uint8_t>(input[i]) << 16) |
                              (static_cast<uint8_t>(input[i + 1]) << 8) |
                              static_cast<uint8_t>(input[i + 2]);
            output.push_back(kTable[(triple >> 18) & 0x3F]);
            output.push_back(kTable[(triple >> 12) & 0x3F]);
            output.push_back(kTable[(triple >> 6) & 0x3F]);
            output.push_back(kTable[triple & 0x3F]);
            i += 3;
        }

        if (i < in_len) {
            uint32_t b0 = static_cast<uint8_t>(input[i]);
            uint32_t b1 = (i + 1 < in_len) ? static_cast<uint8_t>(input[i + 1]) : 0;
            uint32_t triple = (b0 << 16) | (b1 << 8);

            output.push_back(kTable[(triple >> 18) & 0x3F]);
            output.push_back(kTable[(triple >> 12) & 0x3F]);
            if (i + 1 < in_len) {
                output.push_back(kTable[(triple >> 6) & 0x3F]);
                output.push_back('=');
            } else {
                output.push_back('=');
                output.push_back('=');
            }
        }
        return output;
    }

    static std::string Decode(std::string_view input) {
        static const auto kDecodeTable = []() {
            std::vector<int> table(256, -1);
            static constexpr char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            for (int idx = 0; idx < 64; ++idx) {
                table[static_cast<uint8_t>(chars[idx])] = idx;
            }
            return table;
        }();

        std::string output;
        uint32_t buffer = 0;
        int bits_collected = 0;

        for (char c : input) {
            if (c == '=') break;
            int val = kDecodeTable[static_cast<uint8_t>(c)];
            if (val == -1) continue; // ignore whitespace / newline

            buffer = (buffer << 6) | val;
            bits_collected += 6;

            if (bits_collected >= 8) {
                bits_collected -= 8;
                output.push_back(static_cast<char>((buffer >> bits_collected) & 0xFF));
            }
        }

        return output;
    }
};

} // namespace GigaCpp
