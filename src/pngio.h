#ifndef PNGIO_H
#define PNGIO_H

#include <vector>
#include <QString>
#include <cstdint>
#include "wavefunctioncollapse.h" // for Pixel / ImageGrid typedefs

// helpers to load/save PNG using QImage (Qt)
namespace PNGIO {
// Load RGBA or RGB PNG into ImageGrid (dropping alpha) with 0xRRGGBB pixels
// Returns empty grid on failure
ImageGrid loadPNG(const QString& path);

// Save ImageGrid to PNG
bool savePNG(const QString& path, const ImageGrid& grid);
}

#endif // PNGIO_H
