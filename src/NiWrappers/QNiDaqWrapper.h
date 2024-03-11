#ifndef QNIDAQWRAPPER_H
#define QNIDAQWRAPPER_H

#include <vector>
#include <string>
#include <map>
#include <iostream> 
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>
#include "../Conversions/convUtils.h"
#include "../globals/globalEnumStructs.h"
#include "../filesUtils/appendToFileHelper.h"
#include "../threadSafeBuffers/threadSafeVector.h"

#include "../config.h"
#ifdef CrossCompiled
  #include <NIDAQmx.h>
#else
  #include "../../DAQMX_INCLUDE/NIDAQmx.h"
#endif

class NIDeviceModule;

class QNiDaqWrapper {
public:
    QNiDaqWrapper();
    ~QNiDaqWrapper();

    int32 GetNumberOfModules();
    std::vector<std::string> GetDevicesList();
    double       readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps);
    double       readCurrent(NIDeviceModule *deviceModule, std::string  chanName, unsigned int maxRetries, bool autoConvertTomAmps);
    
    void         readMod1();
    void         readMod2();
    void         readMod3();
    void         readMod4();

    double       readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries);
    double       readVoltage(NIDeviceModule *deviceModule, std::string  chanName , unsigned int maxRetries);

    unsigned int readCounter     (NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries);
    unsigned int readCounter     (NIDeviceModule *deviceModule, std::string  chanName );
    unsigned int testReadCounter ();
    void         resetCounter    (NIDeviceModule *deviceModule, const unsigned int &index);
    void         resetCounter    (NIDeviceModule *deviceModule, const std::string &chanName);

    void         setRelayState     (NIDeviceModule* deviceModule, unsigned int chanIndex, const bool &state);
    void         setRelayState     (NIDeviceModule* deviceModule, const std::string& chanName, const bool &state);
    void         testSetRelayState (unsigned int relayIndex, const bool &state); 

    void handleErrorAndCleanTask(TaskHandle taskHandle);

    //signals
    std::function<void(double lastValue,QNiDaqWrapper *sender)>    channelCurrentDataChangedSignal = nullptr;  //emited as soon as the data for a channel has changed, 
                                                                                                               //without any garanty that the channel is ready for a new task  
    std::function<void(double lastValue,QNiDaqWrapper *sender)>    channelCurrentDataReadySignal   = nullptr;  //emited when the task is fully done and the channel is ready for a new one
    
    std::function<void(double lastValue, QNiDaqWrapper *sender)>    channelVoltageDataChangedSignal = nullptr;
    std::function<void(double lastValue, QNiDaqWrapper *sender)>   channelVoltageDataReadySignal   = nullptr;
    
    std::function<void(unsigned int lastValue, QNiDaqWrapper *sender)>    channelCounterDataChangedSignal = nullptr;
    std::function<void(unsigned int lastValue, QNiDaqWrapper *sender)>    channelCounterDataReadySignal   = nullptr; 

    //Getters and setters
    double       getLastSingleCurrentChannelValue () const;
    void         setLastSingleCurrentChannelValue (double value);
    double       getLastSingleVoltageChannelValue () const;
    void         setLastSingleVoltageChannelValue (double       value);
    unsigned int getLastSingleCounterValue        () const;
    void         setLastSingleCounterValue        (unsigned int value);
    //Call backs falling functions
    void handleReadCurrentCompletion(int32 status);
    void handleReadVoltageCompletion(int32 status);
    void handleReadCounterCompletion(int32 status);
    unsigned char random_char();
    std::string generate_hex(const unsigned int len);

    std::atomic<bool> keepCurrentRunning{true}; // Control flag for the reading loop
    ThreadSafeVector<double> Mod1Buffer;
    ThreadSafeVector<double> Mod2Buffer;
    ThreadSafeVector<double> Mod3Buffer;
    ThreadSafeVector<uInt32> Mod4Buffer;
    
private:
    std::mutex voltageMutex;
    std::mutex currentMutex;
    std::mutex countersMutex;
    std::mutex alarmsMutex;
    GlobalFileNamesContainer fileNamesContainer;
    TaskHandle counterHandle = nullptr; // for testing purpose

    
    TaskHandle readCurrentMod1Task = nullptr;
    TaskHandle readCurrentMod2Task = nullptr;
    TaskHandle readVoltageMod3Task = nullptr;
    TaskHandle readCounterMod4Task = nullptr;
    
    
    
    
    std::atomic<double> m_lastSingleCurrentChannelValue;
    std::atomic<double> m_lastSingleVoltageChannelValue;
    unsigned int m_lastSingleCounter             = 0;
    std::map<std::string, TaskHandle> counterTasksMap;
    std::map<std::string, TaskHandle> currentTaskMap; 
};


#endif // QNIDAQWRAPPER_H
