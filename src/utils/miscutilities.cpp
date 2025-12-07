#include <utils/miscutilities.h>
#include <random>

double MiscUtilities::randomGen(float low, float high) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(low, high);

    return dist(gen);
    //srand(time(0));
    /*
    int discretisation = 100000;
    int randomNum = rand() % discretisation;
    float valuesPerUnit = ((float)discretisation / (high - low));
    float retVal = ((float)randomNum / valuesPerUnit) + low;

    return retVal;*/
}
