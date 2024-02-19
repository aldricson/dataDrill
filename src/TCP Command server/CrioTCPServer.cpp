#include "CrioTCPServer.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


CrioTCPServer::CrioTCPServer(unsigned short port,
                            std::shared_ptr<QNiSysConfigWrapper> aConfigWrapper,
                            std::shared_ptr<QNiDaqWrapper>       aDaqWrapper,
                            std::shared_ptr<AnalogicReader>      anAnalogicReader,
                            std::shared_ptr<DigitalReader>       aDigitalReader,
                            std::shared_ptr<NItoModbusBridge>    aBridge) : 
                            port_(port),
                            m_cfgWrapper(aConfigWrapper),
                            m_daqWrapper(aDaqWrapper),
                            m_analogicReader(anAnalogicReader),
                            m_digitalReader(aDigitalReader),
                            m_bridge(aBridge),
                            serverRunning_(false) 
{
    //
}

CrioTCPServer::~CrioTCPServer() {
    stopServer();
}

void CrioTCPServer::startServer() {
    serverRunning_ = true;
    std::thread(&CrioTCPServer::acceptClients, this).detach();
}

void CrioTCPServer::stopServer() {
    serverRunning_ = false;
    clientCondition_.notify_all();
    for (auto& th : clientThreads_) {
        if (th.joinable()) {
            th.join();
        }
    }
}


void CrioTCPServer::acceptClients() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    auto cleanup = [&]() { close(serverSocket); };

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cleanup();
        throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        cleanup();
        throw std::runtime_error("Failed to bind to port: " + std::string(strerror(errno)));
    }

    if (listen(serverSocket, 10) < 0) {
        cleanup();
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }

    while (serverRunning_) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            if (errno == EINTR) {
                // If accept was interrupted by a signal, check if server is still running, then continue
                continue;
            } else {
                cleanup();
                throw std::runtime_error("Failed to accept client: " + std::string(strerror(errno)));
            }
        }
        
        // Successfully accepted a client
        std::lock_guard<std::mutex> lock(clientMutex_);
        clientThreads_.push_back(std::thread(&CrioTCPServer::handleClient, this, clientSocket));
    }

    cleanup();
}



void CrioTCPServer::handleClient(int clientSocket) {
    char buffer[257]; // Buffer size increased by 1 for null termination
    std::string accumulatedData;
    const size_t maxMessageSize = 256;

    while (serverRunning_) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(clientSocket, buffer, maxMessageSize, 0);
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                std::cout << "Client disconnected: Socket " << clientSocket << std::endl;
            } else {
                std::cerr << "Error receiving data: Socket " << clientSocket << std::endl;
            }
            break;
        }

        buffer[bytesRead] = '\0';
        accumulatedData += buffer;

        if (accumulatedData.length() > maxMessageSize) {
            std::string response = "NACK: command rejected";
            send(clientSocket, response.c_str(), response.size(), 0);
            break; // Optionally disconnect the client
        }

        size_t delimiterPos = accumulatedData.find('\n');
        if (delimiterPos != std::string::npos) {
            std::string completeMessage = accumulatedData.substr(0, delimiterPos);
            accumulatedData.erase(0, delimiterPos + 1);

            std::string response = parseRequest(completeMessage);
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    }

    close(clientSocket);
}




std::string CrioTCPServer::parseRequest(const std::string& request) 
{
    std::string requestCopy = request;
    std::vector<std::string> tokens;
    bool ok; 
    tokenize(requestCopy, tokens, ok);
    if (tokens.size() > 3 || !ok) 
    {
        return "NACK: Invalid command format";
    }

    if (checkForReadCommand(tokens[0],"readCurrent"))
    {
        // Ensure there are enough tokens for a valid command
        if (tokens.size() < 3) 
        {
            return "NACK: Invalid command format";
        }
        try
        {
            std::string moduleAlias = tokens[1];
            bool ok;
            unsigned int channelIndex = strToUnsignedInt(tokens[2],ok);
            //double result = m_daqWrapper->readCurrent(deviceModule,channelIndex,50,true); 
            double result;
            m_analogicReader->manualReadOneShot(moduleAlias,channelIndex,result);
            return  std::to_string(result); 
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return std::string("NACK:") + e.what();
        }   
    }

    else if (checkForReadCommand(tokens[0],"readVoltage"))
    {
        // Ensure there are enough tokens for a valid command
        if (tokens.size() < 3) 
        {
            return "NACK: Invalid command format";
        }
        try
        {
            std::string moduleAlias = tokens[1];
            bool ok;
            unsigned int channelIndex = strToUnsignedInt(tokens[2],ok);
            if (ok)
            {
                //std::cout<<"received readVoltage: "<<moduleAlias<<" channel index: "<<tokens[2]<<std::endl;

                try 
                {
                    NIDeviceModule *deviceModule = m_cfgWrapper->getModuleByAlias(moduleAlias);
                    if (deviceModule != nullptr) 
                    {
                        double result = m_daqWrapper->readVoltage(deviceModule,channelIndex,50);
                        return std::to_string(result);
                    } 
                    else 
                    {
                        return std::string("NACK: deviceModule is nullptr");
                    }
                }
                catch (const std::bad_alloc& e)
                {
                    // Handle memory allocation failure
                    std::cerr << "Memory allocation failed: " << e.what() << '\n';
                    return std::string("NACK: Memory allocation failed") + e.what();
                }
                
            }
            else
            {
                return ("NACK:Impossible to convert "+tokens[2]+"to unsignedInt");
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return std::string("NACK:") + e.what();
        }      
    }

    else if (checkForReadCommand(tokens[0],"startModbusSimulation"))
    {
        // Ensure there are enough tokens for a valid command
        if (tokens.size() != 1) 
        {
            return "NACK: Invalid command format";
        }
        
        //broadCastStr("crio debug:\nstartModbusSimulation detected\nin std::string CrioTCPServer::parseRequest(const std::string& request)\n"); 
        try
        {
            if (m_bridge->getSimulateTimer()->isActive()) 
            {
                //broadCastStr("crio debug:\nsimulation timer already active\nin std::string CrioTCPServer::parseRequest(const std::string& request)\n"); 
                //simulation already avtive
                return "ACK";
            }
            if (m_bridge->startModbusSimulation()) 
            {
               return ("ACK");
            }
            else
            { 
              return ("NACK: Impossible to start modbus simulation");
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            //broadCastStr("crio debug:\n"+ std::string(e.what())+"\nin std::string CrioTCPServer::parseRequest(const std::string& request)\n");
            return std::string("NACK:") + std::string(e.what());
        }
    }

    else if (checkForReadCommand(tokens[0],"stopModbusSimulation"))
    {
        // Ensure there is only one token for a valid command
        if (tokens.size() != 1) 
        {
            return "NACK: Invalid command format";
        }
        
        try
        {
            m_bridge->stopModbusSimulation();
            return ("ACK"); 
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return std::string("NACK:") + e.what();
        }
    }

    else if (checkForReadCommand(tokens[0],"startModbusAcquisition"))
    {
        // Ensure there is only one token for a valid command
        if (tokens.size() != 1) 
        {
            return "NACK: Invalid command format";
        }
        
        try
        {
            if (m_bridge->getDataAcquTimer()->isActive()) 
            {
                return "ACK";
            }
            if (m_bridge->startAcquisition()) 
            {
               return ("ACK");
            }
            else
            { 
              return ("NACK: Impossible to start modbus acquisition");
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return std::string("NACK:") + std::string(e.what());
        }
    }

    else if (checkForReadCommand(tokens[0],"stopModbusAcquisition"))
    {
        // Ensure there is only one token for a valid command
        if (tokens.size() != 1) 
        {
            return "NACK: Invalid command format";
        }
        
        try
        {
            m_bridge->stopAcquisition();
            return ("ACK"); 
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return std::string("NACK:") + e.what();
        }
    }

    else
    {
        return "unknow command";
    }
}



void CrioTCPServer::tokenize(const std::string& input, std::vector<std::string>& tokens, bool &ok) 
{
    std::string inputCopy = input;
    size_t pos = 0;
    size_t nbToken=0;
    ok = true;
    while ((pos = inputCopy.find(';')) != std::string::npos)
    {
        std::string token = inputCopy.substr(0, pos);
        tokens.push_back(token);
        inputCopy.erase(0, pos + 1);  // Move the position past the semicolon
        if (nbToken>=20)
        {
            ok = false;
            break;
        }
        nbToken++;
    }

    // After the loop, inputCopy may still have some data after the last semicolon, which can be considered as another token.
    if (!inputCopy.empty())
    {
        tokens.push_back(inputCopy);
    }
}

bool CrioTCPServer::checkForReadCommand(const std::string& request, const std::string& command)
{
    std::string mainString      =  request;
    std::string substringToFind = command;
    size_t foundPos = mainString.find(substringToFind);
    return foundPos != std::string::npos;
}


