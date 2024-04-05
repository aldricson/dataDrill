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
#include "../Filters/LowPassFilter.h"
#include "../Conversions/convUtils.h"
#include "../globals/globalEnumStructs.h"
#include "../filesUtils/appendToFileHelper.h"
#include "../stringUtils/stringUtils.h"
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
    void         initMod3(const std::string& deviceName,
                          double minRange,
                          double maxRange,
                          int32 samplesPerChannel,
                          double samplingRate,
                          int32 channelsCount);
    std::vector<double> readMod3Samples(int32 channelsCount,
                                        int32 samplesPerChannel,
                                        float64 timeOut,
                                        bool &inError); 
    void applyMod3LowPassFilter(const int32 channelsCount,
                                const int32 samplesPerChannel,
                                std::vector<double> &dataBuffer,
                                std::vector<double> &averages,
                                float deltaTime);
    void                readMod3();
    void                readMod4();

    double       readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries);
    double       readVoltage(NIDeviceModule *deviceModule, std::string  chanName , unsigned int maxRetries);

    unsigned int readCounter     (NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries);
    unsigned int readCounter     (NIDeviceModule *deviceModule, std::string  chanName );
    unsigned int testReadCounter1 ();
    unsigned int testReadCounter2 ();
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
    bool         getWindowFilterActiv             () const;
    void         setLWindowFilterActiv            (bool isActiv);
    bool         getLowPassFilterActiv            () const;
    void         setLowPassFilterActiv            (bool isActiv);
    bool         getNotchFilterActiv              () const;
    void         setNotchFilterActiv              (bool isActiv);
    float        getLowPassFilterCutoffFrequency  () const;
    void         setLowPassFilterCutoffFrequency  (float cutOffFrequency);   

    //Call backs falling functions
    void handleReadCurrentCompletion(int32 status);
    void handleReadVoltageCompletion(int32 status);
    void handleReadCounterCompletion(int32 status);
    unsigned char random_char();
    std::string generate_hex(const unsigned int len);

    std::atomic<bool> keepCurrentRunning{true}; // Control flag for the reading loop
    ThreadSafeVector<double> Mod1Buffer;
    std::vector<double> Mod1OldValuesBuffer;
    ThreadSafeVector<double> Mod2Buffer;
    std::vector<double> Mod2OldValuesBuffer;
    ThreadSafeVector<double> Mod3Buffer;
    std::vector<double> Mod3OldValuesBuffer;
    ThreadSafeVector<uInt32> Mod4Buffer;

protected:

  std::vector<double> lowPassFilterDatas(const std::vector<double>& dataBuffer, float deltaTime, float cutOffFrequency);
  void averageWindow(std::vector<double>& averages, const std::vector<double>& oldValues);
  

  bool  m_lowPassFilterActiv       = false;
  bool  m_notchFilterActiv         = false;
  bool  m_rollingWindowFilterActiv = false;
  float m_cutOffFrequency          = 10.0f;

    
private:
    std::mutex voltageMutex;
    std::mutex currentMutex;
    std::mutex countersMutex;
    std::mutex alarmsMutex;
    GlobalFileNamesContainer fileNamesContainer;
    TaskHandle counterHandle1 = nullptr; // for testing purpose
    TaskHandle counterHandle2 = nullptr;

    
    TaskHandle readCurrentMod1Task = nullptr;
    TaskHandle readCurrentMod2Task = nullptr;
    TaskHandle readVoltageMod3Task = nullptr;
    TaskHandle readCounterMod4Task = nullptr;
    
    
    
    
    std::atomic<double> m_lastSingleCurrentChannelValue;
    std::atomic<double> m_lastSingleVoltageChannelValue;
    unsigned int m_lastSingleCounter             = 0;
    std::map<std::string, TaskHandle> counterTasksMap;
    std::map<std::string, TaskHandle> currentTaskMap;


    LowPassFilter lpf; 
};


#endif // QNIDAQWRAPPER_H
