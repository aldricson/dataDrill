#include "NI9239.h"


NI9239::NI9239() 
{
    initModule();  // Initialize the module with default values
}

void NI9239::initModule() 
{
    try {
        // Set default values
        m_moduleName = "NI9239";
        m_nbChannel = 4; // Assumption: NI9239 has 4 channels
        m_nbCounters = 0;
        m_counterCountingEdgeMode = NoEdge;
        m_counterCountDirectionMode = NoCountMode;


        m_moduleType = isAnalogicInputVoltage; // Setting module type to analog input voltage

        // Clearing any existing channel names and reserving space
        m_chanNames.clear(); 
        m_chanNames.reserve(m_nbChannel); // Reserve to avoid reallocation

        // Initialize channel names
        for (int i = 0; i < 4; ++i) {
            m_chanNames.push_back("/ai" + std::to_string(i));
        }

        // Setting channel parameters
        m_analogChanMax = 10.0;
        m_analogChanMin = -10.0;
        m_moduleUnit = Val_Volts;
        m_shuntLocation = noShunt;
        m_shuntValue = -999999.999; // Placeholder value, adjust if necessary
        m_moduleTerminalConfig = differencial; // Setting terminal configuration
    }
    catch (const std::exception& e) {
        // Handle standard exceptions
        std::cerr << "Exception in NI9239::initModule: " << e.what() << std::endl;
        // Additional error handling logic here
    }
    catch (...) {
        // Handle non-standard exceptions
        std::cerr << "Unknown exception in NI9239::initModule" << std::endl;
        // Additional error handling logic here
    }
}


void NI9239::saveConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9239_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

        // Open the file for writing
        FILE* file = fopen(fileName.c_str(), "w");
        if (file == nullptr) {
            throw std::runtime_error("Error opening file for writing");
        }

        // Save configuration to file using  NIDeviceModule::saveToFile function
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


void NI9239::loadConfig() {
    try {
        // Construct file name
        std::string fileName = "NI9239_" + std::to_string(NIDeviceModule::getSlotNb()) + ".ini";

        // Open the file for reading
        FILE* file = fopen(fileName.c_str(), "r");
        if (file == nullptr) {
            throw std::runtime_error("Error opening file for reading");
        }

        // Load configuration from file using your NIDeviceModule::loadFromFile function
        NIDeviceModule::loadFromFile(fileName); // Assuming this is your function for loading from a file

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
std::vector<std::string> NI9239::getChanNames() const 
{
    return m_chanNames;
}


