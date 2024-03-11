#include "analogicReader.h"

AnalogicReader::AnalogicReader(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance, std::shared_ptr<QNiDaqWrapper> aDaqMxInstance)
    : BaseReader(aSysConfigInstance, aDaqMxInstance) // Call the base class constructor
{
    // Your AnalogReader constructor code here
}


void AnalogicReader::manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue)
{
   
    // Check if the moduleAlias is valid
    if (moduleAlias.empty())
    { 
        returnedValue = std::numeric_limits<double>::min();
        return;
    }

    // Getting the device module based on the alias
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    if (!deviceModule) {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }

    // Check the module type
    ModuleType modType = deviceModule->getModuleType();  
    if (modType == isAnalogicInputCurrent || modType == isAnalogicInputVoltage) {
        double value;
        try {
            // Reading the value based on module type
            if (modType == isAnalogicInputCurrent) {
                value = m_daqMx->readCurrent(deviceModule, index, 50, true);
            } else { // modType == isAnalogicInputVoltage
                value = m_daqMx->readVoltage(deviceModule, index, 10);
            }

            returnedValue = value;
            onOneShotValueReaded(value); // Notify value read
        }
        catch (const std::exception &e) {
            // Logging the specific exception message
            onOneShotValueReaded(std::numeric_limits<double>::min());
            returnedValue = std::numeric_limits<double>::min();
        }
    } else {
        // Handle unexpected module type
        returnedValue = std::numeric_limits<double>::min();
    }
}


void AnalogicReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue)
{
    
    // Validate moduleAlias and chanName
    if (moduleAlias.empty() || chanName.empty()) 
    {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }

    // Getting the device module based on the alias
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    if (!deviceModule) 
    {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }

    // Check the module type
    ModuleType modType = deviceModule->getModuleType();
    std::string modName = deviceModule->getAlias();

    if (modType != isAnalogicInputCurrent && modType != isAnalogicInputVoltage) 
    {
        returnedValue = std::numeric_limits<double>::min();
        return;
    }
    
    std::vector<double> valueVector;
    double value = std::numeric_limits<double>::min(); // Initialize value
    unsigned int index = 0;
    try {
        // Reading the value based on module type
        if (modType == isAnalogicInputCurrent) 
        {
            if (modName=="Mod1") 
            {
                valueVector = m_daqMx->Mod1Buffer.copy();
            }
            else if (modName=="Mod2")
            {
                valueVector = m_daqMx->Mod2Buffer.copy();
            }
            index = extractChanIndex(chanName);        
            if (index < valueVector.size()) 
            {
                value = valueVector[index];
            } 
            else 
            {
                value = 0.0; // Set to NaN or another error-indicating value
            }
        } 
        else 
        { 
            if (modName=="Mod3") 
            {
                valueVector = m_daqMx->Mod3Buffer.copy();
                index = extractChanIndex(chanName);

            }
            
            if (index < valueVector.size()) 
            {
                value = valueVector[index];
            } 
            else 
            {
                value = 0.0; // Set to NaN or another error-indicating value
            }
        
        }

        returnedValue = value;
        onOneShotValueReaded(value); // Notify value read
    }
    catch (const std::exception &e) {
        // Logging the specific exception message
        onOneShotValueReaded(std::numeric_limits<double>::min());
        returnedValue = std::numeric_limits<double>::min();
    }
}
