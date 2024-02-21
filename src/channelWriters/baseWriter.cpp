#include "BaseWriter.h"

//------- Creator --------

BaseWriter::BaseWriter(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance, std::shared_ptr<QNiDaqWrapper> aDaqMxInstance)
{
     //thread safe atomic variables
    m_sysConfig = aSysConfigInstance; //the object to handle Ni configuration via NiSysConfig API
    m_daqMx     = aDaqMxInstance;     //the object that handles IO operations withs the devices (crio and its modules)   
}

//---------- destructor -------------

BaseWriter::~BaseWriter()
{
}

void BaseWriter::selectModuleAndChannel(const std::string &moduleName, const std::string &channelName)
{
    // Constants for buffer sizes
    const size_t moduleNameSize = sizeof(m_manuallySelectedModuleName);
    const size_t channelNameSize = sizeof(m_manuallySelectedChanName);

    // Truncate and copy moduleName into m_manuallySelectedModuleName
    std::string truncatedModuleName = moduleName.substr(0, moduleNameSize - 1);
    std::copy(truncatedModuleName.begin(), truncatedModuleName.end(), m_manuallySelectedModuleName);
    m_manuallySelectedModuleName[truncatedModuleName.length()] = '\0'; // Ensure null-termination

    // Truncate and copy channelName into m_manuallySelectedChanName
    std::string truncatedChannelName = channelName.substr(0, channelNameSize - 1);
    std::copy(truncatedChannelName.begin(), truncatedChannelName.end(), m_manuallySelectedChanName);
    m_manuallySelectedChanName[truncatedChannelName.length()] = '\0'; // Ensure null-termination
}



//----------- getters ---------------

std::shared_ptr<QNiSysConfigWrapper> BaseWriter::getSysConfig() const
{
    return m_sysConfig;
}

std::shared_ptr<QNiDaqWrapper> BaseWriter::getDaqMx() const
{
    return m_daqMx;
}

//-------------- setters ----------

void BaseWriter::setSysConfig(const std::shared_ptr<QNiSysConfigWrapper> &newSysConfig)
{
    m_sysConfig = newSysConfig;
    if (sysConfigChangedSignal) 
    {
        sysConfigChangedSignal(m_sysConfig, this);
    }
}

void BaseWriter::setDaqMx(const std::shared_ptr<QNiDaqWrapper> &newDaqMx)
{
    m_daqMx = newDaqMx;
    if (daqMxChangedSignal) 
    {
        daqMxChangedSignal(m_daqMx, this);
    }
}
