#define main original_hash_main
#include "../src/main.cpp"
#undef main

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string decodeHex(const std::string& text)
{
    std::string result;
    for (std::size_t i = 0; i < text.size(); i += 2)
        result.push_back(static_cast<char>(std::stoi(text.substr(i, 2), nullptr, 16)));
    return result;
}

std::string encodeHex(const std::vector<std::uint8_t>& bytes)
{
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (std::uint8_t byte : bytes)
        result << std::setw(2) << static_cast<unsigned>(byte);
    return result.str();
}

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--benchmark") {
        std::string line;
        std::vector<std::string> inputs;
        while (std::getline(std::cin, line))
            inputs.push_back(decodeHex(line));

        volatile std::uint8_t sink = 0;
        const auto start = std::chrono::steady_clock::now();
        for (const std::string& input : inputs)
            sink ^= hash(input).front();
        const auto end = std::chrono::steady_clock::now();
        const double microseconds =
            std::chrono::duration<double, std::micro>(end - start).count();
        std::cout << microseconds << ',' << static_cast<unsigned>(sink) << '\n';
        return 0;
    }

    std::string line;
    while (std::getline(std::cin, line))
        std::cout << encodeHex(hash(decodeHex(line))) << '\n';
}
