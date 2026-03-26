#include <numeric>
#include <random>
#include <filesystem>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────
//  Perlin Noise implementation
// ─────────────────────────────────────────────
class PerlinNoise 
{
public:
    explicit PerlinNoise(uint32_t seed = 666) 
    {
        p_.resize(256);
        std::iota(p_.begin(), p_.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(p_.begin(), p_.end(), engine);
        p_.insert(p_.end(), p_.begin(), p_.end());
    }

    double noise(double x, double y) const 
    {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        double xf = x - std::floor(x);
        double yf = y - std::floor(y);

        double u = fade(xf);
        double v = fade(yf);

        int aa = p_[p_[X] + Y];
        int ab = p_[p_[X] + Y + 1];
        int ba = p_[p_[X + 1] + Y];
        int bb = p_[p_[X + 1] + Y + 1];

        return lerp(v, lerp(u, grad(aa, xf, yf), grad(ba, xf - 1, yf)), lerp(u, grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1)));
    }

    double octave(double x, double y, int octaves = 6, double persistence = 0.5, double lacunarity = 2.0) const 
    {
        double value = 0.0;
        double amplitude = 1.0;
        double frequency = 1.0;
        double maxValue = 0.0;

        for (int i = 0; i < octaves; ++i) 
        {
            value += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        return value / maxValue;
    }

private:
    std::vector<int> p_;

    static double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static double lerp(double t, double a, double b) { return a + t * (b - a); }
    static double grad(int hash, double x, double y) 
    {
        switch (hash & 3) 
        {
            case 0: return  x + y;
            case 1: return -x + y;
            case 2: return  x - y;
            case 3: return -x - y;
            default: return 0;
        }
    }
};

// ─────────────────────────────────────────────
//  Color structure and mapping functions
// ─────────────────────────────────────────────
struct Color { uint8_t r, g, b; };

Color toGreyscale(double v) {
    auto c = static_cast<uint8_t>((v + 1.0) * 0.5 * 255.0);
    return { c, c, c };
}

Color toTerrain(double v) {
    double t = (v + 1.0) * 0.5;
    if (t < 0.15) return { 10,  50, 120 }; // deeper water
    if (t < 0.30) return { 10,  50, 100 }; // deep water
    if (t < 0.40) return { 40,  90, 180 }; // shallow water
    if (t < 0.45) return { 220, 200, 130 }; // sand
    if (t < 0.60) return { 60, 140,  50 }; // grass
    if (t < 0.75) return { 40, 100,  30 }; // forest
    if (t < 0.88) return { 100,  80,  60 }; // rock
    return                { 240, 240, 250 }; // snow
}

// ─────────────────────────────────────────────
//  option parsing configuration structure
// ─────────────────────────────────────────────
struct Config {
    int      width = 512;
    int      height = 512;
    uint32_t seed = 666;
    int      octaves = 6;
    double   persistence = 0.5;
    double   lacunarity = 2.0;
    double   scale = 4.0;
    fs::path outdir = ".";
};