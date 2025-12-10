#include "wavefunctioncollapse.h"
#include <unordered_map>
#include <queue>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <limits>
#include <cmath>

// dir offsets for pattern-cell grid propagation (UP, RIGHT, DOWN, LEFT)
static const int dxDir[4] = {0, 1, 0, -1};
static const int dyDir[4] = {-1, 0, 1, 0};

OverlappingWFC::OverlappingWFC(const ImageGrid& src_,
                               int patternSize,
                               int outWidth,
                               int outHeight,
                               bool periodicInput_,
                               bool periodicOutput_)
    : src(src_),
    srcW((int)(src_.empty() ? 0 : src_[0].size())),
    srcH((int)src_.size()),
    N(std::max(1, patternSize)),
    outW(outWidth),
    outH(outHeight),
    periodicInput(periodicInput_),
    periodicOutput(periodicOutput_),
    Gx(0), Gy(0),
    rng(std::random_device{}())
{
    // extract patterns (periodic sampling so border patterns wrap)
    extractPatternsPeriodic();
    buildAdjacencyTables();
    // output image placeholder
    output.assign(outH, std::vector<Pixel>(outW, 0));
}

Pixel OverlappingWFC::safeGet(const ImageGrid& img, int w, int h, int x, int y, bool periodic) {
    if (periodic) {
        x = ((x % w) + w) % w;
        y = ((y % h) + h) % h;
        return img[y][x];
    } else {
        if (x < 0 || x >= w || y < 0 || y >= h) return 0;
        return img[y][x];
    }
}

void OverlappingWFC::extractPatternsPeriodic() {
    patterns.clear();
    weights.clear();

    if (srcW == 0 || srcH == 0) return;

    // number of pattern positions in source:
    // if periodicInput, we consider every (x,y) as a start and wrap pixels; otherwise valid starts are 0..W-N
    int maxX = periodicInput ? srcW : (srcW - N + 1);
    int maxY = periodicInput ? srcH : (srcH - N + 1);
    if (maxX <= 0) maxX = 1;
    if (maxY <= 0) maxY = 1;

    // map from pattern-data string to id
    struct VecHash {
        size_t operator()(const std::vector<Pixel>& v) const noexcept {
            // simple xor/FNV-ish
            uint64_t h = 1469598103934665603ULL;
            for (Pixel p : v) {
                uint64_t x = p;
                h ^= (x & 0xFF); h *= 1099511628211ULL;
                h ^= ((x>>8) & 0xFF); h *= 1099511628211ULL;
                h ^= ((x>>16) & 0xFF); h *= 1099511628211ULL;
                h ^= ((x>>24) & 0xFF); h *= 1099511628211ULL;
            }
            return (size_t)h;
        }
    };
    std::unordered_map<std::vector<Pixel>, int, VecHash> mapPat;

    for (int sy = 0; sy < maxY; ++sy) {
        for (int sx = 0; sx < maxX; ++sx) {
            std::vector<Pixel> pat;
            pat.reserve(N*N);
            for (int dy = 0; dy < N; ++dy) {
                for (int dx = 0; dx < N; ++dx) {
                    int px = sx + dx;
                    int py = sy + dy;
                    Pixel pix = safeGet(src, srcW, srcH, px, py, periodicInput);
                    pat.push_back(pix);
                }
            }
            auto it = mapPat.find(pat);
            if (it == mapPat.end()) {
                int id = (int)patterns.size();
                patterns.push_back(Pattern{pat});
                weights.push_back(1);
                mapPat.emplace(pat, id);
            } else {
                weights[it->second] += 1;
            }
        }
    }

    // If there are zero patterns (very small image), create one from top-left
    if (patterns.empty()) {
        std::vector<Pixel> pat(N*N, safeGet(src, srcW, srcH, 0, 0, true));
        patterns.push_back(Pattern{pat});
        weights.push_back(1);
    }
}

bool OverlappingWFC::overlapCompatible(const std::vector<Pixel>& A, const std::vector<Pixel>& B, int N, int dir) {
    if (dir == 1) { // RIGHT: A's right col == B's left col
        for (int y = 0; y < N; ++y) if (A[y*N + (N-1)] != B[y*N + 0]) return false;
        return true;
    } else if (dir == 3) { // LEFT: A's left col == B's right col
        for (int y = 0; y < N; ++y) if (A[y*N + 0] != B[y*N + (N-1)]) return false;
        return true;
    } else if (dir == 0) { // UP: A's top row == B's bottom row
        for (int x = 0; x < N; ++x) if (A[0 * N + x] != B[(N-1)*N + x]) return false;
        return true;
    } else { // DOWN: A's bottom row == B's top row
        for (int x = 0; x < N; ++x) if (A[(N-1)*N + x] != B[0 * N + x]) return false;
        return true;
    }
}

void OverlappingWFC::buildAdjacencyTables() {
    int P = (int)patterns.size();
    adjacency.clear();
    adjacency.resize(P);

    for (int a = 0; a < P; ++a) {
        for (int dir = 0; dir < 4; ++dir) {
            adjacency[a][dir].clear();
        }
        for (int b = 0; b < P; ++b) {
            for (int dir = 0; dir < 4; ++dir) {
                if (overlapCompatible(patterns[a].data, patterns[b].data, N, dir)) {
                    adjacency[a][dir].push_back(b);
                }
            }
        }
        // safety: if any adjacency list is empty, allow self as fallback (avoids immediate contradiction)
        for (int dir = 0; dir < 4; ++dir) {
            if (adjacency[a][dir].empty()) adjacency[a][dir].push_back(a);
        }
    }
}

void OverlappingWFC::initializeWave() {
    // Gx/Gy are number of pattern placements in output
    Gx = outW - N + 1;
    Gy = outH - N + 1;
    if (Gx < 1) Gx = 1;
    if (Gy < 1) Gy = 1;

    int P = (int)patterns.size();
    wave.clear();
    wave.resize(Gx * Gy);
    for (auto &c : wave) {
        c.possible.assign(P, 1);
    }
}

int OverlappingWFC::choosePatternWeighted(const Cell& c) {
    std::vector<int> ids;
    std::vector<double> ws;
    for (int i = 0; i < (int)patterns.size(); ++i) {
        if (c.possible[i]) { ids.push_back(i); ws.push_back(double(weights[i])); }
    }
    if (ids.empty()) return -1;
    std::discrete_distribution<int> dist(ws.begin(), ws.end());
    int sel = dist(rng);
    return ids[sel];
}

bool OverlappingWFC::propagate() {
    int P = (int)patterns.size();
    std::queue<std::pair<int,int>> q;
    // push all cells to start (safe)
    for (int y = 0; y < Gy; ++y) for (int x = 0; x < Gx; ++x) q.emplace(x,y);

    while (!q.empty()) {
        auto [x,y] = q.front(); q.pop();
        int idx = y * Gx + x;
        Cell &c = wave[idx];

        for (int dir = 0; dir < 4; ++dir) {
            int nx = x + dxDir[dir];
            int ny = y + dyDir[dir];

            bool outside = false;
            if (periodicOutput) {
                nx = (nx % Gx + Gx) % Gx;
                ny = (ny % Gy + Gy) % Gy;
            } else {
                if (nx < 0 || nx >= Gx || ny < 0 || ny >= Gy) outside = true;
            }
            if (outside) continue;

            int nidx = ny * Gx + nx;
            Cell &nc = wave[nidx];
            bool changed = false;

            for (int pid = 0; pid < P; ++pid) {
                if (!nc.possible[pid]) continue;
                bool hasSupport = false;

                // need some pattern p0 in c such that adjacency[p0][dir] contains pid
                for (int p0 = 0; p0 < P; ++p0) {
                    if (!c.possible[p0]) continue;
                    const auto &alist = adjacency[p0][dir];
                    // linear search ok for moderate P
                    if (std::find(alist.begin(), alist.end(), pid) != alist.end()) { hasSupport = true; break; }
                }

                if (!hasSupport) {
                    nc.possible[pid] = 0;
                    changed = true;
                }
            }

            // contradiction if no possibilities
            bool any = false;
            for (char b : nc.possible) if (b) { any = true; break; }
            if (!any) return false;

            if (changed) q.emplace(nx, ny);
        }
    }
    return true;
}

bool OverlappingWFC::attemptSolveOnce() {
    initializeWave();
    int P = (int)patterns.size();
    if (P == 0) return false;

    while (true) {
        // find lowest entropy (Shannon-like) cell that is not determined
        double bestEntropy = std::numeric_limits<double>::infinity();
        std::optional<std::pair<int,int>> bestCell;
        for (int y = 0; y < Gy; ++y) for (int x = 0; x < Gx; ++x) {
                Cell &c = wave[y*Gx + x];
                int count = 0;
                for (char b : c.possible) if (b) ++count;
                if (count == 0) return false; // contradiction
                if (count == 1) continue;
                double sumW = 0, sumWlogW = 0;
                for (int i = 0; i < P; ++i) if (c.possible[i]) {
                        double w = double(weights[i]);
                        sumW += w;
                        sumWlogW += w * std::log(w);
                    }
                double H = std::log(sumW) - (sumWlogW / sumW);
                // small tie-break
                H -= (std::uniform_real_distribution<double>(0.0, 1e-6)(rng));
                if (H < bestEntropy) { bestEntropy = H; bestCell = std::make_pair(x,y); }
            }

        if (!bestCell.has_value()) break; // all determined

        int bx = bestCell->first;
        int by = bestCell->second;
        Cell &cell = wave[by*Gx + bx];

        int chosen = choosePatternWeighted(cell);
        if (chosen < 0) return false;
        for (int i = 0; i < P; ++i) cell.possible[i] = (i==chosen) ? 1 : 0;

        // propagate constraints
        if (!propagate()) return false;
    }

    // final check: every cell has >=1 possibility
    for (const auto &c : wave) {
        bool any = false;
        for (char b : c.possible) if (b) { any = true; break; }
        if (!any) return false;
    }

    // build output image by stamping patterns (later stamps overwrite earlier)
    buildOutputFromWave();
    return true;
}

void OverlappingWFC::buildOutputFromWave() {
    // fill with black default
    output.assign(outH, std::vector<Pixel>(outW, 0));

    // For each pattern-cell (gx,gy) place its chosen pattern into pixels (gx..gx+N-1, gy..gy+N-1)
    for (int gy = 0; gy < Gy; ++gy) {
        for (int gx = 0; gx < Gx; ++gx) {
            const Cell &c = wave[gy*Gx + gx];
            int chosen = -1;
            for (int i = 0; i < (int)patterns.size(); ++i) if (c.possible[i]) { chosen = i; break; }
            if (chosen < 0) chosen = 0;
            const auto &pat = patterns[chosen].data;
            for (int dy = 0; dy < N; ++dy) {
                for (int dx = 0; dx < N; ++dx) {
                    int sx = gx + dx;
                    int sy = gy + dy;
                    if (sx < 0 || sx >= outW || sy < 0 || sy >= outH) continue;
                    output[sy][sx] = pat[dy*N + dx];
                }
            }
        }
    }
}

bool OverlappingWFC::run(int maxAttempts) {
    if (maxAttempts < 1) maxAttempts = 1;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        // reseed rng slightly for different stochastic choices
        rng.seed(std::random_device{}() + attempt);
        bool ok = attemptSolveOnce();
        if (ok) return true;
        // otherwise continue and retry
    }
    return false;
}

ImageGrid OverlappingWFC::getOutput() const {
    return output;
}

int OverlappingWFC::getOutW() const { return outW; }
int OverlappingWFC::getOutH() const { return outH; }
