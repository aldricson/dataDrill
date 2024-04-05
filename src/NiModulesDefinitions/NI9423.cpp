#include "NI9423.h"


NI9423::NI9423() 
{
    initModule();  // Initialize the module with default values
}

void NI9423::initModule() 
{
    try {
        // Set default values
        m_moduleName = "NI9423";
        m_nbChannel = 0; // Assumption: NI9423 has 4 channels, adjust if necessary
        m_nbCounters = 4;

        // Setting counter modes
        m_counterCountingEdgeMode = Val_Rising;
        m_counterCountDirectionMode = Val_CountUp;

        // Initialize digital IO port and module type
        m_moduleType = isCounter; // Setting module type

        // Clearing and initializing counter names
        m_counterNames.clear(); 
        m_counterNames.reserve(m_nbCounters); // Reserve to avoid reallocation

        for (unsigned int i = 0; i < m_nbCounters; ++i) {
            m_counterNames.push_back("PFI" + std::to_string(i));
        }

        // Setting counter limits
        m_counterMin = 0;
        m_counterMax = 4294967295; // Maximum value for 32-bit unsigned integer

        // Setting shunt location, value and terminal configuration
        m_shuntLocation = noShunt;
        m_shuntValue = -999999.999; // Placeholder value, adjust if necessary
        m_moduleTerminalConfig = noTerminalConfig;   
    }
    catch (const std::exception& e) {
        // Handle standard exceptions
        std::cerr << "Exception in NI9423::initModule: " << e.what() << std::endl;
        // Additional error handling logic here
    }
    catch (...) {
        // Handle non-standard exceptions
        std::cerr << "Unknown exception in NI9423::initModule" << std::endl;
        // Additional error handling logic here
    }
}


void NI9423::loadConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9423_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

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

void NI9423::saveConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9423_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

        // Open the file for writing
        FILE* file = fopen(fileName.c_str(), "w");
        if (file == nullptr) {
            throw std::runtime_error("Error opening file for writing");
        }

        // Save configuration to file using NIDeviceModule::saveToFile function
        NIDeviceModule::saveToFile(fileName); //  function for saving to a file

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


