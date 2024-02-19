#ifndef CONVERSIONUTILS_H
#define CONVERSIONUTILS_H

static inline double mAmpsToAmps(double mA)
{
    return mA/1000; // Convert milliamps to amps
}

static inline double ampsTomAmps(double A)
{
    return A*1000; // Convert amps to milliamps
}

#endif