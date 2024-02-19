#include "digitalReader.h"

// Constructor
DigitalReader::DigitalReader(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
                             std::shared_ptr<QNiDaqWrapper> aDaqMxInstance)
    : BaseReader(aSysConfigInstance, aDaqMxInstance)
{
    // Add any initialization specific to DigitalReader here
}


void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const unsigned int &index, double &returnedValue)
{
    // Implement digital one-shot read logic and set the returnedValue here
}

void DigitalReader::manualReadOneShot(const std::string &moduleAlias, const std::string &chanName, double &returnedValue)
{
    //TODO
}
