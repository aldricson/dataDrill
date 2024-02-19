#ifndef NI9423_H
#define NI9423_H

#include "NIDeviceModule.h"
#include <vector>
#include <string>
#include <fstream>


class NI9423 : public NIDeviceModule {
private:


public:
    NI9423();

    void initModule()  override;
    void loadConfig()  override; 
    void saveConfig()  override;



};

#endif // NI9423_H
