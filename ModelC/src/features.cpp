#include "features.h"
#include <math.h>
#include <string.h>


static float ax[SAMPLE_COUNT];
static float ay[SAMPLE_COUNT];
static float az[SAMPLE_COUNT];
static float at[SAMPLE_COUNT];
static float tmp[SAMPLE_COUNT];

// Mean calculation
static float mean(const float* x, int N) {
    float s = 0.0f;
    for (int i = 0; i < N; i++) s += x[i];
    return s / N;
}

// Variance calculation
static float variance(const float* x, int N, float m) {
    float s = 0.0f;
    for (int i = 0; i < N; i++) {
        float d = x[i] - m;
        s += d*d;
    }
    return s / N;
}

// Root Mean Square calculation
static float rms(const float* x, int N) {
    float s = 0.0f;
    for (int i = 0; i < N; i++) s += x[i]*x[i];
    return sqrtf(s / N);
}

// Skewness calculation
static float skewness(const float* x, int N, float m, float sd) {
    float s = 0.0f;
    for (int i = 0; i < N; i++) {
        float z = x[i] - m;
        s += z*z*z;
    }
    return (s / N) / (sd*sd*sd + 1e-9f);
}

// Kurtosis calculation 
static float kurtosis_f(const float* x, int N, float m, float sd) {
    float s = 0.0f;
    for (int i = 0; i < N; i++) {
        float z = x[i] - m;
        s += z*z*z*z;
    }
    return (s / N) / (sd*sd*sd*sd + 1e-9f);
}

// Function to extract features from the sample buffer
void extract_features(const Sample* buf, float* out) {

    // Kopier data ind i globale (static) arrays
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        ax[i] = buf[i].x;
        ay[i] = buf[i].y;
        az[i] = buf[i].z;
        at[i] = sqrtf(buf[i].x*buf[i].x + buf[i].y*buf[i].y + buf[i].z*buf[i].z);
    }

    int k = 0;
    float* cols[3] = {ax, ay, az};

    for (int c = 0; c < 3; c++) {
        float* x = cols[c];

        float m = mean(x, SAMPLE_COUNT);
        float v = variance(x, SAMPLE_COUNT, m);
        float sd = sqrtf(v);

        float mn = x[0], mx = x[0];
        for (int i = 1; i < SAMPLE_COUNT; i++) {
            if (x[i] < mn) mn = x[i];
            if (x[i] > mx) mx = x[i];
        }

        out[k++] = m;
        out[k++] = sd;
        out[k++] = v;
        out[k++] = mx - mn;
        out[k++] = rms(x, SAMPLE_COUNT);
        out[k++] = skewness(x, SAMPLE_COUNT, m, sd);
        out[k++] = kurtosis_f(x, SAMPLE_COUNT, m, sd);
    }

    float mn = at[0], mx = at[0];
    for (int i = 1; i < SAMPLE_COUNT; i++) {
        if (at[i] < mn) mn = at[i];
        if (at[i] > mx) mx = at[i];
    }

    float energy = 0.0f;
    for (int i = 0; i < SAMPLE_COUNT; i++)
        energy += at[i]*at[i];
    energy /= SAMPLE_COUNT;

    memcpy(tmp, at, SAMPLE_COUNT * sizeof(float));

    for (int i = 1; i < SAMPLE_COUNT; i++) {
        float t = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > t) {
            tmp[j+1] = tmp[j];
            j--;
        }
        tmp[j+1] = t;
    }

    float median = tmp[SAMPLE_COUNT / 2];

    float absmean = 0.0f;
    for (int i = 0; i < SAMPLE_COUNT; i++)
        absmean += fabsf(at[i]);
    absmean /= SAMPLE_COUNT;

    out[k++] = energy;
    out[k++] = mx;
    out[k++] = mn;
    out[k++] = median;
    out[k++] = absmean;
}
