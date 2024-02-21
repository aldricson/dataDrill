#include "digitalWriter.h"

// Constructor
DigitalWriter::DigitalWriter(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
                             std::shared_ptr<QNiDaqWrapper> aDaqMxInstance)
    : BaseWriter(aSysConfigInstance, aDaqMxInstance)
{
    // Add any initialization specific to DigitalReader here
}

void DigitalWriter::manualSetOutput(const std::string &moduleAlias, const unsigned int &index, const bool &state)
{
      // Validate moduleAlias
    if (moduleAlias.empty()) 
    {
        //TODO a better error handling
        return;
    }
     // Fetch the device module by alias
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    if (!deviceModule) 
    {
        //TODO , handle this error
        return;
    }
      // Ensure the module is the correct type (digital input/counter)
    if (deviceModule->getModuleType() == ModuleType::isDigitalOutput) 
    {
        
        try 
        {
            m_daqMx->setRelayState(deviceModule,index,state);
        } 
        catch (const std::exception& e)
        {
            // TODO a true error handling
            return;
        }
    }
    else
    {        
        std::cout<<"module type not handled yet: "<<moduleAlias.c_str()<<" type:"<<deviceModule->getModuleType();
        return;
    }
}

void DigitalWriter::manualSetOutput(const std::string &moduleAlias, const std::string &chanName, const bool &state)
{
    // Check if module alias or channel name is empty and return an error value if so.
    if (moduleAlias.empty() || chanName.empty()) 
    {
        // TODO a true error handling
        return;
    }
    // Attempt to fetch the device module using the provided alias.
    NIDeviceModule *deviceModule = m_sysConfig->getModuleByAlias(moduleAlias);
    // If no module is found, return an error value.
    if (!deviceModule) 
    {
        // TODO a true error handling
        return;
    }
    // Ensure the module is the correct type (digital input/counter)
    if (deviceModule->getModuleType() == ModuleType::isDigitalOutput) 
    { 
        std::cout<<"manualSetOutput:"<< deviceModule->getModuleName().c_str() << "isDigitalOutpu: ready to dive into NidaqMx API"<<std::endl;
        try 
        {
            m_daqMx->setRelayState(deviceModule,chanName,state);
        } 
        catch (const std::exception& e)
        {
            // TODO a true error handling
            return;
        }
    }
    else
    {        
        std::cout<<"module type not handled yet: "<<moduleAlias.c_str()<<" type:"<<deviceModule->getModuleType();
        return;
    }
}

