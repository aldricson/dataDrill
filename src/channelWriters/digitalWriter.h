#ifndef digitalWriter_H
#define digitalWriter_H

#include "baseWriter.h"

class DigitalWriter : public BaseWriter {
public:
    // Constructor
    DigitalWriter(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
                  std::shared_ptr<QNiDaqWrapper> aDaqMxInstance);
    
    // Override the pure virtual functions
    void manualSetOutput (const std::string &moduleAlias, const unsigned int &index,const bool &state) override;
    void manualSetOutput (const std::string &moduleAlias, const std::string  &chanName,const bool &state) override;
    
    // Add any additional member functions specific to digitalWriter here
};

#endif // digitalWriter_H
