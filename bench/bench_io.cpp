#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>
#include <filesystem>
#include <iomanip>

static void benchIO(size_t bytes)
{
    namespace fs = std::filesystem;
    namespace ch = std::chrono;

    std::vector<char> data(bytes);
    {
        std::mt19937 rng(0xDEADBEEF);
        std::uniform_int_distribution<unsigned> dist(0, 255);
        for (auto& b : data) b = static_cast<char>(dist(rng));
    }

    fs::path tmp = fs::temp_directory_path() / "fusion_bench_cache.bin";

    auto t0 = ch::high_resolution_clock::now();
    {
        std::ofstream f(tmp, std::ios::binary);
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
    auto t1 = ch::high_resolution_clock::now();

    std::vector<char> buf;
    auto t2 = ch::high_resolution_clock::now();
    {
        std::ifstream f(tmp, std::ios::binary | std::ios::ate);
        buf.resize(static_cast<size_t>(f.tellg()));
        f.seekg(0);
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    }
    auto t3 = ch::high_resolution_clock::now();

    fs::remove(tmp);

    double writeSec = ch::duration<double>(t1 - t0).count();
    double readSec  = ch::duration<double>(t3 - t2).count();
    double mb       = bytes / 1048576.0;

    std::cout << std::fixed << std::setprecision(2)
              << "  " << std::setw(6) << (bytes / 1024) << " KB"
              << "  write: " << std::setw(6) << (writeSec * 1000.0) << " ms"
              << "  (" << std::setw(7) << (mb / writeSec) << " MB/s)"
              << "  read: "  << std::setw(6) << (readSec  * 1000.0) << " ms"
              << "  (" << std::setw(7) << (mb / readSec)  << " MB/s)\n";
}

int main()
{
    std::cout << "=== fusion bench: binary cache I/O ===\n";
    for (size_t kb : { 64u, 256u, 1024u, 4096u })
        benchIO(kb * 1024);
    return 0;
}
