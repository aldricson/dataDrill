#ifndef NIDEVICEMODULEFACTORY_H
#define NIDEVICEMODULEFACTORY_H

#include "NIDeviceModule.h"
#include <string>
#include <memory>
#include "../filesUtils/cPosixFileHelper.h"

class NIDeviceModuleFactory {
public:
    static NIDeviceModule* createModule(const std::string& productName);

private:
        template <typename T>
    static T* createAndConfigureModule();
};

#endif // NIDEVICEMODULEFACTORY_H
