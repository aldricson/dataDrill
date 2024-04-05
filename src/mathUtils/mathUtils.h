#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <math.h>

// Fonction static inline pour arrondir un double à un nombre spécifié de chiffres significatifs
static inline double roundToNbSignificativDigits(double value, unsigned int nbSignificativDigits) {
    if (value == 0.0) {
        return 0.0;
    }
    else if (nbSignificativDigits==0)
    {
        return value;
    }

    // Calculer le facteur d'échelle pour arrondir le nombre
    double scale = pow(10.0, nbSignificativDigits - 1 - (int)floor(log10(fabs(value))));

    // Arrondir la valeur et la remettre à l'échelle
    return round(value * scale) / scale;
}

static inline  std::pair<double, double> calculateLeastSquares(const std::vector<double>& x, const std::vector<double>& y) {
    const auto n = x.size();
    const double sum_x = std::accumulate(x.begin(), x.end(), 0.0);
    const double sum_y = std::accumulate(y.begin(), y.end(), 0.0);
    const double sum_xx = std::inner_product(x.begin(), x.end(), x.begin(), 0.0);
    const double sum_xy = std::inner_product(x.begin(), x.end(), y.begin(), 0.0);
    const double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    const double intercept = (sum_y - slope * sum_x) / n;
    return {slope, intercept};
}


#endif // MATHUTILS_H
