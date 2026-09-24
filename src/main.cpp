#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

static inline u32 rotl32(u32 x, int r) {
    return (x << r) | (x >> (32 - r));
}

static constexpr std::array<u32, 8> IV = {
    0x8F1B2C3Du, 0x4A7A5F1Cu, 0xD6E7F8A1u, 0x92C17D4Eu,
    0xB9E6A4D2u, 0x5C41F0B7u, 0x76D1AE2Cu, 0xE9C4B5D8u
};

static std::vector<u8> toBytes(const std::string& text) {
    return std::vector<u8>(text.begin(), text.end());
}

static std::vector<u8> readFileBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Unreadable file: " + path);
    }
    return std::vector<u8>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static std::vector<u8> padBytes(std::vector<u8> message) {
    const u64 originalBits = static_cast<u64>(message.size()) * 8ULL;
    message.push_back(0x80u);
    while ((message.size() % 64u) != 56u) {
        message.push_back(0x00u);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<u8>((originalBits >> shift) & 0xFFu));
    }
    return message;
}

static std::array<u32, 8> mixBlock(const std::array<u32, 8>& state, const std::array<u32, 16>& words) {
    std::array<u32, 8> tmp = state;
    for (int round = 0; round < 7; ++round) {
        for (int i = 0; i < 8; ++i) {
            u32 a = tmp[i];
            u32 b = words[(i + round * 3) % 16];
            u32 c = tmp[(i + 3) % 8];
            u32 d = words[(i * 5 + round) % 16];
            u32 x = (b ^ d) + c + (u32(round + 1) * 0x9E3779B9u);
            a ^= rotl32(x, 9 + (i % 5));
            a = rotl32(a + tmp[(i + 5) % 8] + words[(i + round * 2) % 16], 11 + (round % 4));
            tmp[(i + 1) % 8] ^= rotl32(a + words[(i * 7 + round) % 16], 7 + (round % 3));
            tmp[i] = a;
        }
    }

    std::array<u32, 8> out = state;
    for (int i = 0; i < 8; ++i) {
        out[i] ^= rotl32(tmp[(i + 2) % 8] + words[(i * 5 + 3) % 16] + 0xC2B2AE3Du, 13 + (i % 5));
        out[i] += tmp[i] ^ words[(i * 3 + 2) % 16];
    }
    return out;
}

static std::vector<u8> hashData(const std::vector<u8>& input) {
    std::vector<u8> padded = padBytes(input);
    std::array<u32, 8> state = IV;

    for (std::size_t block = 0; block < padded.size(); block += 64) {
        std::array<u32, 16> words{};
        for (int i = 0; i < 16; ++i) {
            std::size_t idx = block + i * 4;
            words[i] = (u32(padded[idx]) |
                        (u32(padded[idx + 1]) << 8) |
                        (u32(padded[idx + 2]) << 16) |
                        (u32(padded[idx + 3]) << 24));
        }

        std::array<u32, 8> mixed = mixBlock(state, words);
        for (int i = 0; i < 8; ++i) {
            state[i] = mixed[i] ^ rotl32(state[(i + 4) % 8] + words[(i * 3 + 1) % 16] + 0xA5A5A5A5u, 17 + i);
        }
    }

    std::vector<u8> digest(32, 0);
    for (int i = 0; i < 8; ++i) {
        u32 word = state[i];
        digest[i * 4 + 0] = static_cast<u8>(word & 0xFFu);
        digest[i * 4 + 1] = static_cast<u8>((word >> 8) & 0xFFu);
        digest[i * 4 + 2] = static_cast<u8>((word >> 16) & 0xFFu);
        digest[i * 4 + 3] = static_cast<u8>((word >> 24) & 0xFFu);
    }
    return digest;
}

static std::string bytesToHex(const std::vector<u8>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (u8 b : bytes) {
        oss << std::setw(2) << static_cast<unsigned int>(b);
    }
    return oss.str();
}

static std::string hashTextUtf8(const std::string& text) {
    return bytesToHex(hashData(toBytes(text)));
}

static std::string hashFile(const std::string& path) {
    return bytesToHex(hashData(readFileBytes(path)));
}

static std::string randomAlphaString(std::mt19937_64& rng, std::size_t len) {
    static const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
    std::uniform_int_distribution<int> dist(0, static_cast<int>(alphabet.size()) - 1);
    std::string out;
    out.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(alphabet[dist(rng)]);
    }
    return out;
}

static long long bitDiffBytes(const std::vector<u8>& a, const std::vector<u8>& b) {
    long long count = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        count += __builtin_popcount(static_cast<unsigned int>(a[i] ^ b[i]));
    }
    return count;
}

static double hexDiffPercent(const std::vector<u8>& a, const std::vector<u8>& b) {
    int differing = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            ++differing;
        }
    }
    return 100.0 * differing / static_cast<double>(a.size());
}

static double bitDiffPercent(const std::vector<u8>& a, const std::vector<u8>& b) {
    long long diffBits = bitDiffBytes(a, b);
    return 100.0 * diffBits / static_cast<double>(a.size() * 8);
}

static void printUsage(const char* argv0) {
    std::cout << "Usage:\n"
              << "  " << argv0 << " --text \"hello world\"\n"
              << "  " << argv0 << " --file /path/to/file\n"
              << "  " << argv0 << " --benchmark\n"
              << "  " << argv0 << " --collision\n"
              << "  " << argv0 << " --avalanche\n"
              << "  " << argv0 << " --guessing\n"
              << "  " << argv0 << " --experiments\n"
              << "  " << argv0 << " --help\n";
}

static void runCollisionExperiment() {
    std::cout << "=== Collision search ===\n";
    std::vector<int> lengths = {10, 100, 500, 1000};
    std::mt19937_64 rng(0xC0FFEE1234ULL);

    for (int len : lengths) {
        std::size_t collisions = 0;
        const std::size_t pairsToCheck = 100000;
        std::vector<std::string> generated;
        generated.reserve(2000);

        for (std::size_t i = 0; i < 2000; ++i) {
            generated.push_back(randomAlphaString(rng, len));
        }

        for (std::size_t i = 0; i < pairsToCheck; ++i) {
            std::string a = randomAlphaString(rng, len);
            std::string b = randomAlphaString(rng, len);
            while (a == b) {
                b = randomAlphaString(rng, len);
            }
            if (hashTextUtf8(a) == hashTextUtf8(b)) {
                ++collisions;
            }
        }

        std::set<std::string> seen;
        for (const auto& s : generated) {
            const std::string h = hashTextUtf8(s);
            if (!seen.insert(h).second) {
                ++collisions;
            }
        }

        std::cout << "length=" << len << " random pairs checked=100000, collisions=" << collisions
                  << ", distinct generated inputs=" << generated.size() << "\n";
    }
}

static void runAvalancheExperiment() {
    std::cout << "=== Avalanche test ===\n";
    std::vector<int> lengths = {10, 100, 500, 1000};
    std::mt19937_64 rng(0xA1B2C3D4E5F6ULL);
    static const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int len : lengths) {
        std::vector<double> bitPercent;
        std::vector<double> hexPercent;
        bitPercent.reserve(25000);
        hexPercent.reserve(25000);

        for (int i = 0; i < 25000; ++i) {
            std::string a = randomAlphaString(rng, len);
            std::string b = a;
            std::uniform_int_distribution<int> posDist(0, static_cast<int>(len) - 1);
            std::uniform_int_distribution<int> charDist(0, static_cast<int>(alphabet.size()) - 1);
            int pos = posDist(rng);
            char ch = alphabet[charDist(rng)];
            while (b[pos] == ch) {
                ch = alphabet[charDist(rng)];
            }
            b[pos] = ch;

            auto ah = hashData(toBytes(a));
            auto bh = hashData(toBytes(b));
            bitPercent.push_back(bitDiffPercent(ah, bh));
            hexPercent.push_back(hexDiffPercent(ah, bh));
        }

        auto [bitMin, bitMax, bitMean] = [&]() {
            double minv = 1e9, maxv = -1e9, sum = 0.0;
            for (double v : bitPercent) {
                minv = std::min(minv, v);
                maxv = std::max(maxv, v);
                sum += v;
            }
            return std::tuple<double, double, double>{minv, maxv, sum / bitPercent.size()};
        }();

        auto [hexMin, hexMax, hexMean] = [&]() {
            double minv = 1e9, maxv = -1e9, sum = 0.0;
            for (double v : hexPercent) {
                minv = std::min(minv, v);
                maxv = std::max(maxv, v);
                sum += v;
            }
            return std::tuple<double, double, double>{minv, maxv, sum / hexPercent.size()};
        }();

        std::cout << "length=" << len << " bit diff: min=" << bitMin << "% max=" << bitMax << "% mean=" << bitMean << "%\n";
        std::cout << "length=" << len << " hex diff: min=" << hexMin << "% max=" << hexMax << "% mean=" << hexMean << "%\n";
    }
}

static void runGuessingDemo() {
    std::cout << "=== Guessing attack demo ===\n";
    const std::string target = "4321";
    std::vector<std::string> candidates;
    candidates.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << i;
        candidates.push_back(oss.str());
    }

    std::string targetHash = hashTextUtf8(target);
    std::cout << "Target input: " << target << "\nTarget hash: " << targetHash << "\n";

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (hashTextUtf8(candidates[i]) == targetHash) {
            std::cout << "Bruteforce found candidate at position " << i << ": " << candidates[i] << "\n";
            break;
        }
    }

    const std::string salt = "s3cret-salt";
    std::string saltedTarget = hashTextUtf8(target + salt);
    std::cout << "Public salt target hash: " << saltedTarget << "\n";
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (hashTextUtf8(candidates[i] + salt) == saltedTarget) {
            std::cout << "Salted brute force found candidate at position " << i << ": " << candidates[i] << "\n";
            break;
        }
    }
}

static void runBenchmark() {
    std::cout << "=== Benchmark ===\n";
    std::string corpus;
    for (int i = 0; i < 2048; ++i) {
        corpus += "line " + std::to_string(i) + " : this is a reproducible UTF-8 benchmark row\n";
    }

    std::vector<std::size_t> sizes = {64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
    for (std::size_t target : sizes) {
        std::vector<u8> data(corpus.begin(), corpus.begin() + std::min<std::size_t>(target, corpus.size()));
        for (int warm = 0; warm < 5; ++warm) {
            hashData(data);
        }

        std::vector<double> samples;
        for (int run = 0; run < 10; ++run) {
            auto begin = std::chrono::steady_clock::now();
            for (int inner = 0; inner < 200; ++inner) {
                auto digest = hashData(data);
                if (digest.size() != 32) {
                    throw std::runtime_error("Invalid digest length");
                }
            }
            auto end = std::chrono::steady_clock::now();
            auto us = std::chrono::duration<double, std::micro>(end - begin).count();
            samples.push_back(us / 200.0);
        }

        double minv = *std::min_element(samples.begin(), samples.end());
        double maxv = *std::max_element(samples.begin(), samples.end());
        double mean = 0.0;
        for (double v : samples) mean += v;
        mean /= static_cast<double>(samples.size());

        std::cout << "size=" << target << " bytes mean=" << std::fixed << std::setprecision(3) << mean
                  << "us min=" << minv << "us max=" << maxv << "us (10 runs)\n";
    }
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::string input;
            std::cout << "Manual text mode: enter text and press Enter\n";
            std::getline(std::cin, input);
            std::cout << hashTextUtf8(input) << "\n";
            return 0;
        }

        std::string mode = argv[1];
        if (mode == "--help" || mode == "-h") {
            printUsage(argv[0]);
            return 0;
        }

        if (mode == "--text") {
            if (argc < 3) {
                throw std::runtime_error("Missing text argument for --text");
            }
            std::string text = argv[2];
            std::cout << hashTextUtf8(text) << "\n";
            return 0;
        }

        if (mode == "--file") {
            if (argc < 3) {
                throw std::runtime_error("Missing path for --file");
            }
            std::string path = argv[2];
            std::cout << hashFile(path) << "\n";
            return 0;
        }

        if (mode == "--benchmark") {
            runBenchmark();
            return 0;
        }

        if (mode == "--collision") {
            runCollisionExperiment();
            return 0;
        }

        if (mode == "--avalanche") {
            runAvalancheExperiment();
            return 0;
        }

        if (mode == "--guessing") {
            runGuessingDemo();
            return 0;
        }

        if (mode == "--experiments") {
            runCollisionExperiment();
            runAvalancheExperiment();
            runGuessingDemo();
            runBenchmark();
            return 0;
        }

        printUsage(argv[0]);
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
}
