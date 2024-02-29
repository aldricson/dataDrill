#ifndef digitalWriter_H
#define digitalWriter_H

#include "baseWriter.h"
#include "../globals/globalEnumStructs.h"
#include "../filesUtils/appendToFileHelper.h"

class DigitalWriter : public BaseWriter {
public:
    // Constructor
    DigitalWriter(std::shared_ptr<QNiSysConfigWrapper> aSysConfigInstance,
                  std::shared_ptr<QNiDaqWrapper> aDaqMxInstance);
    
    // Override the pure virtual functions
    void manualSetOutput (const std::string &moduleAlias, const unsigned int &index,const bool &state) override;
    void manualSetOutput (const std::string &moduleAlias, const std::string  &chanName,const bool &state) override;
    
protected:
    GlobalFileNamesContainer fileNamesContainer;
};

#endif // digitalWriter_H
