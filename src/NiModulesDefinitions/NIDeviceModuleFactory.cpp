#include "NIDeviceModuleFactory.h"
#include "NI9208.h"
#include "NI9239.h"
#include "NI9423.h"
#include "NI9411.h"
#include "NI9481.h"
// Include other module headers here



template <typename T>
T* NIDeviceModuleFactory::createAndConfigureModule() {
    auto result = new T;
    return result;
}


NIDeviceModule* NIDeviceModuleFactory::createModule(const std::string& productName) {
    if (productName == "NI9208") {
        //std::cout<<"Factory create NI9208"<<std::endl;
        return createAndConfigureModule<NI9208>();
    }
    else
        if (productName == "NI9239") {
        //std::cout<<"Factory create NI9239"<<std::endl;
         return createAndConfigureModule<NI9239>();
    }
    else
        if (productName == "NI9423") {
        //std::cout<<"Factory create NI9423"<<std::endl;            
        return createAndConfigureModule<NI9423>();
    }
    else
        if (productName == "NI9411") {
        //std::cout<<"Factory create NI9411"<<std::endl;
        return createAndConfigureModule<NI9411>();
    }
        else
        if (productName == "NI9481") {
        //std::cout<<"Factory create NI9481"<<std::endl;
        return createAndConfigureModule<NI9481>();
    }
    // Add other product names and their corresponding classes here


    return nullptr; // or throw an exception if an unknown product name is passed
}
