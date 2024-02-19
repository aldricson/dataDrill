#ifndef TESTFUNCTIONS_H
#define TESTFUNCTIONS_H

#include "config.h"
#include "./stringUtils/stringUtils.h"
#include "./filesUtils/ini.h"
#ifdef CrossCompiled
  #include <NIDAQmx.h>
#else
  #include "../../DAQMX_INCLUDE/NIDAQmx.h"
#endif
#include "./NiModulesDefinitions/NI9208.h"
#include "./Signals/QSignalTest.h"




static inline double testReadCurrentFromMod1AI0(bool &ok) {
    ok = false;
    std::cout<<"Test 2 : Hardcoded reading of Mod1/ai0"<<std::endl;
    TaskHandle taskHandle = 0;
    int32 error = 0;
    float64 readValue = 0.0;

    // Create a new task
    error = DAQmxCreateTask("readCurrentTask", &taskHandle);
    if (error) {
        ok = false;
        DAQmxClearTask(taskHandle);
        std::cout<<"Test failed!"<<std::endl;
        throw std::runtime_error("Failed to create task for reading current.");
    }

    // Create an analog input current channel
    error = DAQmxCreateAICurrentChan(taskHandle,
                                     "Mod1/ai0", // Physical channel name
                                     "",        // Name to assign to channel (empty to use physical name)
                                     DAQmx_Val_RSE,     // Terminal configuration
                                     -0.02,             // Min value in Amps (-20 mA)
                                     0.02,              // Max value in Amps (20 mA)
                                     DAQmx_Val_Amps,    // Units in Amps
                                     DAQmx_Val_Internal,// Shunt Resistor Location
                                     30.01,             // External Shunt Resistor Value in Ohms
                                     NULL);             // Custom Scale Name
        if (error) 
               {
                  ok = false;
                  //error handling  
                  char errBuff[2048] = {'\0'};
                  DAQmxGetExtendedErrorInfo(errBuff, 2048);
                  std::cerr << "Channel Creation Failed: " << errBuff << std::endl;
                  DAQmxClearTask(taskHandle);
                  std::cout<<"Test failed!"<<std::endl;
                  throw std::runtime_error("Failed to create channel.");
                } 

    // Read the current value
    error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
    if (error) {
        ok = false;
        DAQmxClearTask(taskHandle);
        std::cout<<"Test failed!"<<std::endl;
        throw std::runtime_error("Failed to read current value.");
    }

    // Stop the task and clear it
    DAQmxStopTask(taskHandle);
    DAQmxClearTask(taskHandle);
    if (!error)
    {
        ok = true; 
        std::cout<<"Test succes!"<<std::endl;
    }
    return static_cast<double>(readValue);
}

static inline void testSignalSlotMechanism(bool &ok)
{
  ok = false;
    std::cout << "Test 3: Pure C++ signal/slot mechanism" << std::endl;
    NI9208 testModule;
    testModule.setAlias("TEST");
    QSignalTest slotTestObject;
    // sender (object that sends the signal) || receiver (the object with the slot)
    testModule.moduleSlotNumberChangedSignal = std::bind(&QSignalTest::onIntValueChanged,
        &slotTestObject, // Pass a pointer to slotTestObject
        std::placeholders::_1,
        std::placeholders::_2);
    
    // Test the signal-slot mechanism
    testModule.setSlotNb(133); // Whatever value you want to set
    std::cout << "Returned value: " << std::to_string(slotTestObject.getReturned()) << std::endl;
    if (slotTestObject.getReturned() == 133)
    {
       ok = true;
       std::cout<<"Test succes!"<<std::endl;
    }
    else
    {
       ok = false; 
       std::cout<<"Test Failed!"<<std::endl;
    }
}

static inline void testIniFileSystem(bool &ok)
{
    ok = false;
    std::cout << "Test 4: config system" << std::endl;
    //create file object
    mINI::INIFile file("testIni.ini");
    std::cout << "ini file set to testIni.ini: OK" << std::endl;
    //create a structure for handling datas
    mINI::INIStructure ini;
    std::cout << "data structure created: OK" << std::endl;
    ini["TestSection"]["TestKey"] = "testValue";
    std::cout << "field set to testValue" << std::endl;
    file.generate(ini);
    std::cout << "file generated" << std::endl;

    file.read(ini);
    std::string value = ini.get("TestSection").get("TestKey");
    if(value=="testValue")
     {
       ok = true;
       std::cout<<"Test succes!"<<std::endl;
    }
    else
    {
       ok = false; 
       std::cout<<"Test Failed!"<<std::endl;
    }
}



#endif