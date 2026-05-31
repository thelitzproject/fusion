#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <iomanip>

static constexpr uint32_t SPIRV_MAGIC = 0x07230203u;

static bool readFile(const std::string& path, std::vector<uint32_t>& out)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    auto bytes = static_cast<size_t>(f.tellg());
    if (bytes < 20 || bytes % 4 != 0) return false;
    f.seekg(0);
    out.resize(bytes / 4);
    f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(bytes));
    return f.good();
}

static const char* stageName(uint16_t executionModel)
{
    switch (executionModel) {
    case 0:  return "Vertex";
    case 1:  return "TessellationControl";
    case 2:  return "TessellationEvaluation";
    case 3:  return "Geometry";
    case 4:  return "Fragment";
    case 5:  return "GLCompute";
    case 6:  return "Kernel";
    default: return "Unknown";
    }
}

// Walk SPIR-V instructions to find EntryPoint opcodes (op 15).
static void printEntryPoints(const std::vector<uint32_t>& words)
{
    size_t i = 5; // skip header
    while (i < words.size()) {
        uint32_t word    = words[i];
        uint16_t opcode  = static_cast<uint16_t>(word & 0xFFFF);
        uint16_t wordLen = static_cast<uint16_t>(word >> 16);
        if (wordLen == 0) break;
        if (opcode == 15 && i + 3 < words.size()) { // OpEntryPoint
            uint16_t model = static_cast<uint16_t>(words[i + 1] & 0xFFFF);
            // name starts at word i+3 as a UTF-8 string packed into uint32_ts
            std::string name;
            for (uint16_t j = 3; j < wordLen; ++j) {
                uint32_t w = words[i + j];
                for (int b = 0; b < 4; ++b) {
                    char c = static_cast<char>((w >> (b * 8)) & 0xFF);
                    if (!c) goto done;
                    name += c;
                }
            }
            done:
            std::cout << "    EntryPoint: " << stageName(model) << " \"" << name << "\"\n";
        }
        i += wordLen;
    }
}

static int inspect(const std::string& path)
{
    std::vector<uint32_t> words;
    if (!readFile(path, words)) {
        std::cerr << "shadertool: cannot read '" << path << "'\n";
        return 1;
    }

    if (words[0] != SPIRV_MAGIC) {
        uint32_t swapped = ((words[0] & 0xFF) << 24) | (((words[0] >> 8) & 0xFF) << 16) |
                           (((words[0] >> 16) & 0xFF) << 8) | ((words[0] >> 24) & 0xFF);
        if (swapped == SPIRV_MAGIC) {
            std::cerr << path << ": big-endian SPIR-V (unsupported)\n";
        } else {
            std::cerr << path << ": not a SPIR-V binary (bad magic 0x"
                      << std::hex << words[0] << ")\n";
        }
        return 1;
    }

    uint32_t ver       = words[1];
    uint32_t generator = words[2];
    uint32_t bound     = words[3];

    std::cout << path << ":\n"
              << "  SPIR-V " << ((ver >> 16) & 0xFF) << "." << ((ver >> 8) & 0xFF) << "\n"
              << "  generator:  0x" << std::hex << std::setw(8) << std::setfill('0')
                                    << generator << std::dec << "\n"
              << "  id bound:   " << bound << "\n"
              << "  word count: " << words.size()
              << " (" << (words.size() * 4) << " bytes)\n";
    printEntryPoints(words);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: shadertool <file.spv> [...]\n";
        return EXIT_FAILURE;
    }
    int rc = 0;
    for (int i = 1; i < argc; ++i)
        rc |= inspect(argv[i]);
    return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
