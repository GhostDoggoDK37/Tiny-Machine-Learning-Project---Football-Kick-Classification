#include "Particle.h"
#include "adxl343.h"

#define SAMPLE_COUNT 500
#define FEATURE_COUNT 26

struct Sample {
    float x, y, z;
};

void extract_features(const Sample* buf, float* out);
