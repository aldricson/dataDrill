#ifndef NI9411_H
#define NI9411_H

#include "NIDeviceModule.h"
#include <vector>
#include <string>
#include <fstream>

class NI9411 : public NIDeviceModule {
private:


public:
    NI9411();

    void initModule()  override;
    void loadConfig()  override; 
    void saveConfig()  override;



};

#endif // NI9411_H
