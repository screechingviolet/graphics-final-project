#include "pngio.h"
#include <QImage>
#include <QColor>

ImageGrid PNGIO::loadPNG(const QString& path) {
    QImage img;
    if (!img.load(path)) return {};

    img = img.convertToFormat(QImage::Format_RGB32);
    int w = img.width();
    int h = img.height();
    ImageGrid g(h, std::vector<Pixel>(w));

    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb q = line[x];
            int r = qRed(q), gr = qGreen(q), b = qBlue(q);
            Pixel p = (uint32_t(r) << 16) | (uint32_t(gr)<<8) | uint32_t(b);
            g[y][x] = p;
        }
    }
    return g;
}

bool PNGIO::savePNG(const QString& path, const ImageGrid& grid) {
    if (grid.empty()) return false;
    int h = (int)grid.size();
    int w = (int)grid[0].size();
    QImage img(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            Pixel p = grid[y][x];
            int r = (p >> 16) & 0xFF;
            int g = (p >> 8) & 0xFF;
            int b = p & 0xFF;
            line[x] = qRgb(r, g, b);
        }
    }
    return img.save(path);
}
