#ifndef DigitalReader_H
#define DigitalReader_H

#include "baseReader.h"
#include "../globals/globalEnumStructs.h"
#include "../filesUtils/appendToFileHelper.h"

class DigitalReader : public BaseReader {
public:
    // Constructor
    DigitalReader(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
                  std::shared_ptr<QNiDaqWrapper> aDaqMxInstance);
    
    // Override the pure virtual functions
    void manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue) override;
    void manualReadOneShot(const std::string &moduleAlias, const std::string  &chanName, double &returnedValue) override;
    
    // Add any additional member functions specific to DigitalReader here
private:
    GlobalFileNamesContainer fileNamesContainer;

};

#endif // DigitalReader_H
