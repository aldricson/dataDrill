#ifndef BaselWriter_H
#define BaselWriter_H

#include <memory>
#include <functional>
#include <sstream>
#include <limits>
#include <atomic>
#include <termios.h>

#include "../NiWrappers/QNiSysConfigWrapper.h"
#include "../NiWrappers/QNiDaqWrapper.h"
#include "../NiModulesDefinitions/NIDeviceModule.h"
#include "../stringUtils/stringUtils.h"


class BaseWriter {
public:
    // Constructor 
    BaseWriter(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
                std::shared_ptr<QNiDaqWrapper> aDaqMxInstance);
    // Destructor
    ~BaseWriter();
    //set the state of a digital Output PURE VIRTUAL
    virtual void manualSetOutput(const std::string &moduleAlias, const unsigned int &index,const bool &state) = 0;
    virtual void manualSetOutput(const std::string &moduleAlias, const std::string  &chanName,const bool &state) = 0; 
    //acquisition 
    virtual void selectModuleAndChannel(const std::string& moduleName, const std::string& channelName);
    // Getters
    virtual std::shared_ptr<QNiSysConfigWrapper> getSysConfig() const;
    virtual std::shared_ptr<QNiDaqWrapper>       getDaqMx()     const;
    // Setters
    virtual void setSysConfig(const std::shared_ptr<QNiSysConfigWrapper>& newSysConfig);
    virtual void setDaqMx    (const std::shared_ptr<QNiDaqWrapper>&       newDaqMx    );
    // Signals
    std::function<void(std::shared_ptr<QNiSysConfigWrapper>, BaseWriter* sender)> sysConfigChangedSignal  = nullptr;
    std::function<void(std::shared_ptr<QNiDaqWrapper>,       BaseWriter* sender)> daqMxChangedSignal      = nullptr;


protected:
    std::shared_ptr<QNiSysConfigWrapper> m_sysConfig        ; //wrapper around NiSysConfig

    std::shared_ptr<QNiDaqWrapper>       m_daqMx            ; //wrapper around NiDaqMx


    char            m_manuallySelectedModuleName[256] = ""      ;
    char            m_manuallySelectedChanName  [256] = ""      ;
    unsigned int    m_manuallySelectedChanIndex       =  0      ;
    NIDeviceModule *m_manuallySelectedModule          = nullptr ;
};


#endif // BaselWriter_H
