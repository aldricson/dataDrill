#ifndef NIDEVICEMODULE_H
#define NIDEVICEMODULE_H

#include <vector>
#include <cstring>
#include <string.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>

#include "../filesUtils/iniObject.h"
#include "../filesUtils/cPosixFileHelper.h"
#include "../filesUtils/appendToFileHelper.h"
#include "../globals/globalEnumStructs.h"


#include "../config.h"
#ifdef CrossCompiled
  #include <NIDAQmx.h>
#else
  #include "../../DAQMX_INCLUDE/NIDAQmx.h"
#endif



class NIDeviceModule {
private:
bool loadModules (const std::string &filename,       ModuleType &aModuleType);
bool loadChannels(const std::string &filename, const ModuleType &aModuleType);
bool loadCounters(const std::string &filename, const ModuleType &aModuleType);
bool loadOutputs (const std::string &filename, const ModuleType &aModuleType);

//
void saveModules (const std::string &filename ,      ModuleType &aModuleType);
void saveChannels(const std::string &filename, const ModuleType &aModuleType);
void saveCounters(const std::string &filename, const ModuleType &aModuleType);
void saveOutputs (const std::string &filename, const ModuleType &aModuleType);

protected:
    //number of channels in the module
    unsigned int m_nbChannel       = 16;
    //number of 
   
 
    unsigned int m_slotNumber      = 0  ;
    double       m_analogChanMin   = 0.0;
    double       m_analogChanMax   = 4.0;
    std::vector<std::string> m_chanNames;
    //-------- counters -----------------
    unsigned int m_nbCounters       = 0;
    unsigned int m_counterMin       = 0;
    unsigned int m_counterMax       = 4294967295; //32 bits
    moduleCounterEdgeConfig  m_counterCountingEdgeMode;
    moduleCounterMode        m_counterCountDirectionMode;
    std::vector<std::string> m_counterNames;
    //----------- relays (digital outputs) ----------
    unsigned int m_nbDigitalOutputs = 0; //number of outputs for a digital ouput channel (e.g. for relays)
    std::vector<std::string> m_digitalOutputNames;
    //----------- modules ------------------------

    ModuleType           m_moduleType;
    moduleUnit           m_moduleUnit       = NoUnit;
    std::string          m_moduleName      = "";
    std::string          m_alias           = "";
    std::string          m_moduleInfo      = "";        
    moduleShuntLocation  m_shuntLocation = defaultLocation;
    double               m_shuntValue    = 0.0;
    moduleTerminalConfig m_moduleTerminalConfig = noTerminalConfig;

    



    GlobalFileNamesContainer m_fileNamesContainer;

    std::shared_ptr<IniObject> m_ini;  
public:
    NIDeviceModule();
    
    virtual ~NIDeviceModule() {}
    virtual void loadFromFile(const std::string& filename);
    virtual void saveToFile  (const std::string& filename);


    virtual std::string getAlias();
    

    virtual void initModule()                          = 0;
    virtual std::string              getModuleName                () const;
    virtual unsigned int             getNbChannel                 () const;
    virtual unsigned int             getNbCounters                () const;
    virtual unsigned int             getSlotNb                    () const;
    virtual std::string              getModuleInfo                () const;
    virtual std::vector<std::string> getChanNames                 () const;
    virtual std::vector<std::string> getCounterNames              () const;
    virtual moduleCounterEdgeConfig  getcounterCountingEdgeMode   () const; 
    virtual moduleCounterMode        getCounterCountDirectionMode () const;
    virtual unsigned int             getNbDigitalOutputs          () const;
    virtual ModuleType               getModuleType                () const;
    virtual moduleShuntLocation      getModuleShuntLocation       () const;
    virtual double                   getModuleShuntValue          () const;
    virtual moduleTerminalConfig     getModuleTerminalCfg         () const;
    virtual moduleUnit               getModuleUnit                () const; 
    virtual double                   getChanMin                   () const;
    virtual double                   getChanMax                   () const;
    virtual unsigned int             getminCounters               () const;
    virtual unsigned int             getmaxCounters               () const;
      


    virtual void setModuleName                (const std::string& newModuleName);
    virtual void setNbChannel                 (unsigned int newNbChannels);
    virtual void setModuleInfo                (std::string newModuleInfo); 
    virtual void setModuleShuntLocation       (moduleShuntLocation newLocation);
    virtual void setModuleShuntValue          (double newValue);
    virtual void setModuleTerminalCfg         (moduleTerminalConfig newTerminalConfig); 
    virtual void setSlotNb                    (unsigned int newSlot);
    virtual void setAlias                     (const std::string& newAlias);
    virtual void setChanNames                 (const std::vector<std::string>& names                     );
    //-----------Counters--------
    virtual void setNbCounters                (unsigned int newNbCounters);
    virtual void setCounterNames              (const std::vector<std::string>& names                     );
    virtual void setcounterCountingEdgeMode   (moduleCounterEdgeConfig         newCounterCountingEdgeMode);
    virtual void setCounterCountDirectionMode (moduleCounterMode               newCounterCountMode       );
    virtual void setCounterMin                (unsigned int                    newCountersMin            );
    virtual void setCounterMax                (unsigned int                    newCountersMax            );
    //----------Digital outputs------------
    virtual void setNbDigitalOutputs          (unsigned int                    newNbDigitalOutpits       );

    virtual void setModuleType           (ModuleType newType);
    virtual void setModuleUnit           (moduleUnit newUnit); 

    virtual void setChanMin              (double     newChanMin);
    virtual void setChanMax              (double newChanMax);

   


     
    virtual void loadConfig()  = 0;
    virtual void saveConfig()  = 0; 

//**********************************
//***     PURE C++ SIGNALS      ****
//**********************************
    //module signals
    std::function<void(std::string             , NIDeviceModule *sender)>  moduleNameChangedSignal           = nullptr;
    std::function<void(moduleShuntLocation     , NIDeviceModule *sender)>  moduleShuntLocationChangedSgnal   = nullptr;
    std::function<void(double                  , NIDeviceModule *sender)>  moduleShuntValueChangedSignal     = nullptr;
    std::function<void(moduleTerminalConfig    , NIDeviceModule *sender)>  moduleTerminalConfigChangedSignal = nullptr;
    std::function<void(unsigned int            , NIDeviceModule *sender)>  moduleSlotNumberChangedSignal     = nullptr;
    std::function<void(std::string             , NIDeviceModule *sender)>  moduleInfoChangedSignal           = nullptr;
    //channels signals
    std::function<void(unsigned int            , NIDeviceModule *sender)>  nbChannelsChangedSignal           = nullptr;
    std::function<void(std::vector<std::string>, NIDeviceModule *sender)>  chanNamesChangedSignal            = nullptr;
    std::function<void(double                  , NIDeviceModule *sender)>  chanMinChangedSignal              = nullptr; 
    std::function<void(double                  , NIDeviceModule *sender)>  chanMaxChangedSignal              = nullptr;
    //Counter signals
    std::function<void(unsigned int            , NIDeviceModule *sender)>  nbCountersChangedSignal           = nullptr;
    std::function<void(unsigned int            , NIDeviceModule *sender)>  countersMinChangedSignal          = nullptr; 
    std::function<void(unsigned int            , NIDeviceModule *sender)>  countersMaxChangedSignal          = nullptr;
    std::function<void(moduleCounterEdgeConfig , NIDeviceModule *sender)>  counterEdgeConfigChangedSignal    = nullptr;
    std::function<void(moduleCounterMode       , NIDeviceModule *sender)>  counterModeChangedSignal          = nullptr;
    //digital outputs
    std::function<void(unsigned int            , NIDeviceModule *Sender)> nbDigitalOutputsChangedSignal     = nullptr;

    std::function<void(unsigned int            , NIDeviceModule *sender)>  nbDigitalIoPortsChangedSignal     = nullptr;



    std::function<void(moduleUnit              , NIDeviceModule *sender)>  chanUnitChangedSignal             = nullptr;
        
    std::function<void(std::vector<std::string>, NIDeviceModule *sender)>  counterNamesChangedSignal         = nullptr;


};

#endif // NIDEVICEMODULE_H
