
#include "LowPassFilter.h"

#define ERROR_CHECK (true)

#if ERROR_CHECK
#include <iostream>
#endif

LowPassFilter::LowPassFilter():
	output(0),
	ePow(0){}

// Constructor with parameters for cutoff frequency and delta time
// Initializes the Low Pass Filter (LPF) with a specific cutoff frequency and sampling interval.
// This setup configures how the filter smoothens the input signal, allowing lower frequencies to pass while attenuating higher frequencies.
LowPassFilter::LowPassFilter(float iCutOffFrequency, float iDeltaTime)
: output(0), // Initialize the output of the filter to 0. This is the starting condition, assuming no input has been processed yet.
  // ePow calculation incorporates the exponential decay formula, which is central to the LPF's operation.
  // The formula 1 - exp(-iDeltaTime * 2 * M_PI * iCutOffFrequency) calculates the filter's response factor,
  // based on the analog RC Low Pass Filter's time constant , where R is resistance, and C is capacitance.

  //The exponential decay formula used in the ePow calculation reflects how the LPF's output reacts to changes in its input over time, akin to how an electrical 
  //charge builds up or decays across the components of an RC (resistor-capacitor) circuit. The time constant (τ) of an RC circuit, which determines how quickly 
  //the circuit responds to voltage changes, is given by: τ=RC


  // In the digital domain, the time constant is adapted using the sampling interval (iDeltaTime) and the desired cutoff frequency,
  // affecting how fast the filter responds to input changes. A smaller time constant results in faster response but less smoothing,
  // while a larger time constant results in slower response with more smoothing.



  //In physics and engineering, frequency is expressed in Hertz Hz, indicating cycles per second.
  //However, while we are dealing with oscillatory systems, we will use use angular frequency  which is measured in radians per second.
  //The relationship between angular frequency and regular frequency is given by: ω=2πf
  //π as always is the mathematical constant representing the ratio of a circle's circumference to its diameter, and the factor of 
  //2 convert a cycle into radians, since a complete cycle around a circle corresponds to 2π radians.


  // The `ePow` factor is pivotal in determining how the LPF responds to input signal changes over time. It's calculated as:
  // ePow = 1 - exp(-iDeltaTime * 2 * M_PI * iCutOffFrequency)
  // This formula encapsulates two fundamental concepts in signal processing: angular frequency (ω) and the exponential decay characteristic of the filter.

  // Angular Frequency (ω = 2πf):
  // - Angular frequency (ω), measured in radians per second, provides a way to express frequencies in terms of their angular displacement 
  //   per unit time. The term `2π` converts the cut-off frequency (f), typically given in Hertz (cycles per second), into radians per second. 
  //   This conversion is essential because the LPF's mathematical model is based on continuous-time signal processing principles, 
  //   where angular frequency plays a key role.

  // Exponential Decay and Filter's Responsiveness:
  // - The `exp(-iDeltaTime * 2 * M_PI * iCutOffFrequency)` part of the `ePow` calculation models the exponential decay behavior of the filter. 
  //   This decay directly correlates with the LPF's time constant (τ), signifying how quickly the filter reacts to changes in the input signal. 
  //   The time constant is a measure of the time it takes for the filter's output to adjust significantly in response to a step change in its input.

  // In essence, `ePow` serves as a dynamic coefficient that blends the new input with the previous output, thereby governing the filter's smoothing effect. 
  // A higher value of `ePow` (closer to 1) means that the filter's output will adjust more rapidly to changes in the input, offering less smoothing, 
  // whereas a lower value of `ePow` leads to a slower response, resulting in greater smoothing of the input signal. 
  // This behavior is crucial for effectively attenuating high-frequency noise while preserving the desired low-frequency components of the input signal.
  ePow(1 - exp(-iDeltaTime * 2 * M_PI * iCutOffFrequency))
{
    #if ERROR_CHECK
    // Error checking to ensure valid configuration parameters
    if (iDeltaTime <= 0) {
        // A delta time of 0 or negative is invalid because it implies a non-positive sampling interval.
        // The sampling interval (iDeltaTime) is crucial for determining the rate at which samples are processed by the filter,
        // and it directly influences the filter's temporal resolution and responsiveness.
        std::cout << "Warning: A LowPassFilter instance has been configured with 0 s as delta time.";
        ePow = 0; // Set ePow to 0 to neutralize the filter effect, preventing undefined behavior.
    }
    if (iCutOffFrequency <= 0) {
        // A cutoff frequency of 0 or negative is invalid as it doesn't make sense physically.
        // The cutoff frequency defines the threshold at which frequencies are attenuated by the filter,
        // with frequencies below the cutoff passing through more freely, and those above being reduced.
        // Setting a cutoff frequency of 0 Hz would imply blocking all frequencies, which contradicts the filter's purpose.
        std::cout << "Warning: A LowPassFilter instance has been configured with 0 Hz as cut-off frequency.";
        ePow = 0; // Setting ePow to 0 as a safety measure to disable the filter action.
    }
    #endif
}


float LowPassFilter::update(float input){
	return output += (input - output) * ePow;
}

float LowPassFilter::update(float input, float deltaTime, float cutoffFrequency){
	reconfigureFilter(deltaTime, cutoffFrequency); //Changes ePow accordingly.
	return output += (input - output) * ePow;
}

void LowPassFilter::reconfigureFilter(float deltaTime, float cutoffFrequency){
	#if ERROR_CHECK
	if (deltaTime <= 0){
		std::cout << "Warning: A LowPassFilter instance has been configured with 0 s as delta time.";
		ePow = 0;
	}
	if(cutoffFrequency <= 0){
		std::cout << "Warning: A LowPassFilter instance has been configured with 0 Hz as cut-off frequency.";
		ePow = 0;
	}
	#endif
	ePow = 1-exp(-deltaTime * 2 * M_PI * cutoffFrequency);
}