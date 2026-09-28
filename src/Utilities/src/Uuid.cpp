#include "Utilities/Uuid.h"

#include <array>
#include <cstdint>
#include <random>

namespace noctis::uuid {

namespace {

std::mt19937_64& engine() {
    // Sembrado una sola vez por hilo: generar notas en lote no debe repetir ids.
    thread_local std::mt19937_64 engine{std::random_device{}()};
    return engine;
}

void appendHex(std::string& out, std::uint8_t byte) {
    constexpr char kDigits[] = "0123456789abcdef";
    out.push_back(kDigits[byte >> 4]);
    out.push_back(kDigits[byte & 0x0F]);
}

} // namespace

std::string generate() {
    std::array<std::uint8_t, 16> bytes{};

    std::uniform_int_distribution<std::uint64_t> distribution;
    const std::uint64_t high = distribution(engine());
    const std::uint64_t low = distribution(engine());

    for (std::size_t i = 0; i < 8; ++i) {
        bytes[i] = static_cast<std::uint8_t>(high >> (8 * i));
        bytes[i + 8] = static_cast<std::uint8_t>(low >> (8 * i));
    }

    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0F) | 0x40); // versión 4
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3F) | 0x80); // variante RFC 4122

    std::string result;
    result.reserve(36);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) result.push_back('-');
        appendHex(result, bytes[i]);
    }
    return result;
}

} // namespace noctis::uuid
