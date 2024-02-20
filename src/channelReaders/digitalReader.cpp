#include "digitalReader.h"

// Constructor
DigitalReader::DigitalReader(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
                             std::shared_ptr<QNiDaqWrapper> aDaqMxInstance)
    : BaseReader(aSysConfigInstance, aDaqMxInstance)
{
    // Add any initialization specific to DigitalReader here
}


void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue)
{
    // Validate moduleAlias
    if (moduleAlias.empty()) {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }

    // Fetch the device module by alias
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    if (!deviceModule) {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }

    // Ensure the module is the correct type (digital input/counter)
    if (deviceModule->getModuleType() == ModuleType::isCounter) 
    {
        
        try 
        {
            // Read the counter value
            uint32_t countValue = 0;
            m_daqMx->readCounter(deviceModule, index, countValue);
            returnedValue = static_cast<double>(countValue);
            // Reset the counter for next reading
            m_daqMx->resetCounter(deviceModule, index);
        } 
        catch (const std::exception& e)
        {
            // Error handling
            returnedValue = std::numeric_limits<double>::min();
        }
    }
    else
    {        
        std::cout<<"module type not handled yet: "<<moduleAlias.c_str()<<" type:"<<deviceModule->getModuleType();
        returnedValue = std::numeric_limits<double>::min();
        return;
    }
}



// Method to read a single shot from a digital channel specified by its module alias and channel name.
void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue) {
    // Check if module alias or channel name is empty and return an error value if so.
    if (moduleAlias.empty() || chanName.empty()) {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }
    // Attempt to fetch the device module using the provided alias.
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    // If no module is found, return an error value.
    if (!deviceModule) {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }
    // Check if the module's type matches expected type (counter in this case).
    if (deviceModule->getModuleType() == ModuleType::isCounter) {
        try {
            // Attempt to read the counter value for the specified channel.
            uint32_t countValue = 0;
            m_daqMx->readCounter(deviceModule, chanName, countValue);
            // Convert the read counter value to double and assign it to returnedValue.
            returnedValue = static_cast<double>(countValue);

            // Reset the counter to prepare it for the next reading.
            m_daqMx->resetCounter(deviceModule, chanName);
        } catch (const std::exception& e) {
            // In case of any exception, log it and return an error value.
            std::cerr << "Error reading counter: " << e.what() << std::endl;
            returnedValue = std::numeric_limits<double>::min();
        }
    } else {
        // If the module is not a counter, log this information and return an error value.
        std::cerr << "Module type not handled for reading: " << moduleAlias << ", type: " << deviceModule->getModuleType() << std::endl;
        returnedValue = std::numeric_limits<double>::min();
    }
}

