#ifndef QSIGNALTEST_H
#define QSIGNALTEST_H

#include <iostream>
#include <functional> 
#include "../NiModulesDefinitions/NIDeviceModule.h"

class QSignalTest {
public:
    std::function<void()> successSignal;
    unsigned int getReturned() {return m_returned;} ;

    void onIntValueChanged(unsigned int newValue,NIDeviceModule *sender) {
        m_returned = newValue;
        //std::cout << "Signal received! Value changed to " << newValue << std::endl;
        //std::cout << "Sender alias is  " << sender->getAlias().c_str() << std::endl;
    }
private:
   unsigned int m_returned = 0;
};

#endif // QSIGNALTEST_H
