#include "NewModbusServer.h"
#include <iostream>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


NewModbusServer::NewModbusServer() : ctx(nullptr), mb_mapping(nullptr), server_socket(-1), fdmax(0) 
{
    FD_ZERO(&refset);
    m_ini = std::make_shared<IniObject>();
    loadConfig();
    initializeModbusContext();
    setupServerSocket();
}

NewModbusServer::~NewModbusServer() {
    if (server_socket != -1) {
        close(server_socket);
    }
    modbus_free(ctx);
    modbus_mapping_free(mb_mapping);
}

void NewModbusServer::loadConfig()
{
    bool ok; //test variable to if if all is ok or not:
    SRUMapping.m_modeSRU         = m_ini->readBoolean("exlog"                             , 
                                                      "compatibilitylayer"                , 
                                                      SRUMapping.m_modeSRU                ,
                                                      m_filenNamesContainer.modbusIniFile , 
                                                      ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_filenNamesContainer.newModbusServerLogFile,"in void NewModbusServer::loadConfig() reading 'exlog' 'compatibilitylayer' failed.");
    }
    SRUMapping.m_nbSRUAnalogsIn  = m_ini->readInteger("exlogmapping"                       , 
                                                      "nbanalogsin"                        , 
                                                      SRUMapping.m_nbSRUAnalogsIn          , 
                                                      m_filenNamesContainer.modbusIniFile  , 
                                                      ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_filenNamesContainer.newModbusServerLogFile,"in void NewModbusServer::loadConfig() reading 'exlogmapping' 'nbanalogsin' failed.");

    }
    SRUMapping.m_nbSRUAnalogsOut = m_ini->readInteger("exlogmapping"                       , 
                                                      "nbanalogsout"                       , 
                                                      SRUMapping.m_nbSRUAnalogsOut         , 
                                                      m_filenNamesContainer.modbusIniFile  , 
                                                      ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_filenNamesContainer.newModbusServerLogFile,"in void NewModbusServer::loadConfig() reading 'exlogmapping' 'nbanalogsout' failed.");

    }
    SRUMapping.m_nbSRUCounters   = m_ini->readInteger("exlogmapping"  , 
                                                      "nbcounters"                         , 
                                                      SRUMapping.m_nbSRUCounters           , 
                                                      m_filenNamesContainer.modbusIniFile  , 
                                                      ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_filenNamesContainer.newModbusServerLogFile,"in void NewModbusServer::loadConfig() reading 'exlogmapping' 'nbcounters' failed.");

    }
    //TODO CODERS

    SRUMapping.m_nbSRUAlarms     = m_ini->readInteger("exlogmapping"                      , 
                                                     "nbalarms"                           , 
                                                     SRUMapping.m_nbSRUAlarms             , 
                                                     m_filenNamesContainer.modbusIniFile  , 
                                                     ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_filenNamesContainer.newModbusServerLogFile,"in void NewModbusServer::loadConfig() reading 'exlogmapping' 'nbalarms' failed.");
    }
}

void NewModbusServer::initializeModbusContext() {
    ctx = modbus_new_tcp("0.0.0.0", 502);
//nb_bits: The maximum number of coils.
//nb_input_bits: The maximum number of discrete inputs.
//nb_registers: The maximum number of holding registers.
//nb_input_registers: The maximum number of input registers.

    mb_mapping = modbus_mapping_new(20,20,512,512);
    if (mb_mapping == NULL) {
        std::cerr << "Failed to allocate the mapping: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        exit(EXIT_FAILURE);
    }

    // Additional check for tab_input_registers
    if (!mb_mapping->tab_input_registers) {
        std::cerr << "Failed to allocate tab_input_registers." << std::endl;
        modbus_free(ctx);
        exit(EXIT_FAILURE);
    }
}


void NewModbusServer::setupServerSocket() {
    server_socket = modbus_tcp_listen(ctx, NB_CONNECTION);
    if (server_socket == -1) {
        std::cerr << "Unable to listen TCP connection\n";
        modbus_free(ctx);
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, NewModbusServer::closeServer);
    FD_SET(server_socket, &refset);
    fdmax = server_socket;
}

void NewModbusServer::runServer() {
    while (true) {
        fd_set rdset = refset;
        if (select(fdmax + 1, &rdset, NULL, NULL, NULL) == -1) {
            perror("Server select() failure.");
            closeServer(EXIT_FAILURE);
        }

        for (int master_socket = 0; master_socket <= fdmax; master_socket++) {
            if (!FD_ISSET(master_socket, &rdset)) {
                continue;
            }

            if (master_socket == server_socket) {
                handleNewConnection();
            } else {
                handleClientRequest(master_socket);
            }
        }
    }
}

void NewModbusServer::handleNewConnection() {
    // Declare variables for socket address and new file descriptor
    socklen_t addrlen;
    struct sockaddr_in clientaddr;
    int newfd;
    // Set the size of the client address
    addrlen = sizeof(clientaddr);
    // Initialize the client address structure to zero
    memset(&clientaddr, 0, sizeof(clientaddr));
    // Accept a new connection on the server socket
    newfd = accept(server_socket, (struct sockaddr *) &clientaddr, &addrlen);
    // Check if the new connection is successfully accepted
    if (newfd == -1) {
        // If there is an error, print the error message
        perror("Server accept() error");
    } else {
        // If the connection is successful, add the new socket to the reference set
        FD_SET(newfd, &refset);
        // Update the maximum file descriptor number if the new socket is greater
        if (newfd > fdmax) {
            fdmax = newfd;
        }
        // Add the new client to the client list and broadcast the update
        std::string ipAddress = inet_ntoa(clientaddr.sin_addr);
        updateClientList(newfd, ipAddress,false);
        broadcastClientList();
    }
}

void NewModbusServer::updateClientList(int socket, const std::string& ipAddress, bool remove) {
    std::lock_guard<std::mutex> lock(clientListMutex);
    if (remove) {
        clientList.erase(socket);
    } else {
        clientList[socket] = ipAddress;
    }
}

void NewModbusServer::broadcastClientList() 
{
    std::string message = "cli:Connected Clients: " + std::to_string(clientList.size());
    for (const auto& clientPair : clientList) {
        int clientSocket = clientPair.first;        // Extract the socket
        std::string clientIp = clientPair.second;   // Extract the IP address
        message += "\nSocket " + std::to_string(clientSocket) + ": " + clientIp;
    }
}


void NewModbusServer::handleClientRequest(int master_socket) {
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    modbus_set_socket(ctx, master_socket);
    int rc = modbus_receive(ctx, query);

    if (rc > 0) 
    {
        // Process the request
        modbus_reply(ctx, query, rc, mb_mapping);
    } 
    else if (rc == -1) 
    {
        // Connection closed by client
        std::cout << "Connection closed on socket " << master_socket << std::endl;
        // Update the client list to reflect the disconnection
        updateClientList(master_socket, "", true);  // 'true' indicates removal
        broadcastClientList();

        // Close the socket and remove it from the set
        close(master_socket);
        FD_CLR(master_socket, &refset);

        // Update fdmax if necessary
        if (master_socket == fdmax) {
            // Decrease fdmax to the highest active socket
            fdmax = findMaxSocket();
        }
    }
}

int NewModbusServer::findMaxSocket() {
    int maxSock = server_socket;
    for (const auto& client : clientList) {
        if (client.first > maxSock) {
            maxSock = client.first;
        }
    }
    return maxSock;
}

void NewModbusServer::closeServer(int signal) {
    std::cout << "Closing the server due to signal " << signal << std::endl;
    exit(signal);
}

/*void NewModbusServer::reMapInputRegisterValuesForAnalogics(const std::vector<uint16_t>& newValues) 
{
    std::lock_guard<std::mutex> lock(mb_mapping_mutex);
    CrioDebugServer::broadcastMessage("void NewModbusServer::reMapInputRegisterValuesForAnalogics(const std::vector<uint16_t>& newValues)\n"
                                      "mutex locked, enter loop:");
    for (size_t i = 0; i < newValues.size() && i < MODBUS_MAX_READ_REGISTERS; ++i) 
    {
        CrioDebugServer::broadcastMessage("index: "+std::to_string(i)+" value: "+std::to_string(newValues[i]));
        mb_mapping->tab_input_registers[i] = newValues[i];
    }
}*/

void NewModbusServer::reMapInputRegisterValuesForAnalogics(const std::vector<uint16_t>& newValues) {
    std::lock_guard<std::mutex> lock(mb_mapping_mutex);

    // Check if mb_mapping is valid
    if (!mb_mapping) 
    {
        return;
    }

    // Check if tab_input_registers is valid
    if (!mb_mapping->tab_input_registers) 
    {
        return;
    }

    // Ensure we do not write beyond the allocated array
    size_t numRegistersToWrite = std::min(newValues.size(), static_cast<size_t>(MODBUS_MAX_READ_REGISTERS));

    for (size_t i = 0; i < numRegistersToWrite; ++i) {
        mb_mapping->tab_input_registers[i] = newValues[i];
    }
}


void NewModbusServer::reMapCoilsValues(const std::vector<bool>& newValues) {
    std::lock_guard<std::mutex> lock(mb_mapping_mutex);
    for (size_t i = 0; i < newValues.size() && i < MODBUS_MAX_READ_BITS; ++i) {
        mb_mapping->tab_bits[i] = newValues[i];
    }
}


bool NewModbusServer::modbusSetSlaveId(int newSlaveId) {
    std::lock_guard<std::mutex> lock(ctxMutex); // Lock the mutex to ensure thread safety

    if (newSlaveId < 0 || newSlaveId > 255) {
        std::cerr << "Invalid slave ID. Must be between 0 and 255." << std::endl;
        return false;
    }

    // Set the new slave ID
    if (modbus_set_slave(ctx, newSlaveId) == -1) 
    {   
        std::string error = modbus_strerror(errno);
        return false;
    }
    return true;
}

SensorRigUpStruct NewModbusServer::getSRUMapping() const {
    std::lock_guard<std::mutex> lock(sruMappingMutex);
    return SRUMapping;
}


void NewModbusServer::setSRUMapping(const SensorRigUpStruct &newMapping)
{
    std::lock_guard<std::mutex> lock(sruMappingMutex);
    SRUMapping = newMapping;
}

int NewModbusServer::getSRUMappingSizeWithoutAlarms()
{
    int totalSize =  SRUMapping.m_nbSRUAnalogsIn    +
                     SRUMapping.m_nbSRUAnalogsOut   +
                    (SRUMapping.m_nbSRUCoders   *2) + //2 register of 16 bits by coder
                    (SRUMapping.m_nbSRUCounters *3);  //3 register of 16 bits by counter (1x for frequency 2x value)  
    return totalSize;
}
