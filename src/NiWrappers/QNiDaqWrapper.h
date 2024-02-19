#ifndef QNIDAQWRAPPER_H
#define QNIDAQWRAPPER_H

#include <vector>
#include <string>

#include <iostream> 
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>
#include "../Conversions/convUtils.h"

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

    double       readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries);
    double       readVoltage(NIDeviceModule *deviceModule, std::string  chanName , unsigned int maxRetries);

    unsigned int readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries);
    void handleErrorAndCleanTask();

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
    
private:
    std::mutex voltageMutex;
    std::mutex currentMutex;
    TaskHandle taskHandle;
    std::atomic<double> m_lastSingleCurrentChannelValue;
    std::atomic<double> m_lastSingleVoltageChannelValue;
    unsigned int m_lastSingleCounter             = 0;  
};


#endif // QNIDAQWRAPPER_H
