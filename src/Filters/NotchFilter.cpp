#include "NotchFilter.h"


/**
 * @brief Constructor for the NotchFilter class that initializes filter coefficients.
 * 
 * This constructor calculates the coefficients for a second-order notch filter based on
 * the specified sampling frequency, gain at the notch frequency, quality factor (Q),
 * frequency step, and minimum frequency. The coefficients are calculated for a series of
 * center frequencies, starting from 'fmin' and increasing by 'fstep' for a total of
 * 'MAX_COEFS' frequencies. The filter attenuates frequencies around each center frequency
 * based on the specified 'gainAtNotch' (gain at notch) and 'Q' (quality factor).
 * 
 * @param samplingRate Sampling frequency in Hz. It's the rate at which the input signal is sampled.
 * @param gainAtNotch Gain at the notch frequencies. A value closer to 0 results in deeper notches
 *           (more attenuation), whereas values closer to 1 result in shallower notches.
 * @param Q Quality factor of the notch filter. Higher Q values result in narrower notches,
 *          providing sharper attenuation around the center frequency.
 * @param fstep Frequency step in Hz. This determines the increment between consecutive
 *              center frequencies for which the filter coefficients are calculated.
 * @param fmin Minimum frequency in Hz. This is the starting point for the range of center
 *             frequencies over which the filter coefficients are calculated.
 * 
 * Example usage:
 * @code
 * // Create a notch filter with a sampling frequency of 48000 Hz, a gain of 0.707 at the
 * // notch frequencies, a quality factor of 2, a frequency step of 50 Hz, starting at 60 Hz.
 * NotchFilter nf(48000, 0.707, 2, 50, 60);
 * @endcode
 * 
 * @code
 * // Create a notch filter designed to attenuate 60 Hz and its harmonics in a 48000 Hz sampled signal.
 * // The filter has a higher Q factor for sharper notches, starting at 60 Hz and increasing in 60 Hz steps.
 * NotchFilter powerLineFilter(48000, 0.5, 10, 60, 60);
 * @endcode
 */
NotchFilter::NotchFilter(double samplingRate, double gainAtNotch, double Q, double fstep, short fmin)
{
    // Calculate the damping factor based on the bandwidth gain (gb).
    // The damping factor (damp) determines the width of the notch.
    // A higher 'gb' value results in a narrower notch, as it implies a smaller bandwidth for attenuation.
    double damp = std::sqrt(1 - std::pow(gainAtNotch, 2)) / gainAtNotch;
    double wo; // Angular frequency in radians per second.

    coeffArr.resize(MAX_COEFS);// Prepare the coefficient array to hold the filter coefficients.

    for (int i = 0; i < MAX_COEFS; ++i) 
    {
        // Calculate the angular frequency 'wo' for the current step.
        // This involves converting the linear frequency (fstep*i + fmin) into angular frequency
        // using 'wo = 2 * PI * frequency', normalized by the sampling frequency 'fsfilt'.
        // The resulting 'wo' is the angular frequency at which the notch filter is centered.
        wo = 2 * PI * (fstep * i + fmin) / samplingRate;
        // Calculate the first coefficient 'e', which is influenced by the damping factor
        // and the quality factor 'Q'. The 'Q' factor affects the sharpness of the notch,
        // with a higher 'Q' leading to a sharper notch. The term 'tan(wo/(Q*2))' adjusts
        // the width of the notch based on 'Q' and the center frequency 'wo'.
        coeffArr[i].e = 1 / (1 + damp * std::tan(wo / (Q * 2)));
        // Calculate the pole position 'p' using the cosine of 'wo'.
        // This determines the filter's response characteristics around the notch frequency.
        coeffArr[i].p = std::cos(wo);
        // The remaining coefficients 'd[0]', 'd[1]', and 'd[2]' are derived from 'e' and 'p'
        // to complete the filter's difference equation. These coefficients dictate how the input
        // signal is transformed to attenuate the specified frequency band.
        coeffArr[i].d[0] = coeffArr[i].e;
        coeffArr[i].d[1] = 2 * coeffArr[i].e * coeffArr[i].p;
        coeffArr[i].d[2] = (2 * coeffArr[i].e - 1);
    }
}

void NotchFilter::setup(int index) 
{
    //The setup method allows for dynamic selection of filter coefficients based on an index.
    //The approach is to pre-calculate a range of filter settings for different center frequencies
    //and quickly switch between them at runtime without recalculating the coefficients. 
    //It's a way to adapt the filter's behavior based on the signal's characteristics or external conditions.
    state.e = coeffArr[index].e;
    state.p = coeffArr[index].p;
    state.d[0] = coeffArr[index].d[0];
    state.d[1] = coeffArr[index].d[1];
    state.d[2] = coeffArr[index].d[2];
}

/**
 * @brief Filters an input sample through the notch filter and returns the filtered output.
 *
 * This method applies the notch filter to an input sample ('yin') and updates the filter's
 * internal state. The filtering process uses a digital second-order notch filter formula,
 * which is designed to attenuate specific frequencies while leaving others unaffected.
 *
 * @param yin The input sample to be filtered.
 * @return The filtered output sample.
 *
 * The mathematical operation performed by this function can be described by the difference equation:
 *   y[n] = d[0]*x[n] - d[1]*x[n-1] + d[0]*x[n-2] + d[1]*y[n-1] - d[2]*y[n-2]
 * where:
 *   - y[n] is the current output,
 *   - x[n], x[n-1], x[n-2] are the current and previous two input samples, respectively,
 *   - y[n-1], y[n-2] are the previous two output samples,
 *   - d[0], d[1], d[2] are the filter coefficients calculated during initialization.
 *
 * The equation is derived from the z-transform of the filter's transfer function, representing
 * the filter's behavior in the frequency domain. By applying this equation in the time domain,
 * we effectively apply the notch filter's frequency-selective attenuation to the input signal.
 *
 * Physics behind the operation:
 * - The filter's operation leverages the principle of destructive interference to attenuate
 *   specific frequency components of the input signal. By carefully choosing the filter coefficients
 *   (d[0], d[1], d[2]), certain frequencies are cancelled out in the output due to phase differences
 *   introduced between the input signal and the filter's response.
 * - The specific frequencies to be attenuated (notch frequencies) are determined by the filter's
 *   design parameters (samplingRate, gainAtNotch, Q, fstep, fmin) set during initialization. These parameters
 *   influence the coefficients and, therefore, the filter's frequency response.
 *
 * The function updates the filter's internal state (x and y arrays) to maintain a history of the
 * last two samples for both the input and output. This state is crucial for the recursive nature
 * of the filter's operation, allowing it to apply the current input sample's effect while considering
 * the immediate past, thus producing a smooth and continuous output over time.
 */
double NotchFilter::filter(double yin) 
{
    // Shift the previous input samples backward in the array to make room for the new input.
    state.x[0] = state.x[1]; 
    state.x[1] = state.x[2]; 
    state.x[2] = yin; // Update the current input sample.
    
    // Similarly, shift the previous output samples backward in the array.
    state.y[0] = state.y[1]; 
    state.y[1] = state.y[2]; 

    // Apply the filter equation using the current and previous input and output samples,
    // along with the filter's coefficients, to calculate the new output.
    state.y[2] = state.d[0] * state.x[2] - state.d[1] * state.x[1] + state.d[0] * state.x[0] + state.d[1] * state.y[1] - state.d[2] * state.y[0];
    
    return state.y[2]; // Return the newly calculated output sample.
}

