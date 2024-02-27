#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <string.h>
#include <thread>
#include <memory> // for std::unique_ptr
#include "./NiWrappers/QNiSysConfigWrapper.h"
#include "./NiWrappers/QNiDaqWrapper.h"
#include "./channelReaders/analogicReader.h"
#include "./channelReaders/digitalReader.h"
#include "./Modbus/NewModbusServer.h"
#include "./Bridge/niToModbusBridge.h"
#include "./Signals/QSignalTest.h"
#include "./stringUtils/stringUtils.h"
#include "./TCP Command server/CrioSSLServer.h"
#include "testFunctions.h"


// These wrappers utilize low-level APIs that have hardware access. 
// Proper destruction is essential to restore certain hardware states when they go out of scope.
// Using smart pointers like std::shared_ptr ensures safer and automatic resource management.
std::shared_ptr<QNiSysConfigWrapper> sysConfig             ;
std::shared_ptr<QNiDaqWrapper      > daqMx                 ;
std::shared_ptr<AnalogicReader     > analogReader          ;
std::shared_ptr<DigitalReader      > digitalReader         ;
std::shared_ptr<DigitalWriter      > m_digitalWriter       ;
std::shared_ptr<NewModbusServer    > modbusServer          ;
std::shared_ptr<NItoModbusBridge   >  m_crioToModbusBridge ;


//std::shared_ptr<CrioTCPServer>       m_crioTCPServer;
std::shared_ptr<CrioSSLServer>         m_crioTCPServer;

void createNecessaryInstances()
{
  //std::string str; 
  //c++ wrapper around NiDaqMx low level C API (used mainly to read or write on devices channels) 
  daqMx          = std::make_shared<QNiDaqWrapper>();
  std::cout<<"daqMx Wrapper created"<<std::endl;
  //c++ wrapper around NISysConfig low level C API (used to get or set parameters of devices)
  sysConfig      = std::make_shared<QNiSysConfigWrapper>();
  std::cout<<"sysconfig Wrapper created"<<std::endl;
  //object to read anlogic channels (both current and voltage)
  analogReader   = std::make_shared<AnalogicReader>     (sysConfig,daqMx);
  std::cout<<"analogic reader created"<<std::endl;
  //object to read mainly coders and 32 bit counters
  digitalReader   = std::make_shared<DigitalReader>      (sysConfig,daqMx);
    std::cout<<"digital reader created"<<std::endl;
  //object to write to coils (relays and alarms typically)
  m_digitalWriter = std::make_shared<DigitalWriter>      (sysConfig,daqMx);
  std::cout<<"digital writer created"<<std::endl;
  //Object that handle the modbus server
  modbusServer = std::make_shared<NewModbusServer>();
  modbusServer->modbusSetSlaveId(1);
  // Run the server in a separate thread
  std::thread modbusServerThread(&NewModbusServer::runServer, modbusServer);
  modbusServerThread.detach(); // Detach the thread to allow it to run independently
  std::cout << "Modbus server created" << std::endl;
  //Object in charge of routing crio datas to modbus
  m_crioToModbusBridge = std::make_shared<NItoModbusBridge>(analogReader,digitalReader,m_digitalWriter,modbusServer);
  std::cout<<"modbus bridge created"<<std::endl;
  //object in charge of all non ssh commands

  //m_crioTCPServer = std::make_shared<CrioTCPServer>(8222,sysConfig,daqMx,analogReader,digitalReader, m_crioToModbusBridge);
  m_crioTCPServer = std::make_shared<CrioSSLServer>(8222,sysConfig,daqMx,analogReader,digitalReader,m_digitalWriter, m_crioToModbusBridge);
  
  std::cout<<"TCP server created"<<std::endl;

}

int main(void)
{  
  

 
  //bool ok;
  //double value =  testReadCurrentFromMod1AI0(ok);
  //if (!ok) return EXIT_FAILURE;
  //std::cout << "Read current: " << value << " Amps" << std::endl;
  //testSignalSlotMechanism(ok);
  //if (!ok) return EXIT_FAILURE;
  //testIniFileSystem(ok);
  //if (!ok) return EXIT_FAILURE;

  createNecessaryInstances();

  m_crioToModbusBridge->loadMapping();
  
  //auto closeLambda = []() { std::exit(EXIT_SUCCESS); };
  //-----------------------------------------------------------
  //get the number of modules for security testing
  daqMx->GetNumberOfModules();
  int32 numberOfModules = daqMx->GetNumberOfModules();
  if (numberOfModules >= 0) 
    {
        printf("Number of modules: %d\n", numberOfModules);
    } 
    else 
    {
        printf("An error occurred.\n");
    }
   //here no error let's continue
   std::cout <<  std::endl;
   std::cout << "*** Init phase 2: retrieve modules and load defaults ***" << std::endl<< std::endl;
   //show a list of all modules REALLY PHYSICALLY present on the crio
   std::vector<std::string> modules = sysConfig->EnumerateCRIOPluggedModules();
   //Show internal of each module
   for (const std::string& str : modules)
   {
     std::cout << "╔═══════════════════════════════════════╗ "<< std::endl;
     std::cout << "║ "<< str    << std::endl;
     std::cout << "╚═══════════════════════════════════════╝"<< std::endl;
   }
   std::cout <<  std::endl;
   
   std::cout << "*** Init phase 3: testing counters ***" << std::endl<< std::endl;
  
    unsigned int counterResult = daqMx->testReadCounter();
    std::string str = std::to_string(counterResult);
    std::cout << "╔/¨¨══════════════════════════════════════╗ "<< std::endl;
    std::cout << "║ "<< str    << std::endl;
    std::cout << "╚\\_═══════════════════════════════════════╝"<< std::endl;


    //boot strap finished
    m_crioTCPServer->startServer();
    std::cout <<  std::endl;
    std::cout << "*** Init phase 4: command server started ***" << std::endl<< std::endl; 
    clearConsole();
    showBanner();
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}