#ifndef NOTCHFILTER_H
#define NOTCHFILTER_H

#include <vector>
#include <cmath>

class NotchFilter {
public:
    NotchFilter(double samplingRate, double gainAtNotch, double Q, double fstep, short fmin);
    void setup(int index);
    double filter(double yin);

private:
    struct Coeffs {
        double e;
        double p;
        double d[3];
    };

    struct FilterState {
        double e;
        double p;
        double d[3];
        double x[3] = {0.0, 0.0, 0.0};
        double y[3] = {0.0, 0.0, 0.0};
    };

    static constexpr int MAX_COEFS = 120;
    static constexpr double PI = 3.1415926;
    std::vector<Coeffs> coeffArr;
    FilterState state;
};

#endif // NOTCHFILTER_H
