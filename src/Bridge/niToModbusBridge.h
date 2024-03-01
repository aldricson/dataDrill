#ifndef NITOMODBUSBRIDGE_H
#define NITOMODBUSBRIDGE_H

#include <memory>
#include <functional>

#include "../channelReaders/analogicReader.h"
#include "../channelReaders/digitalReader.h"
#include "../channelWriters/digitalWriter.h"
#include "../Modbus/NewModbusServer.h"
#include "../globals/globalEnumStructs.h"
#include "../timers/simpleTimer.h"
#include "../circularBuffer/ThreadSafeCircularBuffer.h"
#include "../stringUtils/stringUtils.h"
#include "../filesUtils/appendToFileHelper.h"
#include <algorithm> 



class NItoModbusBridge {
public:
    // Constructor
    NItoModbusBridge  (std::shared_ptr<AnalogicReader>  analogicReader,
                       std::shared_ptr<DigitalReader>   digitalReader,
                       std::shared_ptr<DigitalWriter>   digitalWriter,
                       std::shared_ptr<NewModbusServer> modbusServer);

    // Getters and setters for AnalogicReader
    std::shared_ptr<AnalogicReader> getAnalogicReader() const;
    void setAnalogicReader(std::shared_ptr<AnalogicReader> analogicReader);

    // Getters and setters for DigitalReader
    std::shared_ptr<DigitalReader> getDigitalReader() const;
    void setDigitalReader(std::shared_ptr<DigitalReader> digitalReader);



    ThreadSafeCircularBuffer<std::vector<uint16_t>>& getSimulationBuffer();
    std::shared_ptr<SimpleTimer>       getSimulateTimer()  const;
    std::shared_ptr<SimpleTimer>       getDataAcquTimer()  const;
    std::shared_ptr<NewModbusServer>   getModbusServer()   const;
    const std::vector<MappingConfig>&  getMappingData()    const;

    // Load mapping from a configuration file
    void loadMapping();
    void loadAlarmMapping();

    bool startModbusSimulation();
    void stopModbusSimulation();
    bool startAcquisition();
    void stopAcquisition();

    void acquireCounters();
    void setRelays(uint16_t coilAddr, bool state);


protected:
    unsigned long long m_simulationCounter=0;
    GlobalFileNamesContainer                             m_fileNamesContainer;
    ThreadSafeCircularBuffer<std::vector<uint16_t>>      m_simulationBuffer  ;
    ThreadSafeCircularBuffer<std::vector<uint16_t>>      m_realDataBuffer    ;         
    std::shared_ptr<SimpleTimer>                         m_simulateTimer     ;
    std::shared_ptr<SimpleTimer>                         m_dataAcquTimer     ;
    std::shared_ptr<AnalogicReader>                      m_analogicReader    ;
    std::shared_ptr<DigitalReader>                       m_digitalReader     ;  
    std::shared_ptr<DigitalWriter>                       m_digitalWriter     ;
    std::shared_ptr<NewModbusServer>                     m_modbusServer      ;
    std::vector<MappingConfig>                           m_mappingData       ;
    std::vector<AlarmsMappingConfig>                     m_alarmsMappingData ;

    std::vector<uint16_t>                                m_realDataBufferLine; // a Real Buffer Data

    void acquireData();

    uint16_t linearInterpolation16Bits(double value, double minSource, double maxSource, uint16_t minDestination, uint16_t maxDestination);
    void onSimulationTimerTimeOut ();
    void simulateAnalogicInputs   (std::vector<uint16_t> &analogChannelsResult);
    void simulateCounters         (std::vector<uint16_t> &analogChannelsResult);
    void simulateCoders           (std::vector<uint16_t> &analogChannelsResult);
    void simulateRelays           ();

    // After updating all relay states, you may want to trigger updates or notifications
    // to reflect these changes in the simulation environment or UI if applicable.


    
    void onDataAcquisitionTimerTimeOut();
    

    // Signal functions to notify changes
    std::function<void()> onAnalogicReaderChanged;
    std::function<void()> onDigitalReaderChanged;
    std::function<void()> onDigitalWriterChanged;
    std::function<void()> newSimulationBufferReadySignal;

    uint32_t m_simulatedCounterValue     = 0;
    uint32_t m_simulatedCodersValue      = 0;
    uint8_t m_simulatedAlarmStepCounter  = 0;
};

#endif // NITOMODBUSBRIDGE_H
