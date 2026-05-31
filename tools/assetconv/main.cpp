#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
#include <cstdlib>

// Raw asset format: [uint32 width][uint32 height][uint8 * width * height * 4 RGBA bytes]
static void writeRaw(const std::string& path, const stbi_uc* pixels, int w, int h)
{
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "assetconv: cannot write '" << path << "'\n";
        return;
    }
    uint32_t uw = static_cast<uint32_t>(w);
    uint32_t uh = static_cast<uint32_t>(h);
    f.write(reinterpret_cast<const char*>(&uw), 4);
    f.write(reinterpret_cast<const char*>(&uh), 4);
    f.write(reinterpret_cast<const char*>(pixels), static_cast<std::streamsize>(w) * h * 4);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: assetconv <input.png|jpg|bmp|tga> [output.raw]\n"
                  << "  If no output is given, only image info is printed.\n"
                  << "  Output format: uint32 width, uint32 height, RGBA bytes.\n";
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        const char* input = argv[i];

        // Treat pairs: input output (if the next arg looks like .raw)
        const char* output = nullptr;
        if (i + 1 < argc) {
            std::string next = argv[i + 1];
            if (next.size() >= 4 && next.rfind(".raw") == next.size() - 4) {
                output = argv[i + 1];
                ++i;
            }
        }

        int w = 0, h = 0, ch = 0;
        stbi_uc* pixels = stbi_load(input, &w, &h, &ch, STBI_rgb_alpha);
        if (!pixels) {
            std::cerr << "assetconv: failed to load '" << input << "': "
                      << stbi_failure_reason() << "\n";
            continue;
        }

        std::cout << "file:     " << input << "\n"
                  << "size:     " << w << " x " << h << "\n"
                  << "channels: " << ch << " (loaded as RGBA)\n"
                  << "bytes:    " << (w * h * 4) << "\n";

        if (output) {
            writeRaw(output, pixels, w, h);
            std::cout << "written:  " << output
                      << " (" << (8 + w * h * 4) << " bytes)\n";
        }

        stbi_image_free(pixels);
    }

    return EXIT_SUCCESS;
}
