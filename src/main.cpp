#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <iterator>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;
using State = std::array<std::uint32_t, 8>;

std::uint32_t rotl(std::uint32_t value, unsigned shift)
{
    return std::rotl(value, static_cast<int>(shift));
}

std::uint32_t rotr(std::uint32_t value, unsigned shift)
{
    return std::rotr(value, static_cast<int>(shift));
}

State initialState()
{
    return {
        0x452821E6u, 0x38D01377u, 0xBE5466CFu, 0x34E90C6Cu,
        0xC0AC29B7u, 0xC97C50DDu, 0x3F84D5B5u, 0xB5470917u
    };
}

std::array<std::uint32_t, 64> roundConstants()
{
    return {
        0x243F6A88u, 0x85A308D3u, 0x13198A2Eu, 0x03707344u,
        0xA4093822u, 0x299F31D0u, 0x082EFA98u, 0xEC4E6C89u,
        0x452821E6u, 0x38D01377u, 0xBE5466CFu, 0x34E90C6Cu,
        0xC0AC29B7u, 0xC97C50DDu, 0x3F84D5B5u, 0xB5470917u,
        0x9216D5D9u, 0x8979FB1Bu, 0xD1310BA6u, 0x98DFB5ACu,
        0x2FFD72DBu, 0xD01ADFB7u, 0xB8E1AFEDu, 0x6A267E96u,
        0xBA7C9045u, 0xF12C7F99u, 0x24A19947u, 0xB3916CF7u,
        0x0801F2E2u, 0x858EFC16u, 0x636920D8u, 0x71574E69u,
        0xA458FEA3u, 0xF4933D7Eu, 0x0D95748Fu, 0x728EB658u,
        0x718BCD58u, 0x82154AEEu, 0x7B54A41Du, 0xC25A59B5u,
        0x9C30D539u, 0x2AF26013u, 0xC5D1B023u, 0x286085F0u,
        0xCA417918u, 0xB8DB38EFu, 0x8E79DCB0u, 0x603A180Eu,
        0x6C9E0E8Bu, 0xB01E8A3Eu, 0xD71577C1u, 0xBD314B27u,
        0x78AF2FDAu, 0x55605C60u, 0xE65525F3u, 0xAA55AB94u,
        0x57489862u, 0x63E81440u, 0x55CA396Au, 0x2AAB10B6u,
        0xB4CC5C34u, 0x1141E8CEu, 0xA15486AFu, 0x7C72E993u
    };
}

void pad(Bytes& bytes)
{
    const std::uint64_t bitLength = static_cast<std::uint64_t>(bytes.size()) * 8u;
    bytes.push_back(0x80u);
    while (bytes.size() % 64u != 56u)
        bytes.push_back(0u);
    for (int shift = 56; shift >= 0; shift -= 8)
        bytes.push_back(static_cast<std::uint8_t>(bitLength >> shift));
}

std::uint32_t readWord(const Bytes& bytes, std::size_t offset)
{
    return (static_cast<std::uint32_t>(bytes[offset]) << 24u)
         | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16u)
         | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8u)
         | static_cast<std::uint32_t>(bytes[offset + 3]);
}

std::uint32_t sigmaZero(std::uint32_t value)
{
    return rotl(value, 7) ^ rotl(value, 13) ^ (value << 5)
         ^ ~(rotl(value, 7) ^ rotl(value, 13));
}

std::uint32_t sigmaOne(std::uint32_t value)
{
    return rotr(value, 8) ^ rotr(value, 19) ^ (value >> 5);
}

std::uint32_t bigSigmaZero(std::uint32_t value)
{
    return rotl(value, 3) ^ rotl(value, 14) ^ rotl(value, 25);
}

std::uint32_t bigSigmaOne(std::uint32_t value)
{
    return rotl(value, 5) ^ rotl(value, 11) ^ rotl(value, 27);
}

std::uint32_t choose(std::uint32_t a, std::uint32_t b, std::uint32_t c,
                     std::uint32_t e, std::uint32_t f, std::uint32_t g)
{
    std::uint32_t result = 0;
    for (unsigned bit = 0; bit < 32; ++bit) {
        const bool even = bit % 2u == 0u;
        const std::uint32_t x = (even ? a : e) >> bit & 1u;
        const std::uint32_t y = (even ? b : f) >> bit & 1u;
        const std::uint32_t z = (even ? c : g) >> bit & 1u;
        result |= ((x & y) ^ ((x ^ 1u) & z)) << bit;
    }
    return result;
}

std::uint32_t majority(std::uint32_t a, std::uint32_t b, std::uint32_t c,
                       std::uint32_t d, std::uint32_t e)
{
    std::uint32_t result = 0;
    for (unsigned bit = 0; bit < 32; ++bit) {
        const unsigned count = ((a >> bit) & 1u) + ((b >> bit) & 1u)
                             + ((c >> bit) & 1u) + ((d >> bit) & 1u)
                             + ((e >> bit) & 1u);
        result |= (count >= 3u ? 1u : 0u) << bit;
    }
    return result;
}

State compress(const Bytes& padded)
{
    State state = initialState();
    const auto constants = roundConstants();

    for (std::size_t block = 0; block < padded.size(); block += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i)
            words[i] = readWord(padded, block + i * 4);
        for (std::size_t i = 16; i < words.size(); ++i)
            words[i] = sigmaOne(words[i - 3]) + words[i - 4]
                     + sigmaZero(words[i - 10]) + words[i - 16];

        std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (std::size_t i = 0; i < words.size(); ++i) {
            const std::uint32_t t1 = h + bigSigmaOne(e)
                + choose(a, b, c, e, f, g) + constants[i] + words[i];
            const std::uint32_t t2 = bigSigmaZero(a) + majority(a, b, c, d, e);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
    return state;
}

Bytes hashBytes(const Bytes& input)
{
    Bytes padded = input;
    pad(padded);
    const State state = compress(padded);
    Bytes digest;
    digest.reserve(32);
    for (std::uint32_t word : state) {
        digest.push_back(static_cast<std::uint8_t>(word >> 24));
        digest.push_back(static_cast<std::uint8_t>(word >> 16));
        digest.push_back(static_cast<std::uint8_t>(word >> 8));
        digest.push_back(static_cast<std::uint8_t>(word));
    }
    return digest;
}

std::string toHex(const Bytes& bytes)
{
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (std::uint8_t byte : bytes)
        output << std::setw(2) << static_cast<unsigned>(byte);
    return output.str();
}

Bytes readAll(std::istream& input)
{
    return Bytes(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

Bytes readFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Nepavyko atidaryti failo: " + path);
    return readAll(input);
}

void printUsage(const char* program)
{
    std::cerr << "Naudojimas:\n"
              << "  " << program << " --text TEKSTAS\n"
              << "  " << program << " --file KELIAS\n"
              << "  " << program << " --stdin\n"
              << "  " << program << " --batch\n";
}

int runBatch()
{
    std::uint64_t length = 0;
    while (std::cin.read(reinterpret_cast<char*>(&length), sizeof(length))) {
        Bytes input(length);
        if (!std::cin.read(reinterpret_cast<char*>(input.data()), static_cast<std::streamsize>(length)))
            throw std::runtime_error("Nebaigtas paketinis įrašas.");
        const Bytes digest = hashBytes(input);
        std::cout.write(reinterpret_cast<const char*>(digest.data()),
                        static_cast<std::streamsize>(digest.size()));
    }
    return 0;
}

}

#ifndef HASH_GENERATOR_LIBRARY
int main(int argc, char* argv[])
{
    try {
        if (argc == 2 && std::string(argv[1]) == "--stdin")
            std::cout << toHex(hashBytes(readAll(std::cin))) << '\n';
        else if (argc == 2 && std::string(argv[1]) == "--batch")
            return runBatch();
        else if (argc == 3 && std::string(argv[1]) == "--text")
            std::cout << toHex(hashBytes(Bytes(argv[2], argv[2] + std::strlen(argv[2])))) << '\n';
        else if (argc == 3 && std::string(argv[1]) == "--file")
            std::cout << toHex(hashBytes(readFile(argv[2]))) << '\n';
        else {
            printUsage(argv[0]);
            return 2;
        }
    } catch (const std::exception& error) {
        std::cerr << "Klaida: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
#endif
