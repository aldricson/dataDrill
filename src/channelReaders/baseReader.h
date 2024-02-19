#ifndef BaseReader_H
#define BaseReader_H

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


class BaseReader {
public:
    // Constructor 
    BaseReader(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
               std::shared_ptr<QNiDaqWrapper> aDaqMxInstance);
    // Destructor
    ~BaseReader();
    //raw data acquisition PURE VIRTUAL
    virtual void manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue) = 0;
    virtual void manualReadOneShot(const std::string &moduleAlias, const std::string  &chanName, double &returnedValue) = 0;
    //acquisition 
    virtual void selectModuleAndChannel(const std::string& moduleName, const std::string& channelName);
    //public slots
    virtual void onChannelDataReady (double lastValue,QNiDaqWrapper *sender);
    virtual void onOneShotValueReaded(double aValue);
    // Getters
    virtual std::shared_ptr<QNiSysConfigWrapper> getSysConfig() const;
    virtual std::shared_ptr<QNiDaqWrapper>       getDaqMx()     const;
    // Setters
    virtual void setSysConfig(const std::shared_ptr<QNiSysConfigWrapper>& newSysConfig);
    virtual void setDaqMx    (const std::shared_ptr<QNiDaqWrapper>&       newDaqMx    );
    // Signals
    std::function<void(std::shared_ptr<QNiSysConfigWrapper>, BaseReader* sender)> sysConfigChangedSignal  = nullptr;
    std::function<void(std::shared_ptr<QNiDaqWrapper>,       BaseReader* sender)> daqMxChangedSignal      = nullptr;
    std::function<void()> showMainMenuSignal = nullptr;


protected:
    std::shared_ptr<QNiSysConfigWrapper> m_sysConfig        ; //wrapper around NiSysConfig

    std::shared_ptr<QNiDaqWrapper>       m_daqMx            ; //wrapper around NiDaqMx


    char            m_manuallySelectedModuleName[256] = ""      ;
    char            m_manuallySelectedChanName  [256] = ""      ;
    unsigned int    m_manuallySelectedChanIndex       =  0      ;
    NIDeviceModule *m_manuallySelectedModule          = nullptr ;
};


#endif // BaseReader_H
