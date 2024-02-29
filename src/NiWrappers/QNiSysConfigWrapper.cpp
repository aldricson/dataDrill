#include "QNiSysConfigWrapper.h"
#include <iostream>
#include <cstring>



// Implementation of the constructor
QNiSysConfigWrapper::QNiSysConfigWrapper() 
{
    // Initialize session
    const char *login = "";
    const char *password = "";
    unsigned int timeOut = 1000; //1 second timeout  
    NISysCfgInitializeSession("localhost",login, password, NISysCfgLocaleDefault, NISysCfgBoolFalse, timeOut, NULL, &sessionHandle);
    //build the lookup table for each type of module
    lookupModType.push_back("\n║ type: Analog Input Current"); //isAnalogicInputCurrent = 0
    lookupModType.push_back("\n║ type: Analog Input Voltage"); //isAnalogicInputVoltage = 1
    lookupModType.push_back("\n║ type: Digital Input");        //isDigitalInput         = 2
    lookupModType.push_back("\n║ type: Digital Output");       //isDigitalOutput        = 3
    lookupModType.push_back("\n║ type: Counter");              //isCounter              = 4
    lookupModType.push_back("\n║ type: Coder");                //isCoder                = 5
    //build the lookuptable for shunts location
    lookupShuntLocation[noShunt]          = "\n║ No shunt resistor for this module";
    lookupShuntLocation[defaultLocation]  = "\n║ Shunt in default location";
    lookupShuntLocation[internalLocation] = "\n║ Shunt in internal location";
    lookupShuntLocation[externalLocation] = "\n║ Shunt in external location";
    //build the lookup table for the channel units
    lookupUnits[NoUnit                    ]=   "No unit               ";
    lookupUnits[Val_Volts                 ]=   "Volts                 ";                                                
    lookupUnits[Val_Amps                  ]=   "Amps                  ";                               
    lookupUnits[Val_DegF                  ]=   "DegF                  ";                               
    lookupUnits[Val_DegC                  ]=   "DegC                  ";                               
    lookupUnits[Val_DegR                  ]=   "DegR                  ";                               
    lookupUnits[Val_Kelvins               ]=   "Kelvins               ";                               
    lookupUnits[Val_Strain                ]=   "Strain                ";                               
    lookupUnits[Val_Ohms                  ]=   "Ohms                  ";                               
    lookupUnits[Val_Hz                    ]=   "Hz                    ";                               
    lookupUnits[Val_Seconds               ]=   "Seconds               ";                               
    lookupUnits[Val_Meters                ]=   "Meters                ";                               
    lookupUnits[Val_Inches                ]=   "Inches                ";                               
    lookupUnits[Val_Degrees               ]=   "Degrees               ";                               
    lookupUnits[Val_Radians               ]=   "Radians               ";                               
    lookupUnits[Val_Ticks                 ]=   "Ticks                 ";                               
    lookupUnits[Val_RPM                   ]=   "RPM                   ";                               
    lookupUnits[Val_RadiansPerSecond      ]=   "RadiansPerSecond      ";                               
    lookupUnits[Val_DegreesPerSecond      ]=   "DegreesPerSecond      ";                               
    lookupUnits[Val_g                     ]=   "g                     ";                               
    lookupUnits[Val_MetersPerSecondSquared]=   "MetersPerSecondSquared";                               
    lookupUnits[Val_InchesPerSecondSquared]=   "InchesPerSecondSquared";                               
    lookupUnits[Val_MetersPerSecond       ]=   "MetersPerSecond       ";                               
    lookupUnits[Val_InchesPerSecond       ]=   "InchesPerSecond       ";                               
    lookupUnits[Val_Pascals               ]=   "Pascals               ";                               
    lookupUnits[Val_Newtons               ]=   "Newtons               ";                               
    lookupUnits[Val_Pounds                ]=   "Pounds                ";                               
    lookupUnits[Val_KilogramForce         ]=   "KilogramForce         ";                               
    lookupUnits[Val_PoundsPerSquareInch   ]=   "PoundsPerSquareInch   ";                               
    lookupUnits[Val_Bar                   ]=   "Bar                   ";                               
    lookupUnits[Val_NewtonMeters          ]=   "NewtonMeters          ";                               
    lookupUnits[Val_InchOunces            ]=   "InchOunces            ";                               
    lookupUnits[Val_InchPounds            ]=   "InchPounds            ";                               
    lookupUnits[Val_FootPounds            ]=   "FootPounds            ";                               
    lookupUnits[Val_VoltsPerVolt          ]=   "VoltsPerVolt          ";                               
    lookupUnits[Val_mVoltsPerVolt         ]=   "mVoltsPerVolt         ";                               
    lookupUnits[Val_Coulombs              ]=   "Coulombs              ";                               
    lookupUnits[Val_PicoCoulombs          ]=   "PicoCoulombs          ";                               
    lookupUnits[Val_FromTEDS              ]=   "FromTEDS              ";
}



// Implementation of the destructor
QNiSysConfigWrapper::~QNiSysConfigWrapper() {
    // Close session handle
    NISysCfgCloseHandle(sessionHandle);
}

void QNiSysConfigWrapper::generateReadbleInfo()
{

}

std::vector<std::string> QNiSysConfigWrapper::EnumerateCRIOPluggedModules() {
    std::vector<std::string>   modules;
    NISysCfgEnumResourceHandle resourceEnumHandle;

    char moduleName      [shortStringSize]; //product name of the module
    char moduleAlias     [shortStringSize]; //alias of the module e.g mod1
    unsigned int nb_chan          = 0;      //number of channels in the module
    unsigned int nb_counters      = 0;
    unsigned int nb_digitalOutput = 0;      //number of digital io port in the module
    double       analogChanMin    = 0.0;
    double       analogChanMax    = 10.0;
    unsigned int counterMin       = 0.0;
    unsigned int counterMax       = 4294967295;
    std::string  analogUnits      = "";
    ModuleType modType;
    moduleShuntLocation shuntLoc;
    double              shuntVal = 0.0; 
    int slotNumber;       //slot where is the module

    // Find hardware
    NISysCfgFindHardware(sessionHandle, NISysCfgFilterModeMatchValuesAll, NULL, NULL, &resourceEnumHandle);

    NISysCfgResourceHandle resourceHandle;
    int separatorCount = 0; //used as a crude way to isolate the part of interest
    while (NISysCfgNextResource(sessionHandle, resourceEnumHandle, &resourceHandle) == NISysCfg_OK) 
    {
        // Get the name of the module
        NISysCfgGetResourceProperty(resourceHandle, NISysCfgResourcePropertyProductName, moduleName);
        // Get the slot number
        NISysCfgGetResourceProperty(resourceHandle, NISysCfgResourcePropertySlotNumber, &slotNumber);
        //get the alias
        NISysCfgGetResourceIndexedProperty(resourceHandle, NISysCfgIndexedPropertyExpertUserAlias, 0, moduleAlias);
        
      

        if (std::strcmp(moduleName, "") == 0) {
            separatorCount++;
        }

        if (separatorCount >= 2) {
           
            removeSpacesFromCharStar(moduleName);
            std::string shortedModuleName = removeSpacesFromCharStar(moduleName);
             //output information for debug purpose
            std::string moduleInfo = std::string(moduleName) + 
                                     "\n║ Alias: "        + moduleAlias+
                                     "\n║ Slot: "         + std::to_string(slotNumber);
            //generate a device module object from the productname
            auto module = NIDeviceModuleFactory::createModule(shortedModuleName);
            if (module) 
            {
                //add to our list of modules
                moduleList.push_back(module);
                //set what we must
                module->setAlias(std::string(moduleAlias));
                module->setSlotNb(slotNumber);
                //load previous config if it exists (otherwise this will be default values of the module)
                module->loadConfig();
                //get what we need (or from the config file or default if it's the first run)
                nb_chan          = module->getNbChannel          ();
                nb_counters      = module->getNbCounters         ();
                modType          = module->getModuleType         ();
                nb_digitalOutput = module->getNbDigitalOutputs   ();
                analogChanMax    = module->getChanMax            ();
                analogChanMin    = module->getChanMin            ();
                analogUnits      = module->getModuleUnit         ();
                counterMax       = module->getmaxCounters        ();
                counterMin       = module->getminCounters        ();
                shuntLoc         = module->getModuleShuntLocation();
                shuntVal         = module->getModuleShuntValue   ();
                //after setting the properties we could retrieve from NISysConfig
                //let's ensure our config files stay synchronized
                module->saveConfig();
                //convert it to string for debug purpose
                std::string modTypeAsString = "";
                switch (modType)
                {
                    case isAnalogicInputCurrent :
                    {
                        modTypeAsString = "\n║ type: Analog Input Current";
                        break;
                    }

                    case isAnalogicInputVoltage :
                    {
                        modTypeAsString = "\n║ type: Analog Input Voltage";
                        break;
                    }

                   case isDigitalInput :
                   {
                       modTypeAsString = "\n║ type: Digital Input Voltage";
                       break;
                   }

                   
                   case isDigitalOutput :
                   {
                       modTypeAsString = "\n║ type: Digital Output Voltage";
                       break;
                   }

                   case isCounter :
                   {
                       modTypeAsString = "\n║ type: counter";
                       break;
                   }

                   case isCoder :
                   {
                       modTypeAsString = "\n║ type: coder";
                       break;
                   }

                   default :
                   {
                       modTypeAsString = "\n║ type: Not recognized";
                       break;
                   }

                } 
                
           
                switch (shuntLoc)
                { 
                    case noShunt :
                    {
                        moduleInfo += "\n║ No shunt resistor for this module" ;
                        break;
                    }

                    case defaultLocation :
                    {
                        moduleInfo += "\n║ shunt location: default" ;
                        moduleInfo += "\n║ shunt value   : "+std::to_string(shuntVal);
                        break;
                    }

                    case internalLocation :
                    {
                        moduleInfo += "\n║ shunt location: internal" ;
                        moduleInfo += "\n║ shunt value   : "+std::to_string(shuntVal);
                        break;
                    }
                    case externalLocation :
                    {
                        moduleInfo += "\n║ shunt location: external" ;
                        moduleInfo += "\n║ shunt value   : "+std::to_string(shuntVal);
                        break;
                    }
                    default :
                    {
                        moduleInfo += "\n║ shunt location: /!\\ NOT RECOGNIZED" ;
                        break;
                    }
                }
                

                moduleInfo += modTypeAsString+
                              "\n║ nb digital output: "  + 
                                std::to_string(nb_digitalOutput) +
                              "\n║ nb channels: "  + 
                                std::to_string(nb_chan)+
                              "\n║ nb counters: "+
                               std::to_string(nb_counters);
                std::vector<std::string> channelNames = module->getChanNames();
                for (long unsigned int i=0;i<channelNames.size();++i)
                {
                   moduleInfo += "\n║ ╬"+ channelNames[i];
                }
                if (channelNames.size()>0)
                {
                    moduleInfo += "\n║ Analog min value  : "   + std::to_string(analogChanMin);
                    moduleInfo += "\n║ Analog max value  : "   + std::to_string(analogChanMax);
                    moduleInfo += "\n║ Analog input unit : "   + analogUnits;
                }

                std::vector<std::string> counterNames = module->getCounterNames();
                for (long unsigned int i=0;i<counterNames.size();++i)
                {
                   moduleInfo += "\n║ ╬"+ counterNames[i];
                }

                if (counterNames.size()>0)
                {
                    //in case of counters
                    moduleInfo += "\n║ 32 bits counters";
                    moduleInfo += "\n║ Counter Min Value : " + std::to_string(counterMin);
                    moduleInfo += "\n║ Counter Max Value : " + std::to_string(counterMax);
                }  
              module->setModuleInfo(moduleInfo);
            } 
            else 
            {
               moduleInfo += "\n║ Module inner definition not yet implemented";
            }
                                                
            modules.push_back(moduleInfo);
            
        }

        // Close the resource handle
        NISysCfgCloseHandle(resourceHandle);
    }

    // Close the resource enumeration handle
    NISysCfgCloseHandle(resourceEnumHandle);

    if (modules.size() >= 2) 
    {
        modules.erase(modules.begin(), modules.begin() + 2);
    } else {
        // Handle the case where there are fewer than 2 elements
    }

    return modules;
}


 //   std::vector<std::string> QNiSysConfigWrapper::EnumerateCRIOPluggedModules() {
 //          std::vector<std::string>   modules;
 //       NISysCfgEnumResourceHandle resourceEnumHandle;
 //   
 //       char moduleName      [shortStringSize]; //product name of the module
 //       char moduleAlias     [shortStringSize]; //alias of the module e.g mod1
 //       unsigned int nb_chan          = 0;      //number of channels in the module
 //       unsigned int nb_counters      = 0;
 //       unsigned int nb_digitalIoPort = 0;      //number of digital io port in the module
 //       double       analogChanMin    = 0.0;
 //       double       analogChanMax    = 10.0;
 //       unsigned int counterMin       = 0.0;
 //       unsigned int counterMax       = 4294967295;
 //       std::string  analogUnits      = "";
 //       moduleType modType;
 //       moduleShuntLocation shuntLoc;
 //       double              shuntVal = 0.0; 
 //       int slotNumber;       //slot where is the module
 //   
 //       // Find hardware
 //       NISysCfgFindHardware(sessionHandle, NISysCfgFilterModeMatchValuesAll, NULL, NULL, &resourceEnumHandle);
 //   
 //       NISysCfgResourceHandle resourceHandle;
 //       int separatorCount = 0; //used as a crude way to isolate the part of interest
 //       while (NISysCfgNextResource(sessionHandle, resourceEnumHandle, &resourceHandle) == NISysCfg_OK) 
 //       {
 //           // Get the name of the module
 //           NISysCfgGetResourceProperty(resourceHandle, NISysCfgResourcePropertyProductName, moduleName);
 //           // Get the slot number
 //           NISysCfgGetResourceProperty(resourceHandle, NISysCfgResourcePropertySlotNumber, &slotNumber);
 //           //get the alias
 //           NISysCfgGetResourceIndexedProperty(resourceHandle, NISysCfgIndexedPropertyExpertUserAlias, 0, moduleAlias);
 //           
 //         
 //   
 //           if (std::strcmp(moduleName, "") == 0) {
 //               separatorCount++;
 //           }
 //   
 //           if (separatorCount >= 2) {
 //              
 //               removeSpacesFromCharStar(moduleName);
 //               std::string shortedModuleName = removeSpacesFromCharStar(moduleName);
 //                //output information for debug purpose
 //               std::string moduleInfo = std::string(moduleName) + 
 //                                        "\n║ Alias: "        + moduleAlias+
 //                                        "\n║ Slot: "         + std::to_string(slotNumber);
 //               //generate a device module object from the productname
 //               auto module = NIDeviceModuleFactory::createModule(shortedModuleName);
 //               if (module) 
 //               {
 //                   module->setModuleName (std::string(moduleAlias));
 //                   module->setAlias      (std::string(moduleAlias));
 //                   module->setSlotNb (slotNumber);
 //                   //add to our list of modules
 //                   moduleList.push_back(module);
 //                  //set what we must
 //                  //load previous config if it exists (otherwise this will be default values of the module)
 //                  
 //                  //****************//
 //                  //SEGFAULT NEXT LINE
 //                  
 //                   module->loadConfig();
 //                  
 //                  
 //                  
 //                  //get what we need (or from the config file or default if it's the first run)
 //                  nb_chan          = module->getNbChannel          ();
 //                  nb_counters      = module->getNbCounters         ();
 //                  modType          = module->getModuleType         ();
 //                  nb_digitalIoPort = module->getNbDigitalIOPorts   ();
 //                  analogChanMax    = module->getChanMax            ();
 //                  analogChanMin    = module->getChanMin            ();
 //                  analogUnits      = lookupUnits[module->getModuleUnit()];
 //                  counterMax       = module->getmaxCounters        ();
 //                  counterMin       = module->getminCounters        ();
 //                  shuntLoc         = module->getModuleShuntLocation();
 //                  shuntVal         = module->getModuleShuntValue   ();
 //                  //after setting the properties we could retrieve from NISysConfig
 //                  //let's ensure our config files stay synchronized
 //                  module->saveConfig();
 //                  //convert it to string for user comfort purpose
 //                  std::string modTypeAsString = "";
 //                  // get all the needed for lookup tables
 //                  modTypeAsString = lookupModType[modType];
 //                  moduleInfo += lookupShuntLocation[shuntLoc];                
 //                  moduleInfo += "\n║ Shunt value: "+std::to_string(shuntVal);
 //                  moduleInfo += modTypeAsString+
 //                                "\n║ nb digital IO port: "  + 
 //                                  std::to_string(nb_digitalIoPort) +
 //                                "\n║ nb channels: "  + 
 //                                  std::to_string(nb_chan)+
 //                                "\n║ nb counters: "+
 //                                 std::to_string(nb_counters);
 //                  std::vector<std::string> channelNames = module->getChanNames();
 //                  for (long unsigned int i=0;i<channelNames.size();++i)
 //                  {
 //                     moduleInfo += "\n║ ╬"+ channelNames[i];
 //                  }
 //                  if (channelNames.size()>0)
 //                  {
 //                      moduleInfo += "\n║ Analog min value  : "   + std::to_string(analogChanMin);
 //                      moduleInfo += "\n║ Analog max value  : "   + std::to_string(analogChanMax);
 //                      moduleInfo += "\n║ Analog input unit : "   + analogUnits;
 //                  }
 //   //
 //                  std::vector<std::string> counterNames = module->getCounterNames();
 //                  for (long unsigned int i=0;i<counterNames.size();++i)
 //                  {
 //                     moduleInfo += "\n║ ╬"+ counterNames[i];
 //                  }
 //   //
 //                  if (counterNames.size()>0)
 //                  {
 //                      //in case of counters
 //                      moduleInfo += "\n║ 32 bits counters";
 //                      moduleInfo += "\n║ Counter Min Value : " + std::to_string(counterMin);
 //                      moduleInfo += "\n║ Counter Max Value : " + std::to_string(counterMax);
 //                  }  
 //                module->setModuleInfo(moduleInfo);
 //               } 
 //               //else 
 //               {
 //                  moduleInfo += "\n║ Module inner definition not yet implemented";
 //               }
 //                                                   
 //               modules.push_back(moduleInfo);
 //               
 //           }
 //   
 //           // Close the resource handle
 //           NISysCfgCloseHandle(resourceHandle);
 //       }
 //   
 //       // Close the resource enumeration handle
 //       NISysCfgCloseHandle(resourceEnumHandle);
 //   
 //       if (modules.size() >= 2) 
 //       {
 //           modules.erase(modules.begin(), modules.begin() + 2);
 //       } else {
 //           // Handle the case where there are fewer than 2 elements
 //       }
 //   
 //       return modules;
 //   }

// Method to check if a property is present for a given resource
bool QNiSysConfigWrapper::IsPropertyPresent(NISysCfgResourceHandle resourceHandle, NISysCfgResourceProperty propertyID)
{
       NISysCfgIsPresentType isPresent;
        NISysCfgStatus status = NISysCfgGetResourceProperty(resourceHandle, propertyID, &isPresent);

        if (status == NISysCfg_OK) 
        {
            return (isPresent == NISysCfgIsPresentTypePresent);
        } 
        else 
        {
            std::cout << "An error occurred while checking for the device presence." << std::endl;
            // Handle error
            return false;
        }
}

//********************* getters **********************
// Function to get a module by its index
NIDeviceModule *  QNiSysConfigWrapper::getModuleByIndex(size_t index) {
    if (index >= moduleList.size()) {
        throw std::out_of_range("Index out of range");
    }
    return moduleList[index];
}

// Function to get a module by its alias
NIDeviceModule * QNiSysConfigWrapper::getModuleByAlias(const std::string& alias) {
    std::string lowerAlias = toLowerCase(alias);
    for (auto& module : moduleList) {
        if (toLowerCase(module->getAlias()) == lowerAlias) {  
            return module;
        }
    }
    throw std::invalid_argument("Module with given alias not found");
}


NIDeviceModule * QNiSysConfigWrapper::getModuleBySlot(unsigned int slotNb) {
        for (auto& module : moduleList) {
        if (module->getSlotNb() == slotNb) {  
            return module;
        }
    }
    throw std::invalid_argument("Module with given alias not found");

}

// Getter for moduleList
std::vector<NIDeviceModule*> QNiSysConfigWrapper::getModuleList() const {
    return moduleList;
}

// Setter for moduleList
void QNiSysConfigWrapper::setModuleList(const std::vector<NIDeviceModule*>& newModuleList) {
    moduleList = newModuleList;

    // Emit the signal if it's connected
    if (moduleListChangedSignal) {
        moduleListChangedSignal(moduleList, this);
    }
}
