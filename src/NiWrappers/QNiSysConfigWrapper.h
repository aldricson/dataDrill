#ifndef QNISYSCONFIGWRAPPER_H
#define QNISYSCONFIGWRAPPER_H

#include <vector>
#include <string>

#include "../config.h"
#ifdef CrossCompiled
 #include <nisyscfg.h>
#else
  #include "../../NISYSCFG_INCLUDE/nisyscfg.h"
#endif



#include <stdio.h>
#include <vector>
#include <map>
#include "../globals/globalConsts.h"
#include "../stringUtils/stringUtils.h"
#include "../NiModulesDefinitions/NIDeviceModule.h"
#include "../NiModulesDefinitions/NIDeviceModuleFactory.h"


class QNiSysConfigWrapper {
public:
    QNiSysConfigWrapper();
    ~QNiSysConfigWrapper();

    // Method to enumerate cRIO modules and their properties
    std::vector<std::string> EnumerateCRIOPluggedModules();
    NIDeviceModule * getModuleByIndex(size_t index);
    NIDeviceModule * getModuleBySlot(unsigned int slotNb);
    NIDeviceModule * getModuleByAlias(const std::string& alias);
    bool IsPropertyPresent(NISysCfgResourceHandle resourceHandle, NISysCfgResourceProperty propertyID);

     // Getter and Setter for moduleList
    std::vector<NIDeviceModule*> getModuleList() const;
    void setModuleList(const std::vector<NIDeviceModule*>& newModuleList);
    
    // Signal for moduleList changes
    std::function<void(const std::vector<NIDeviceModule*>&, QNiSysConfigWrapper*)> moduleListChangedSignal;

protected:
 //simple std::vector for this lookup table is ordered, it's an exception
  std::vector<std::string>   lookupModType;
  std::map<int, std::string> lookupShuntLocation;
  std::map<int, std::string> lookupUnits; 

private:

   NISysCfgSessionHandle sessionHandle;       // Session handle
   std::vector<NIDeviceModule*> moduleList;
   void generateReadbleInfo();    
};


#endif // QNISYSCONFIGWRAPPER_H