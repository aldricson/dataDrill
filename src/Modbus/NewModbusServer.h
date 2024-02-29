#ifndef NEWMODBUSSERVER_H
#define NEWMODBUSSERVER_H

#include <map>
#include <modbus.h>
#include <mutex>
#include <vector>
#include <string>
#include "../filesUtils/iniObject.h"
#include "../filesUtils/cPosixFileHelper.h"
#include "../filesUtils/appendToFileHelper.h"
#include "../globals/globalEnumStructs.h"

class NItoModbusBridge;

struct SensorRigUpStruct {
    bool m_modeSRU         = true;
    int  m_nbSRUAnalogsIn  = 64;
    int  m_nbSRUAnalogsOut = 0;
    int  m_nbSRUCounters   = 8;
    int  m_nbSRUCoders     = 0;
    int  m_nbSRUAlarms     = 4;
};


class NewModbusServer {
public:
    NewModbusServer();
    ~NewModbusServer();

    void runServer();
    bool modbusSetSlaveId                    (int newSlaveId);
    void reMapInputRegisterValuesForAnalogics(const std::vector<uint16_t>& newValues);
    void reMapCoilsValues                    (const std::vector<bool>& newValues);
    
    SensorRigUpStruct getSRUMapping() const;  
    void setSRUMapping(const SensorRigUpStruct& newMapping);

    int getSRUMappingSizeWithoutAlarms();

    std::shared_ptr<NItoModbusBridge> getModbusBridge() const;
    void setModbusBridge(const std::shared_ptr<NItoModbusBridge>& modbusBridge);


protected:
    static const int NB_CONNECTION = 25 ;
    std::mutex mb_mapping_mutex         ; // Mutex for thread-safe access to mb_mapping
    std::mutex ctxMutex                 ; // Mutex for thread-safe access to modbus context
    mutable std::mutex sruMappingMutex  ; // Mutex for thread-safe access to sru (client) Mapping
                                          //the mutext must be mutable for the const getter

    std::mutex clientListMutex;            // Mutex for thread-safe access to client list
    std::map<int, std::string> clientList; // Map of client socket to IP address 

    
    modbus_t          *ctx                           ; //modbus context
    modbus_mapping_t  *mb_mapping                    ; //internal modbus mapping
    std::shared_ptr<NItoModbusBridge> m_modbusBridge ; //alarms needs direct access to the bridge
    int               server_socket                  ; //socket id
    fd_set            refset                         ; //for select and pselect
    int               fdmax                          ;
    
    SensorRigUpStruct SRUMapping    ;  //this define the client configuration
    std::shared_ptr<IniObject> m_ini;  //helper object to read/write inifiles
    GlobalFileNamesContainer fileNamesContainer;
    void loadConfig();

    void        initializeModbusContext      ();
    void        setupServerSocket            ();
    void        handleNewConnection          ();
    void        handleClientRequest          (int master_socket);
    void        handleWriteSingleCoilRequest (uint16_t coilAddr, bool state);
    int         mapCoilAddressToChannel      (uint16_t coilAddr);  
    int         findMaxSocket                (); 
    void        updateClientList             (int socket, const std::string& ipAddress, bool remove);
    void        broadcastClientList          (); 
    static void closeServer                  (int signal);
    

    // Disallowing copying and assignment
    NewModbusServer(const NewModbusServer&)            = delete;
    NewModbusServer& operator=(const NewModbusServer&) = delete;

};

#endif // NEWMODBUSSERVER_H
