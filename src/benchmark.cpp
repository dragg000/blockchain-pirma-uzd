#define HASH_GENERATOR_LIBRARY
#include "main.cpp"

#include <chrono>
#include <algorithm>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Naudojimas: benchmark KELIAS\n";
        return 2;
    }

    const Bytes input = readFile(argv[1]);
    for (int i = 0; i < 3; ++i)
        static_cast<void>(hashBytes(input));

    constexpr int repetitions = 5;
    double total = 0.0;
    double minimum = 1e100;
    double maximum = 0.0;
    volatile std::uint8_t sink = 0;

    for (int i = 0; i < repetitions; ++i) {
        const auto start = std::chrono::steady_clock::now();
        const Bytes digest = hashBytes(input);
        const auto end = std::chrono::steady_clock::now();
        sink ^= digest.front();
        const double microseconds =
            std::chrono::duration<double, std::micro>(end - start).count();
        total += microseconds;
        minimum = std::min(minimum, microseconds);
        maximum = std::max(maximum, microseconds);
    }

    std::cout << input.size() << ',' << total / repetitions << ','
              << minimum << ',' << maximum << ',' << static_cast<unsigned>(sink) << '\n';
}
