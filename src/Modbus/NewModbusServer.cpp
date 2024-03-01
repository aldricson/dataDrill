#include "NewModbusServer.h"
#include <iostream>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cerrno>
#include "../Bridge/niToModbusBridge.h"


NewModbusServer::NewModbusServer()
    : ctx(nullptr), mb_mapping(nullptr), server_socket(-1), fdmax(0)
{
    // Initialize the reference set for socket descriptors
    FD_ZERO(&refset);
    // Create a shared pointer for IniObject
    m_ini = std::make_shared<IniObject>();
    // Load configuration from an INI file
    loadConfig();
    // Initialize the modbus context and handle any errors
    initializeModbusContext();
    // Setup the server socket and handle any errors
    setupServerSocket();
}

NewModbusServer::~NewModbusServer() {
    // Check if the server_socket is valid and close it
    if (server_socket != -1) {
        close(server_socket);
    }

    // Free the modbus context
    if (ctx != nullptr) {
        modbus_free(ctx);
    }

    // Free the modbus mapping
    if (mb_mapping != nullptr) {
        modbus_mapping_free(mb_mapping);
    }
}

void NewModbusServer::loadConfig()
{
    bool ok;
    try 
    {
        // Read and update the 'm_modeSRU' setting from the configuration file
        SRUMapping.m_modeSRU = m_ini->readBoolean("exlog", "compatibilitylayer", SRUMapping.m_modeSRU, fileNamesContainer.modbusIniFile,ok);
        if (!ok)
        {
            appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::loadConfig() reading 'exlog' 'compatibilitylayer' failed");
        }
        // Read and update the 'm_nbSRUAnalogsIn' setting from the configuration file
        SRUMapping.m_nbSRUAnalogsIn = m_ini->readInteger("exlogmapping", "nbanalogsin", SRUMapping.m_nbSRUAnalogsIn, fileNamesContainer.modbusIniFile,ok);
        if (!ok)
        {
            appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::loadConfig() reading 'exlog' 'nbanalogsin' failed");
        }
        // Read and update the 'm_nbSRUAnalogsOut' setting from the configuration file
        SRUMapping.m_nbSRUAnalogsOut = m_ini->readInteger("exlogmapping", "nbanalogsout", SRUMapping.m_nbSRUAnalogsOut, fileNamesContainer.modbusIniFile,ok);
         if (!ok)
        {
            appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::loadConfig() reading 'exlog' 'nbanalogsout' failed");
        }
        // Read and update the 'm_nbSRUCounters' setting from the configuration file
        SRUMapping.m_nbSRUCounters = m_ini->readInteger("exlogmapping", "nbcounters", SRUMapping.m_nbSRUCounters, fileNamesContainer.modbusIniFile,ok);
        if (!ok)
        {
            appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::loadConfig() reading 'exlog' 'nbcounters' failed");
        }
        // Read and update the 'm_nbSRUAlarms' setting from the configuration file
        SRUMapping.m_nbSRUAlarms = m_ini->readInteger("exlogmapping", "nbalarms", SRUMapping.m_nbSRUAlarms, fileNamesContainer.modbusIniFile,ok);
        if (!ok)
        {
            appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::loadConfig() reading 'exlog' 'nbalarms' failed");
        }
    } 
    catch (const std::exception& e) 
    {
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::loadConfig() Error loading configuration");
        // Handle any exceptions that might occur during configuration loading
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
    }
}

void NewModbusServer::initializeModbusContext() {
    // Initialize modbus context with TCP protocol listening on "0.0.0.0" and port 502
    ctx = modbus_new_tcp("0.0.0.0", 502);

    // Check for context initialization errors
    if (ctx == nullptr) 
    {
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"inNewModbusServer::initializeModbusContext() Failed to initialize modbus context: " + std::string(modbus_strerror(errno)));
        std::cerr << "Unable to listen TCP connection: " << modbus_strerror(errno) << std::endl;
        std::cerr << "Failed to initialize modbus context: " << modbus_strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Create an internal modbus mapping
    mb_mapping = modbus_mapping_new(20, 20, 512, 512);

    // Check for mapping allocation errors
    if (mb_mapping == nullptr) 
    {
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"inNewModbusServer::initializeModbusContext() Failed to allocate the mapping: " + std::string(modbus_strerror(errno)));
        std::cerr << "Failed to allocate the mapping: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx); // Free the context before exiting
        exit(EXIT_FAILURE);
    }

    // Additional check for tab_input_registers
    if (!mb_mapping->tab_input_registers) {
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"inNewModbusServer::initializeModbusContext() Failed to allocate tab_input_registers.");
        std::cerr << "Failed to allocate tab_input_registers." << std::endl;
        modbus_mapping_free(mb_mapping); // Free the mapping before exiting
        modbus_free(ctx); // Free the context before exiting
        exit(EXIT_FAILURE);
    }
}

void NewModbusServer::setupServerSocket() {
    // Create a TCP socket and listen for incoming connections
    server_socket = modbus_tcp_listen(ctx, NB_CONNECTION);

    // Check for socket setup errors
    if (server_socket == -1) 
    {
 
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::setupServerSocket() Failed to listen TCP connection: " + std::string(modbus_strerror(errno)));
        std::cerr << "Unable to listen TCP connection: " << modbus_strerror(errno) << std::endl;
        // Free the modbus context before exiting
        modbus_free(ctx);
        exit(EXIT_FAILURE);
    }

    // Register a signal handler for SIGINT (Ctrl+C) to gracefully exit the server
    signal(SIGINT, NewModbusServer::closeServer);

    // Add the server socket to the reference set for select()
    FD_SET(server_socket, &refset);

    // Update fdmax with the server socket descriptor
    fdmax = server_socket;
}

void NewModbusServer::runServer() {
    while (true) {
        // Create a copy of the reference set for select()
        fd_set rdset = refset;

        // Use select() to wait for activity on sockets
        if (select(fdmax + 1, &rdset, NULL, NULL, NULL) == -1) {
            perror("Server select() failure.");
            closeServer(EXIT_FAILURE); // Handle select() failure and exit
        }
         // Iterate through all possible sockets
        for (int master_socket = 0; master_socket <= fdmax; master_socket++) {
            if (!FD_ISSET(master_socket, &rdset)) {
                continue; // Skip sockets with no activity
            }

            if (master_socket == server_socket) {
                // Handle a new connection
                handleNewConnection(); 
            } else {
                // Handle client requests
                handleClientRequest(master_socket);
            }
        }
    }
}

void NewModbusServer::handleWriteSingleCoilRequest(uint16_t coilAddr, bool state) 
{
    if (!m_modbusBridge)
    {
       appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,
                                  "in\n"
                                  "void NewModbusServer::handleWriteSingleCoilRequest(uint16_t coilAddr, bool state)\n"
                                  "Error: m_modbusBridge is nullptr");
 
    }
    // TODO
    m_modbusBridge->setRelays(coilAddr,state);
    // modbus_reply(ctx, query, rc, mb_mapping); // This is a generic placeholder. You'll need to adapt it.
}

void NewModbusServer::acknowledgeSingleCoilWriting(const uint8_t *query, int query_length)
{
    if (!ctx) 
    {          
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,
                                  "in\n"
                                  "void NewModbusServer::acknowledgeSingleCoilWriting(const uint8_t *query, int query_length)\n"
                                  "Error: Modbus context is not initialized.");
        std::cerr << "Modbus context is not initialized." << std::endl;
        return;
    }
    
    int rc = modbus_reply(ctx, query, query_length, mb_mapping);
    if (rc == -1) 
    {
       appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,
                                  "in\n"
                                  "void NewModbusServer::acknowledgeSingleCoilWriting(const uint8_t *query, int query_length)\n"
                                  "Error: Failed to send acknowledgment for Write Single Coil request\n"+
                                  std::string(modbus_strerror(errno))); 
    }
}

void NewModbusServer::handleWriteMultipleCoilRequest(std::vector<uint16_t> coilsAddr, std::vector<bool> states)
{
    if (coilsAddr.size() != states.size()) {
        // Log error: The sizes of coil addresses and states vectors do not match
        return;
    }

    for (size_t i = 0; i < coilsAddr.size(); ++i) {
        handleWriteSingleCoilRequest(coilsAddr[i], states[i]);
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
        // Handle accept() error by printing an error message
        perror("Server accept() error");
    } else {
        // Add the new socket to the reference set for select()
        FD_SET(newfd, &refset);

        // Update the maximum file descriptor number if the new socket is greater
        if (newfd > fdmax) {
            fdmax = newfd;
        }

        // Convert the client's IP address to a string
        std::string ipAddress = inet_ntoa(clientaddr.sin_addr);

        // Add the new client to the client list and broadcast the update
        updateClientList(newfd, ipAddress, false);
        broadcastClientList();
    }
}

void NewModbusServer::updateClientList(int socket, const std::string& ipAddress, bool remove) {
    // Lock the client list mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(clientListMutex);

    // Check if the operation is to remove a client
    if (remove) {
        // Remove the client with the specified socket from the list
        clientList.erase(socket);
    } else {
        // Add or update the client with the specified socket and IP address
        clientList[socket] = ipAddress;
    }
}

void NewModbusServer::broadcastClientList() 
{
    // Create a message to display the connected clients
    std::string message = "cli:Connected Clients: " + std::to_string(clientList.size());

    // Iterate through the client list
    for (const auto& clientPair : clientList) {
        int clientSocket = clientPair.first;        // Extract the socket
        std::string clientIp = clientPair.second;   // Extract the IP address

        // Append client information to the message
        message += "\nSocket " + std::to_string(clientSocket) + ": " + clientIp;
    }

    // At this point, the 'message' variable contains the list of connected clients
    // In the future we may want to perform some action with this message, such as sending it to a specific client or logging it.
}


void NewModbusServer::handleClientRequest(int master_socket) {
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

    // Initially, we do not know if we will receive function code 0x05.
    bool receivedFunctionCode05 = false;

    do {
        // Reset the flag for each iteration of the loop.
        receivedFunctionCode05 = false;

        // Set the socket for the modbus context to ensure replies go to the correct client.
        modbus_set_socket(ctx, master_socket);

        // Attempt to receive a Modbus request from the client.
        int rc = modbus_receive(ctx, query);

        if (rc > 0) 
        {
            // Successfully received a request, now determine the function code.
            uint8_t function_code = query[7]; // Function code is at position 7 in the query array.

            if (function_code == 0x05) {
                // Handle Write Single Coil request.
                receivedFunctionCode05 = true; // Mark that we've received a 0x05 request.

                // Extract the coil address and the desired state from the request.
                uint16_t coilAddr = (query[8] << 8) + query[9]; // Combine bytes 8 and 9 for the coil address.
                bool state = query[10] == 0xFF; // State is determined by byte 10; 0xFF00 means ON, 0x0000 means OFF.

                // Process the Write Single Coil request.
                handleWriteSingleCoilRequest(coilAddr, state);
                if (!SRUMapping.m_modeSRU)
                {
                    // Send an acknowledgment back to the client.
                    acknowledgeSingleCoilWriting(query, rc);
                }
            } 
            else if (function_code == 0x0F) 
            {
                // Handle Write Multiple Coils request.
                // Write Multiple Coils
                uint16_t startingAddr = (query[8] << 8) + query[9];
                uint16_t quantityOfOutputs = (query[10] << 8) + query[11];
                // Extract coil states from the request
                std::vector<uint16_t> coilsAddr;
                std::vector<bool> states;
                for (uint16_t i = 0; i < quantityOfOutputs; i++) 
                {
                    coilsAddr.push_back(startingAddr + i);
                    // Determine the bit position in the request byte array
                    uint8_t byteIndex = 13 + (i / 8); // Starting byte index for coil values is 13
                    uint8_t bitPosition = i % 8;
                    bool state = query[byteIndex] & (1 << bitPosition);
                    states.push_back(state);
                }
                handleWriteMultipleCoilRequest(coilsAddr, states);
            } 
            else 
            {
                // For all other function codes, process the request normally and send a standard Modbus response.
                modbus_reply(ctx, query, rc, mb_mapping);
            }
        } 
        else if (rc == -1) 
        {
            // If an error occurs or if the connection is closed by the client, log the event.
            std::cout << "Error or connection closed on socket " << master_socket << ": " << modbus_strerror(errno) << std::endl;
            // Update the client list and clean up resources as needed.
            // ...
            break; // Exit the loop on error or closed connection.
        }

        // If we received a function code 0x05, immediately check for another request without exiting the loop.
        // This allows handling bursts of 0x05 requests efficiently.
    } while (receivedFunctionCode05);

    // After processing any burst of 0x05 requests, normal operation resumes.
    // Additional cleanup or processing logic can be placed here if needed.
}



/*void NewModbusServer::handleClientRequest(int master_socket) 
{
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

    // Set the socket for modbus context
    modbus_set_socket(ctx, master_socket);

    // Receive a Modbus request
    int rc = modbus_receive(ctx, query);

    if (rc > 0) 
    {
        
        uint8_t function_code = query[7]; // Function code position in the query array
        if (function_code == 0x05) 
        {
            // Extract coil address and state from the query
            uint16_t coilAddr = (query[8] << 8) + query[9]; // Coil address
            bool state = query[10] == 0xFF; // State (0xFF00 for ON, 0x0000 for OFF)
            handleWriteSingleCoilRequest(coilAddr,state);
            acknowledgeSingleCoilWriting(query);
        }
        else if (function_code == 0x0F) 
        {   // Write Multiple Coils
            uint16_t startingAddr = (query[8] << 8) + query[9];
            uint16_t quantityOfOutputs = (query[10] << 8) + query[11];
            // Extract coil states from the request
            std::vector<uint16_t> coilsAddr;
            std::vector<bool> states;
            for (uint16_t i = 0; i < quantityOfOutputs; i++) 
            {
                coilsAddr.push_back(startingAddr + i);
                // Determine the bit position in the request byte array
                uint8_t byteIndex = 13 + (i / 8); // Starting byte index for coil values is 13
                uint8_t bitPosition = i % 8;
                bool state = query[byteIndex] & (1 << bitPosition);
                states.push_back(state);
            }
            handleWriteMultipleCoilRequest(coilsAddr, states);
        }
        else
        {
            // Process the valid Modbus request and send a response
            modbus_reply(ctx, query, rc, mb_mapping);
        }
    } 
    else if (rc == -1) 
    {
        // Connection closed by the client
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
}*/


int NewModbusServer::findMaxSocket() {
    int maxSock = server_socket;

    // Iterate through the client list to find the maximum socket
    for (const auto& client : clientList) {
        if (client.first > maxSock) {
            maxSock = client.first;
        }
    }

    // Return the maximum socket found
    return maxSock;
}

void NewModbusServer::closeServer(int signal) {
    // Display a message indicating the reason for server closure
    std::cout << "Closing the server due to signal " << signal << std::endl;
    
    // Exit the server with the provided signal
    exit(signal);
}

void NewModbusServer::reMapInputRegisterValuesForAnalogics(const std::vector<uint16_t>& newValues) {
    // Lock the mutex to ensure thread safety while accessing mb_mapping
    std::lock_guard<std::mutex> lock(mb_mapping_mutex);

    // Check if mb_mapping is valid
    if (!mb_mapping) {
        // If mb_mapping is not valid, return without making any changes
        return;
    }

    // Check if tab_input_registers is valid
    if (!mb_mapping->tab_input_registers) {
        // If tab_input_registers is not valid, return without making any changes
        return;
    }

    // Determine the number of registers to write, ensuring not to exceed the allocated array size
    size_t numRegistersToWrite = std::min(newValues.size(), static_cast<size_t>(MODBUS_MAX_READ_REGISTERS));

    // Copy new values to the input registers
    for (size_t i = 0; i < numRegistersToWrite; ++i) {
        mb_mapping->tab_input_registers[i] = newValues[i];
    }
}

void NewModbusServer::reMapCoilsValues(const std::vector<bool>& newValues) {
    // Lock the mutex to ensure thread safety while accessing mb_mapping
    std::lock_guard<std::mutex> lock(mb_mapping_mutex);

    // Iterate through the new values and update corresponding coils in mb_mapping
    for (size_t i = 0; i < newValues.size() && i < MODBUS_MAX_READ_BITS; ++i) {
        // Check if the index is within bounds of mb_mapping->tab_bits
        if (i < MODBUS_MAX_READ_BITS) {
            // Update the coil value in mb_mapping
            mb_mapping->tab_bits[i] = newValues[i];
        } else {
            // If the index exceeds the bounds, break the loop to prevent accessing invalid memory
            break;
        }
    }
}

bool NewModbusServer::modbusSetSlaveId(int newSlaveId) {
    // Lock the mutex to ensure thread safety when accessing the modbus context (ctx)
    std::lock_guard<std::mutex> lock(ctxMutex);

    // Check if the newSlaveId is within the valid range (0 to 255)
    if (newSlaveId < 0 || newSlaveId > 255) 
    {
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::modbusSetSlaveId(int newSlaveId) Invalid slave ID. Must be between 0 and 255.");
        std::cerr << "Invalid slave ID. Must be between 0 and 255." << std::endl;
        return false; // Return false to indicate failure
    }

    // Attempt to set the new slave ID
    if (modbus_set_slave(ctx, newSlaveId) == -1) {
        // If there was an error, retrieve the error message and handle it
        std::string error = modbus_strerror(errno);
        appendCommentWithTimestamp(fileNamesContainer.newModbusServerLogFile,"in NewModbusServer::modbusSetSlaveId(int newSlaveId) Failed to set the slave ID: "+error);
        std::cerr << "Failed to set the slave ID: " << error << std::endl;
        return false; // Return false to indicate failure
    }

    return true; // Return true to indicate success
}

// Get the SRU mapping structure with thread-safe access
SensorRigUpStruct NewModbusServer::getSRUMapping() const {
    std::lock_guard<std::mutex> lock(sruMappingMutex);
    return SRUMapping;
}

// Set the SRU mapping structure with thread-safe access
void NewModbusServer::setSRUMapping(const SensorRigUpStruct &newMapping)
{
    std::lock_guard<std::mutex> lock(sruMappingMutex);
    SRUMapping = newMapping;
}
// Calculate and return the size of the SRU mapping without alarms
int NewModbusServer::getSRUMappingSizeWithoutAlarms()
{
    int totalSize =  SRUMapping.m_nbSRUAnalogsIn    +
                     SRUMapping.m_nbSRUAnalogsOut   +
                    (SRUMapping.m_nbSRUCoders   *2) + //2 registers of 16 bits each for each coder
                    (SRUMapping.m_nbSRUCounters *3);  // 3 registers of 16 bits each for each counter (1x frequency, 2x value) 
    return totalSize;
}

std::shared_ptr<NItoModbusBridge> NewModbusServer::getModbusBridge() const
{
    return m_modbusBridge;
}

void NewModbusServer::setModbusBridge(const std::shared_ptr<NItoModbusBridge> &modbusBridge)
{
    m_modbusBridge = modbusBridge;
}
