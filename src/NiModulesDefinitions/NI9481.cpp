#include "NI9481.h"


NI9481::NI9481() 
{
    initModule();  // Initialize the module with default values
}

void NI9481::initModule() 
{
    try {
        // Set default values
        m_moduleName = "NI9481";
        //NI9481 has no analog channels
        setNbChannel (0);
        //NI9481 has no analog counters
        setNbCounters(0);

        m_nbDigitalOutputs = 4; // Assuming NI9481 has 4 digital outputs, adjust if necessary
        m_moduleType = isDigitalOutput; // Setting module type to digital output

        // Clearing any existing names
        m_counterNames.clear(); 
        m_digitalOutputNames.clear();

        std::string portPrefix = "/port";

        // Initializing digital IO and output names
        for (unsigned int j = 0; j < m_nbDigitalOutputs; ++j) 
        {
            m_digitalOutputNames.push_back(portPrefix + std::to_string(j)+"/");

            for (unsigned int i = 0; i < m_nbDigitalOutputs; ++i) 
            {
                m_digitalOutputNames.push_back(m_digitalOutputNames[j] + "line" + std::to_string(i));
                std::cout<<m_digitalOutputNames[j].c_str()<<std::endl;
            }
        }

        // Setting counter and shunt values
        m_counterMin = 0;
        m_counterMax = 4294967295; // Maximum value for 32-bit unsigned integer
        m_shuntLocation = noShunt;
        m_shuntValue = -999999.999; // Placeholder value, adjust if necessary
        m_moduleTerminalConfig = noTerminalConfig;   
    }
    catch (const std::exception& e) {
        // Handle standard exceptions
        std::cerr << "Exception in NI9481::initModule: " << e.what() << std::endl;
        // Additional error handling logic here
    }
    catch (...) {
        // Handle non-standard exceptions
        std::cerr << "Unknown exception in NI9481::initModule" << std::endl;
        // Additional error handling logic here
    }
}

void NI9481::loadConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9481_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

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

void NI9481::saveConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9481_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

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



