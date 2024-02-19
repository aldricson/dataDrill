#ifndef NI9481_H
#define NI9481_H

#include "NIDeviceModule.h"
#include <vector>
#include <string>
#include <fstream>


class NI9481 : public NIDeviceModule {
private:


public:
    NI9481();

    void initModule()  override;
    void loadConfig()  override; 
    void saveConfig()  override;



};

#endif // NI9481_H
