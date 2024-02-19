#ifndef NI9239_H
#define NI9239_H

#include "NIDeviceModule.h"
#include <vector>
#include <string>
#include <fstream>

class NI9239 : public NIDeviceModule {
private:


public:
    NI9239();  // Constructor

    void initModule()                       override;  // Initialize the module with default values
    std::vector<std::string> getChanNames() const override;  // Get the names of the channels

    void loadConfig()  override ;
    void saveConfig()  override ;  // Save the module configuration to a file
};

#endif // NI9239_H
