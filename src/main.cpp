#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <bit>

void toBinary(const std::string& in, std::vector<uint8_t>& out)
{
    for (std::size_t i = 0; i < in.size(); ++i)
    {
        uint8_t tmp = in[i];
        out.push_back(tmp);
    }
}

void pad(std::vector<uint8_t>& in)
{
    uint64_t len = in.size();

    //add a 10000000
    in.push_back(0x80);

    //pad till mod 512 - 64
    while(in.size() % 64 != 56)
        in.push_back(0x00);
    
    
    for (int i = 7; i >= 0; --i)
        in.push_back(static_cast<uint8_t>(len >> (i * 8)));
}

std::array<uint32_t, 8> computeHashValues()
{
    //iprastai naudojama sha256 pirmi 8 prime numeriai: 2 - 9
    //cia naudosiu 9 - 17: 23 - 59
    const std::array<double, 8> primes = {23, 29, 31, 37, 41, 43, 47, 53};

    std::array<uint32_t, 8> hash;

    for (std::size_t i = 0; i < primes.size(); i++)
    {
        double intPart;
        double frac = std::modf(std::sqrt(primes[i]), &intPart);

        hash[i] = static_cast<uint32_t>(std::floor(frac * 4294967296.0));
    }

    return hash;
}

std::array<uint32_t, 64> computeRoundConstants()
{
    const std::array<uint32_t, 64> primes = {
        2,   3,   5,   7,  11,  13,  17,  19,
        23,  29,  31,  37,  41,  43,  47,  53,
        59,  61,  67,  71,  73,  79,  83,  89,
        97, 101, 103, 107, 109, 113, 127, 131,
        137, 139, 149, 151, 157, 163, 167, 173,
        179, 181, 191, 193, 197, 199, 211, 223,
        227, 229, 233, 239, 241, 251, 257, 263,
        269, 271, 277, 281, 283, 293, 307, 311
    };

    std::array<uint32_t, 64> k;

    for (std::size_t i = 0; i < primes.size(); i++)
    {
        double intPart;
        double frac = std::modf(std::cbrt(static_cast<double>(primes[i])), &intPart);

        k[i] = static_cast<uint32_t>(std::floor(frac * 4294967296.0));
    }

    return k;
}


inline uint32_t rotl(uint32_t x, int n) { return (x << n) | (x >> (32 - n));}

inline uint32_t xnor(uint32_t x, uint32_t y){ return ~(x ^ y); }

inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

inline uint32_t bigSigmaZero(uint32_t x) { return rotl(x, 3) ^ rotl(x, 14) ^ rotl(x, 25); }

inline uint32_t bigSigmaOne(uint32_t x) { return rotl(x, 5) ^ rotl(x, 11) ^ rotl(x, 27); }


uint32_t sigmaZero(uint32_t messageBlock) //input 4 byte block
{
    uint32_t a = messageBlock;
    uint32_t b = messageBlock;
    uint32_t c = messageBlock;
    //left rotate 7 times
    a = rotl(a, 7);

    //left rotate 13 times
    b = rotl(b, 13);

    //left shift 5 times
    c = c << 5;

    //XNOR (ekvivalencija)
    uint32_t result = xnor(xnor(a, b), c);
    return result;
}

uint32_t sigmaOne(uint32_t messageBlock)
{
//pakeisti tik skaiciai nuo sha256
    uint32_t a = messageBlock;
    uint32_t b = messageBlock;
    uint32_t c = messageBlock;  
//right rotate 8 times
    a = rotr(a, 8);

//right rotate 19 times
    b = rotr(b, 19);

//right shift 5
    c = c >> 5;

//XOR naudojamas
    uint32_t result = a ^ b ^ c;
    return result;
}

std::vector<uint32_t> messageSchedule(std::vector<uint8_t>& vec, int blockCount)
{
    //suskirtsom zinute i message blocks M - po 32 bitus
    std::vector<uint32_t> messageBlocks(blockCount * 16);

    for (std::size_t i = 0; i < messageBlocks.size(); i++)
    {
        messageBlocks[i] = (uint32_t(vec[4 * i])     << 24) |
                           (uint32_t(vec[4 * i + 1]) << 16) |
                           (uint32_t(vec[4 * i + 2]) << 8)  |
                            uint32_t(vec[4 * i + 3]);
    }

    //pakeiciau sigma0 ir sigma1 funkcijas
    //Wt = sigmaOne(Wt-2) + Wt-7 + sigmaZero(Wt-15) + Wt-16 - sha256 formula
    //mano Wt = sigmaOne(Wt-3) + Wt-4 + sigmaZero(Wt-10) + Wt-16 - mano formula
    std::vector<uint32_t> W(blockCount * 64);

    for (int i = 0; i < blockCount; i++)
    {
        uint32_t* w = &W[i * 64];
        const uint32_t* m = &messageBlocks[i * 16];

        // first 16 words are the raw message block
        for (int t = 0; t < 16; t++)
            w[t] = m[t];

        // expand 16 -> 64 words
        for (int t = 16; t < 64; t++)
            w[t] = sigmaOne(w[t - 3]) + w[t - 4] + sigmaZero(w[t - 10]) + w[t - 16];
    }

    return W;
}

uint32_t choose(uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f, uint32_t g)
{
    //6 inputai
    uint32_t result = 0;

    for (std::size_t i = 0; i < 32; i++)
    {
        uint32_t x, y, z;

        if (i % 2 == 0)
        {
            //a, b, c naudot choose'ui
            x = (a >> i) & 1;
            y = (b >> i) & 1;
            z = (c >> i) & 1;
        }
        else
        {
            //e, f, g naudoti
            x = (e >> i) & 1;
            y = (f >> i) & 1;
            z = (g >> i) & 1;
        }

        // Ch on single bits: x picks y if x = 1, otherwise z
        uint32_t bit = ((x & y) ^ (~x & z)) & 1;

        result |= bit << i;
    }

    return result;
}

uint32_t majority(uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f)
{
    //majority is 5 inputu
    uint32_t result = 0;

    for (std::size_t i = 0; i < 32; i++)
    {
        uint32_t count = ((a >> i) & 1) + ((b >> i) & 1) + ((c >> i) & 1)
                        + ((e >> i) & 1) + ((f >> i) & 1);

        uint32_t bit = (count >= 3) ? 1 : 0;
        result |= bit << i;
    }

    return result;
}

std::array<uint32_t, 8> compression(const std::vector<uint32_t>& W, int blockCount)
{
    std::array<uint32_t, 8> H = computeHashValues();
    const std::array<uint32_t, 64> K = computeRoundConstants();

    for (int blk = 0; blk < blockCount; blk++)
    {
        const uint32_t* w = &W[blk * 64];

        // working variables start from the current state
        uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
        uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

        for (int t = 0; t < 64; t++)
        {
            // T1
            uint32_t T1 = h + bigSigmaOne(e) + choose(a, b, c, e, f, g) + K[t] + w[t];

            // T2
            uint32_t T2 = bigSigmaZero(a) + majority(a, b, c, d, e);

            // shift the registers; only a and e get new values
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        // feed-forward: add the working variables back into the state
        H[0] += a; H[1] += b; H[2] += c; H[3] += d;
        H[4] += e; H[5] += f; H[6] += g; H[7] += h;
    }

    return H;
}



//final hash
std::vector<uint8_t> hash(const std::string& input)
{
    std::vector<uint8_t> bytes;
    toBinary(input, bytes);
    pad(bytes);

    int blockCount = static_cast<int>(bytes.size() / 64);

    std::vector<uint32_t> W = messageSchedule(bytes, blockCount);
    std::array<uint32_t, 8> H = compression(W, blockCount);

    // state words -> 32 bytes, big-endian (same order as you read the input)
    std::vector<uint8_t> digest;
    digest.reserve(32);

    for (uint32_t word : H)
    {
        digest.push_back(static_cast<uint8_t>(word >> 24));
        digest.push_back(static_cast<uint8_t>(word >> 16));
        digest.push_back(static_cast<uint8_t>(word >> 8));
        digest.push_back(static_cast<uint8_t>(word));
    }

    return digest;

}

int main()
{

    std::vector<uint8_t> d = hash("hello");

    for (uint8_t byte : d)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << int(byte);
    std::cout << '\n';

    return 0;
}
