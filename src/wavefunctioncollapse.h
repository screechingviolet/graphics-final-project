#ifndef WAVEFUNCTIONCOLLAPSE_H
#define WAVEFUNCTIONCOLLAPSE_H

#include <vector>
#include <cstdint>
#include <random>
#include <optional>
#include <array>

/// Overlapping Wave Function Collapse (periodic input sampling supported)
/// Pixel stored as 0xRRGGBB (uint32_t)
using Pixel = uint32_t;
using ImageGrid = std::vector<std::vector<Pixel>>;

class OverlappingWFC {
public:
    // Construct with an input image grid (ImageGrid[y][x])
    OverlappingWFC(const ImageGrid& src,
                   int patternSize,
                   int outWidth,
                   int outHeight,
                   bool periodicInput = true,
                   bool periodicOutput = false);

    // Run WFC with up to maxAttempts restarts (returns true on success).
    bool run(int maxAttempts = 8);

    // Get the synthesized output as ImageGrid (pixels packed 0xRRGGBB)
    ImageGrid getOutput() const;

    // Getters
    int getOutW() const;
    int getOutH() const;

private:
    // Internal types
    struct Pattern {
        std::vector<Pixel> data; // length N*N
    };

    const ImageGrid src;
    const int srcW, srcH;
    const int N;            // pattern size
    const int outW, outH;   // output pixel size
    const bool periodicInput;
    const bool periodicOutput;

    // extracted patterns and weights
    std::vector<Pattern> patterns;
    std::vector<int> weights;

    // adjacency: adjacency[p][dir] -> vector of pattern IDs allowed when neighbor is at dir
    // dir: 0=UP,1=RIGHT,2=DOWN,3=LEFT
    std::vector<std::array<std::vector<int>,4>> adjacency;

    // solver wave: for each pattern-cell (Gx x Gy) maintain possible pattern flags
    int Gx, Gy; // number of pattern placements horizontally and vertically: Gx = outW - N + 1 (clamped >=1)
    struct Cell { std::vector<char> possible; };
    std::vector<Cell> wave;

    // final output
    ImageGrid output;

    // RNG
    std::mt19937 rng;

private:
    // steps
    void extractPatternsPeriodic();
    void buildAdjacencyTables();
    void initializeWave();
    bool attemptSolveOnce();
    bool propagate();
    int choosePatternWeighted(const Cell& c);
    void buildOutputFromWave();

    // helpers
    static Pixel safeGet(const ImageGrid& img, int w, int h, int x, int y, bool periodic);
    static bool overlapCompatible(const std::vector<Pixel>& A, const std::vector<Pixel>& B, int N, int dir);
};

#endif // WAVEFUNCTIONCOLLAPSE_H
