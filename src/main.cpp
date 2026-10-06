#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <bit>

void toBinary(std::string& in, std::vector<uint8_t>& out)
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


uint32_t sigmaZero(uint32_t messageBlock) //input 4 byte block
{
    uint32_t a = messageBlock;
    uint32_t b = messageBlock;
    uint32_t c = messageBlock;
    //left rotate 7 times

    //left rotate 13 times

    //left shift 5 times

    //XNOR (ekvivalencija)
    uint32_t result = 0;
    return result;
}

uint32_t sigmaOne(uint32_t messageBlock)
{
//pakeisti tik skaiciai nuo sha256
    uint32_t a = messageBlock;
    uint32_t b = messageBlock;
    uint32_t c = messageBlock;  
//right rotate 8 times

//right rotate 19 times

//right shift 5

//XOR naudojamas
    uint32_t result = 0;
    return result;
}
void messageSchedule(std::vector<uint8_t>& vec, int blockCount)
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
    for(int i = 0; i < blockCount; i++)
    {

    }
}   

uint32_t choose(uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f, uint32_t g)
{
    //5 inputai
    for(std::size_t i = 0; i < 32; i++)
    {
        if(i % 2 == 0)
        {
            //a, b, c naudot choose'ui
        }
        else
        {
            //e, f, g naudoti

        }
    }   
}

uint32_t majority(uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f, uint32_t g)
{
    //majority is 5 inputu
}
void compression(std::vector<uint8_t>& vec)
{

}



//final hash
std::vector<uint8_t> hash(std::string& input)
{

}

int main()
{
    /*
    TODO
    Add CIN input

    */
    std::string input = "aaaaaaaaaaaaaaaaaasardrydtfyguguguifyfyhftffhdjnajdnjandjnsajdnasjdajsdnjasdnjsandjasjdnasjdasojdjaosjdahdiojaidjasidjaidohfiofiwdiadpi3qidqpidaisjdiqjdi2dioasdioqjdoajdsodiapwjdiasjdikjwidjaskdnijdijaijediqgyffyfiftfytoftorftftftfotuftoftuftuftftufuyfyfgfyyuut";
    std::vector<uint8_t> v;
    int blockCount = v.size() / 64;
    std::array<uint32_t, 8> hashValues = computeHashValues(); //tik 8 hash values

    toBinary(input, v);
    pad(v);

    for(auto x : v)
        std::cout << std::bitset<8>(x) << std::endl;    
}
