#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <iomanip>

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
    
    //naudojau ai sitam loop
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


int main()
{
    std::string input = "hello world";
    std::vector<uint8_t> v;
    std::array<uint32_t, 8> hashValues = computeHashValues(); //tik 8 hash values

    toBinary(input, v);
    pad(v);

    for(auto x : v)
        std::cout << std::bitset<8>(x) << std::endl;    
}
