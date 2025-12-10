#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <QFile>
#include <QTextStream>
#include <iostream>
#include <GL/glew.h>

class MiscUtilities {
    int m_numRandomCalls = 0;
public:
    static double randomGen(float low, float high);
};
