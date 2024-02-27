#include "NI9208.h"


NI9208::NI9208() 
{
    
   initModule();
}


void NI9208::initModule() 
{
    try {
        // Set default values
        m_moduleName = "NI9208";
        m_nbChannel = 16; // Consider external configuration if flexibility is needed
        // Resetting counters and digital IO ports to safe defaults
        m_nbCounters = 0;
        m_counterCountingEdgeMode = NoEdge;
        m_counterCountDirectionMode = NoCountMode;
        m_nbDigitalIoPort = 0;
        // Setting module type
        m_moduleType = isAnalogicInputCurrent;
        // Safely initializing channel names with appropriate capacity
        m_chanNames.clear(); // Clear existing names to avoid appending to existing list
        m_chanNames.reserve(m_nbChannel); // Reserve space to avoid reallocations
        for (unsigned int i = 0; i < m_nbChannel; ++i) {
            m_chanNames.push_back("/ai" + std::to_string(i));
        }

        // Setting analog channel limits using a utility function 
        m_analogChanMax = mAmpsToAmps(20.0);  //Convert milliamps to amps
        m_analogChanMin = mAmpsToAmps(-20.0); //Convert milliamps to amps

        // Assigning module unit, shunt location, value, and terminal config
        m_moduleUnit = Val_Amps;
        m_shuntLocation = defaultLocation; // Ensure defaultLocation is a valid, safe default
        m_shuntValue = 34.01; // Consider external configuration for flexibility
        m_moduleTerminalConfig = referencedSingleEnded; // Validate if this config is always safe
    }
    catch (const std::exception& e) {
       // Handle standard exceptions
        std::cerr << "Standard exception: " << e.what() << std::endl;
        // Additional error handling logic here
    }
    catch (...) {
         // Handle non-standard exceptions
        std::cerr << "Unknown exception caught" << std::endl;
        // Additional error handling logic here
    }
}

void NI9208::saveConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9208_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

        // Open the file for writing
        FILE* file = fopen(fileName.c_str(), "w");
        if (file == nullptr) {
            throw std::runtime_error("Error opening file for writing");
        }

        // Save configuration to file
        NIDeviceModule::saveToFile(fileName); // function for saving to file

        // Close the file
        fclose(file);
    }
    catch (const std::exception& e) {
        // Handle exceptions
        std::cerr << "Error saving configuration: " << e.what() << std::endl;
        // Additional error handling logic here
    }
    catch (...) {
        // Handle non-standard exceptions
        std::cerr << "Unknown error occurred while saving configuration." << std::endl;
        // Additional error handling logic here
    }
}

void NI9208::loadConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9208_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

        // Open the file for reading
        FILE* file = fopen(fileName.c_str(), "r");
        if (file == nullptr) {
            throw std::runtime_error("Error opening file for reading");
        }

        // Load configuration from file using  NIDeviceModule::loadFromFile function
        NIDeviceModule::loadFromFile(fileName); // function for loading from a file

        // Close the file
        fclose(file);
    }
    catch (const std::exception& e) {
        // Handle exceptions
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        // Additional error handling logic here
    }
    catch (...) {
        // Handle non-standard exceptions
        std::cerr << "Unknown error occurred while loading configuration." << std::endl;
        // Additional error handling logic here
    }
}



//************* getters ************
std::vector<std::string> NI9208::getChanNames() const 
{
    return m_chanNames;
}

