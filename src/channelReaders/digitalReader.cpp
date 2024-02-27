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
    if (moduleAlias.empty()) 
    {
        appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                   "in\n"
                                   "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue)\n"
                                   "Error: moduleAlias is empty.\n"
                                   "moduleAlias:\n"+moduleAlias+
                                   "\nindex:\n"+std::to_string(index));
        returnedValue = std::numeric_limits<double>::min();
        return;
    }

    // Fetch the device module by alias
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                   "in\n"
                                   "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue)\n"
                                   "Error: deviceModule is nullptr.");
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
            appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                      "in\n"
                                      "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue)\n"
                                      "Error: reading counter failed\n"
                                      "\nindex:\n"+std::to_string(index)+
                                      "Exception:\n"+std::string(e.what()));

            returnedValue = std::numeric_limits<double>::min();
        }
    }
    else
    {        
        appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                  "in\n"
                                  "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue)\n"
                                  "Error: Module type not handled for reading.\n"
                                  "Alias:\n"+ moduleAlias+
                                  "\nindex:\n"+
                                  std::to_string(index)+
                                  "\ntype:\n" +
                                   std::to_string(static_cast<int>(deviceModule->getModuleType())));
        std::cout<<"module type not handled yet: "<<moduleAlias.c_str()<<" type:"<<deviceModule->getModuleType();
        returnedValue = std::numeric_limits<double>::min();
        return;
    }
}



// Method to read a single shot from a digital channel specified by its module alias and channel name.
void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue) 
{
    // Check if module alias or channel name is empty and return an error value if so.
    if (moduleAlias.empty() || chanName.empty()) 
    {
        appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                   "in\n"
                                   "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue)\n"
                                   "Error: moduleAlias or chanName are empty\n"
                                   "moduleAlias:\n"+moduleAlias+
                                   "\nchanName:\n"+chanName);
        returnedValue = std::numeric_limits<double>::min();
        return;
    }
    // Attempt to fetch the device module using the provided alias.
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    // If no module is found, return an error value.
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                   "in\n"
                                   "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue)\n"
                                   "Error: deviceModule is nullptr");
        returnedValue = std::numeric_limits<double>::min();
        return;
    }
    // Check if the module's type matches expected type (counter in this case).
    if (deviceModule->getModuleType() == ModuleType::isCounter) 
    {
        try {
            // Attempt to read the counter value for the specified channel.
            uint32_t countValue = 0;
            m_daqMx->readCounter(deviceModule, chanName, countValue);
            // Convert the read counter value to double and assign it to returnedValue.
            returnedValue = static_cast<double>(countValue);

            // Reset the counter to prepare it for the next reading.
            m_daqMx->resetCounter(deviceModule, chanName);
        } 
        catch (const std::exception& e) 
        {
            // In case of any exception, log it and return an error value.
            appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                      "in\n"
                                      "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue)\n"
                                      "Error: reading counter failed\n"
                                      "Exception:\n"+std::string(e.what()));

            returnedValue = std::numeric_limits<double>::min();
        }
    } 
    else 
    {
        // If the module is not a counter, log this information and return an error value.
        appendCommentWithTimestamp(fileNamesContainer.digitalReaderLogFile,
                                  "in\n"
                                  "void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue)\n"
                                  "Error: Module type not handled for reading.\n"
                                  "Alias:\n"+ moduleAlias+
                                  "type:\n" +
                                   std::to_string(static_cast<int>(deviceModule->getModuleType())));
        returnedValue = std::numeric_limits<double>::min();
    }
}

