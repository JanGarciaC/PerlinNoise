// ─────────────────────────────────────────────────────────────────────────────
//  2D Perlin Noise Generator
//  Outputs greyscale + terrain images in both PPM and PNG formats.
//
//  Dependencies:
//    stb_image_write.h  –  https://github.com/nothings/stb
//
//  Usage:
//    perlin.exe [options]
//
//  Options (all optional, any order):
//    --width   <int>     Image width  in pixels  (default: 512)
//    --height  <int>     Image height in pixels  (default: 512)
//    --seed    <int>     RNG seed                (default: 666)
//    --scale   <float>   Noise zoom level        (default: 4.0)
//    --octaves <int>     fBm octave count        (default: 6)
//    --persist <float>   Amplitude persistence   (default: 0.5)
//    --lacun   <float>   Frequency lacunarity    (default: 2.0)
//    --outdir  <path>    Output directory        (default: .)
//    --help              Print this help and exit
// ─────────────────────────────────────────────────────────────────────────────

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "Header.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────
//  Writers
// ─────────────────────────────────────────────
void writePPM(const fs::path& path, const std::vector<std::vector<Color>>& img, int width, int height) 
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) { std::cerr << "Error: cannot open " << path << "\n"; return; }

    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const Color& c = img[y][x];
            ofs.put(static_cast<char>(c.r));
            ofs.put(static_cast<char>(c.g));
            ofs.put(static_cast<char>(c.b));
        }
    std::cout << "  [PPM] " << path << "\n";
}

void writePNG(const fs::path& path, const std::vector<std::vector<Color>>& img, int width, int height) 
{
    std::vector<uint8_t> buf;
    buf.reserve(static_cast<size_t>(width) * height * 3);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            buf.push_back(img[y][x].r);
            buf.push_back(img[y][x].g);
            buf.push_back(img[y][x].b);
        }

    int ok = stbi_write_png(path.string().c_str(), width, height, 3, buf.data(), width * 3);

    if (ok)
        std::cout << "  [PNG] " << path << "\n";
    else
        std::cerr << "Error: stbi_write_png failed for " << path << "\n";
}

// ─────────────────────────────────────────────
//  Help Option
// ─────────────────────────────────────────────
void printHelp(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [options]\n\n"
        "Options:\n"
        "  --width   <int>    Image width in pixels        (default: 512)\n"
        "  --height  <int>    Image height in pixels       (default: 512)\n"
        "  --seed    <int>    RNG seed                     (default: 666)\n"
        "  --scale   <float>  Noise zoom level             (default: 4.0)\n"
        "  --octaves <int>    fBm octave count             (default: 6)\n"
        "  --persist <float>  Amplitude persistence        (default: 0.5)\n"
        "  --lacun   <float>  Frequency lacunarity         (default: 2.0)\n"
        "  --outdir  <path>   Output directory             (default: .)\n"
        "  --help             Print this help and exit\n\n"
        "Example:\n"
        "  " << prog << " --width 1024 --height 1024 --seed 7 --outdir ./output\n";
}

// ─────────────────────────────────────────────
//  Argument Parsing
// ─────────────────────────────────────────────
Config parseArgs(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i];

        if (flag == "--help" || flag == "-h") { printHelp(argv[0]); std::exit(0); }

        auto nextArg = [&]() -> std::string {
            if (++i >= argc) {
                std::cerr << "Error: " << flag << " requires a value\n";
                std::exit(1);
            }
            return argv[i];
            };

        if (flag == "--width") cfg.width = std::stoi(nextArg());
        else if (flag == "--height") cfg.height = std::stoi(nextArg());
        else if (flag == "--seed") cfg.seed = static_cast<uint32_t>(std::stoi(nextArg()));
        else if (flag == "--scale") cfg.scale = std::stod(nextArg());
        else if (flag == "--octaves") cfg.octaves = std::stoi(nextArg());
        else if (flag == "--persist") cfg.persistence = std::stod(nextArg());
        else if (flag == "--lacun") cfg.lacunarity = std::stod(nextArg());
        else if (flag == "--outdir") cfg.outdir = nextArg();
        else {
            std::cerr << "Unknown option: " << flag << "  (use --help for usage)\n";
            std::exit(1);
        }
    }
    return cfg;
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    Config cfg = parseArgs(argc, argv);

    // Ensure output directory exists
    std::error_code ec;
    fs::create_directories(cfg.outdir, ec);
    if (ec) {
        std::cerr << "Error creating output directory '"
            << cfg.outdir << "': " << ec.message() << "\n";
        return 1;
    }

    std::cout << "\n2D Perlin Noise Generator\n"
        << "─────────────────────────────────────\n"
        << "  size:        " << cfg.width << " x " << cfg.height << "\n"
        << "  seed:        " << cfg.seed << "\n"
        << "  scale:       " << cfg.scale << "\n"
        << "  octaves:     " << cfg.octaves << "\n"
        << "  persistence: " << cfg.persistence << "\n"
        << "  lacunarity:  " << cfg.lacunarity << "\n"
        << "  output dir:  " << fs::absolute(cfg.outdir) << "\n"
        << "─────────────────────────────────────\n\n";

    PerlinNoise pn(cfg.seed);

    std::vector<std::vector<Color>> grey(cfg.height, std::vector<Color>(cfg.width));
    std::vector<std::vector<Color>> terrain(cfg.height, std::vector<Color>(cfg.width));

    for (int y = 0; y < cfg.height; ++y) {
        for (int x = 0; x < cfg.width; ++x) {
            double nx = static_cast<double>(x) / cfg.width * cfg.scale;
            double ny = static_cast<double>(y) / cfg.height * cfg.scale;
            double v = pn.octave(nx, ny, cfg.octaves, cfg.persistence, cfg.lacunarity);
            grey[y][x] = toGreyscale(v);
            terrain[y][x] = toTerrain(v);
        }
    }

    std::cout << "Writing greyscale...\n";
    writePPM(cfg.outdir / "perlin_grey.ppm", grey, cfg.width, cfg.height);
    writePNG(cfg.outdir / "perlin_grey.png", grey, cfg.width, cfg.height);

    std::cout << "\nWriting terrain...\n";
    writePPM(cfg.outdir / "perlin_terrain.ppm", terrain, cfg.width, cfg.height);
    writePNG(cfg.outdir / "perlin_terrain.png", terrain, cfg.width, cfg.height);

    std::cout << "\nDone. 4 files saved to: "
        << fs::absolute(cfg.outdir) << "\n";
    return 0;
}