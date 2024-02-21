#include "CrioSSLServer.h"
#include <fcntl.h> // Include fcntl for manipulating file descriptor options

#include <dirent.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <regex>

// Custom deleter for SSL objects
struct SslDeleter {
    void operator()(SSL* ptr) const {
        if (ptr) {
            SSL_free(ptr);
        }
    }
};

CrioSSLServer::CrioSSLServer(unsigned short port,
                            std::shared_ptr<QNiSysConfigWrapper> aConfigWrapper,
                            std::shared_ptr<QNiDaqWrapper>       aDaqWrapper,
                            std::shared_ptr<AnalogicReader>      anAnalogicReader,
                            std::shared_ptr<DigitalReader>       aDigitalReader  ,
                            std::shared_ptr<DigitalWriter>       aDigitalWriter  ,
                            std::shared_ptr<NItoModbusBridge>    aBridge) : 
                            port_(port),
                            m_cfgWrapper(aConfigWrapper),
                            m_daqWrapper(aDaqWrapper),
                            m_analogicReader(anAnalogicReader),
                            m_digitalReader(aDigitalReader),
                            m_digitalWriter(aDigitalWriter),
                            m_bridge(aBridge),
                            serverRunning_(false) 
{
    initializeSSLContext();
}

CrioSSLServer::~CrioSSLServer() 
{
    stopServer();
    cleanupSSLContext(); // Ensure SSL context and resources are cleaned up
}

void CrioSSLServer::startServer() {
    serverRunning_ = true;
    std::thread(&CrioSSLServer::acceptClients, this).detach();
}

void CrioSSLServer::stopServer() {
    serverRunning_ = false;
    clientCondition_.notify_all();
    for (auto& th : clientThreads_) {
        if (th.joinable()) {
            th.join();
        }
    }
}

void CrioSSLServer::initializeSSLContext() 
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    sslContext_ = SSL_CTX_new(SSLv23_server_method());

    if (!sslContext_) {
        logSslErrors("Failed to create SSL context");
        throw std::runtime_error("Failed to create SSL context");
    }

    // Check if certificate file exists
    std::ifstream certFileStream(certFile.c_str());
    if (!certFileStream.good()) 
    {
        std::cerr << "Certificate file " << certFile << " does not exist." << std::endl;
        SSL_CTX_free(sslContext_);
        throw std::runtime_error("Certificate file does not exist");
    }

    // Now use the certificate
    if (SSL_CTX_use_certificate_file(sslContext_, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        logSslErrors("Failed to load certificate");
        SSL_CTX_free(sslContext_);
        throw std::runtime_error("Failed to load certificate");
    }

    if (SSL_CTX_use_PrivateKey_file(sslContext_, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        logSslErrors("Failed to load private key");
        SSL_CTX_free(sslContext_);
        throw std::runtime_error("Failed to load private key");
    }
}

void CrioSSLServer::logSslErrors(const std::string& message) 
{
    std::cerr << message << std::endl;
    unsigned long errCode;
    while((errCode = ERR_get_error())) {
        char *err = ERR_error_string(errCode, NULL);
        std::cerr << "OpenSSL error: " << err << std::endl;
    }
}

std::string CrioSSLServer::handleFileUploadToClient(std::shared_ptr<SSL> ssl, const std::vector<std::string>& tokens) 
{
    if (tokens.size() != 2) {
        return "NACK: Incorrect download command format";
    }

    const std::string& filename = tokens[1];
    std::ifstream inFile(filename, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) {
        return "NACK: Unable to open file for reading";
    }

    long long fileSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    std::string sizeResponse = "Size:" + std::to_string(fileSize);
    SSL_write(ssl.get(), sizeResponse.c_str(), sizeResponse.size());

    char buffer[1024];
    while (!inFile.eof()) {
        inFile.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = inFile.gcount();
        SSL_write(ssl.get(), buffer, bytesRead);
    }

    return "ACK: File download successful";
}

std::string CrioSSLServer::handleFileDownloadFromClient(std::shared_ptr<SSL> ssl, const std::vector<std::string>& tokens) 
{
    if (tokens.size() != 3) {
        return "NACK: Incorrect upload command format";
    }

    const std::string& filename = tokens[1];
    long long fileSize;
    try {
        fileSize = std::stoll(tokens[2]);
    } catch (...) {
        return "NACK: Invalid file size";
    }

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) {
        return "NACK: Unable to open file for writing";
    }

    char buffer[1024];
    long long totalBytesRead = 0;
    while (totalBytesRead < fileSize) {
        int bytesRead = SSL_read(ssl.get(), buffer, sizeof(buffer));
        if (bytesRead <= 0) break;  // Error or disconnect
        outFile.write(buffer, bytesRead);
        totalBytesRead += bytesRead;
    }

    if (totalBytesRead != fileSize) {
        return "NACK: File transfer incomplete";
    }

    return "ACK: File upload successful";
}



void CrioSSLServer::cleanupSSLContext() 
{
    // Check if the SSL context exists
    if (sslContext_ != nullptr) {
        // Free the SSL context
        SSL_CTX_free(sslContext_);
        sslContext_ = nullptr; // Set the pointer to nullptr to avoid use after free
    }

    // Clean up OpenSSL error strings and algorithms
    ERR_free_strings();
    EVP_cleanup();
}


void CrioSSLServer::acceptClients() {
    // Create a server socket using TCP in the IPv4 domain.
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        // If creating the socket fails, throw an exception with details.
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Lambda function for closing the server socket, ensuring cleanup.
    auto cleanup = [&]() { close(serverSocket); };

    // Option for the socket to reuse the address.
    int opt = 1;
    // Set the option on the socket for address reuse.
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        // If setting the socket option fails, perform cleanup and throw an error.
        cleanup();
        throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }

    // Set the server socket to non-blocking mode.
    fcntl(serverSocket, F_SETFL, O_NONBLOCK);

    // Create a structure to hold server address information.
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET; // Address family (IPv4).
    serverAddr.sin_port = htons(port_); // Port in network byte order.
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces.

    // Bind the socket to the server address.
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        // If binding fails, perform cleanup and throw an error.
        cleanup();
        throw std::runtime_error("Failed to bind to port: " + std::string(strerror(errno)));
    }

    // Listen for incoming connections on the socket.
    if (listen(serverSocket, maxNbClient) < 0) {
        // If listening fails, perform cleanup and throw an error.
        cleanup();
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }

    // Main loop for accepting incoming connections.
    while (serverRunning_) {
        sockaddr_in clientAddr; // Struct to store client address.
        socklen_t clientAddrLen = sizeof(clientAddr); // Size of the client address structure.

        // Accept a new connection.
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        // Check if the accept call was successful.
        if (clientSocket < 0) {
            // If no pending connections, pause to prevent CPU spinning.
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                // Log other errors and continue.
                std::cerr << "Error accepting client: " << strerror(errno) << std::endl;
                continue;
            }
        }

        // Convert client IP to string format.
        std::string clientIP = inet_ntoa(clientAddr.sin_addr);
        // Create a shared pointer for SSL object management.
        std::shared_ptr<SSL> ssl(SSL_new(sslContext_), [](SSL* ptr) { if (ptr) SSL_free(ptr); });

        // Check if SSL object is created successfully and perform SSL handshake.
        if (!ssl || SSL_set_fd(ssl.get(), clientSocket) <= 0 || SSL_accept(ssl.get()) <= 0) {
            // If any SSL step fails, close the client socket and continue.
            close(clientSocket);
            continue;
        }

        // Lock client list for thread safety.
        std::lock_guard<std::mutex> lock(clientMutex_);
        // Store the client's IP address in the list.
        m_clientIPs[clientSocket] = clientIP;

        // Spawn a new thread to handle the client.
        clientThreads_.push_back(std::thread(&CrioSSLServer::handleClient, this, clientSocket, ssl));
    }

    // Perform cleanup of the server socket after the loop ends.
    cleanup();
}



void CrioSSLServer::handleClient(int clientSocket, std::shared_ptr<SSL> ssl) 
{
    bool criticalError = false; // Flag to track critical SSL errors.
    try {
        char buffer[257]; // Buffer to store data read from SSL.
        std::string accumulatedData; // String to accumulate received data.
        const size_t maxMessageSize = 256; // Max size of message to read.

        // Loop while the server is running.
        while (serverRunning_) 
        {
            memset(buffer, 0, sizeof(buffer)); // Clear the buffer before reading.
            ssize_t bytesRead = SSL_read(ssl.get(), buffer, maxMessageSize); // Read data from SSL.

            // Check if SSL read was successful.
            if (bytesRead <= 0) {
                int sslError = SSL_get_error(ssl.get(), bytesRead); // Get SSL error code.

                // Check for graceful disconnection or error.
                if (bytesRead == 0 || sslError == SSL_ERROR_ZERO_RETURN) {
                    std::cout << "Client disconnected gracefully: Socket " << clientSocket << std::endl;
                } else if (sslError == SSL_ERROR_SYSCALL) {
                    std::cerr << "SSL read error (syscall) on socket " << clientSocket << std::endl;
                    criticalError = true; // Mark as critical error.
                } else {
                    std::cerr << "SSL read error on socket " << clientSocket << ": " << sslError << std::endl;
                    criticalError = true; // Mark as critical error.
                }
                break; // Exit loop on disconnection or error.
            }

            buffer[bytesRead] = '\0'; // Null-terminate the buffer.
            accumulatedData += buffer; // Append data to the accumulated string.

             if (accumulatedData.length() > maxMessageSize) 
             {
                std::string response = "NACK: command rejected";
                // Use SSL_write instead of send for SSL communication
                SSL_write(ssl.get(), response.c_str(), response.size());
                break; // Optionally disconnect the client
            }

            size_t delimiterPos = accumulatedData.find('\n');
            if (delimiterPos != std::string::npos) 
            {
                std::string completeMessage = accumulatedData.substr(0, delimiterPos);
                accumulatedData.erase(0, delimiterPos + 1);

                std::string response = parseRequest(completeMessage,ssl);
                // Use SSL_write to send response back to the client
                SSL_write(ssl.get(), response.c_str(), response.size());
            }
    

        }
    } 
    catch (const std::exception& e) 
    {
        // Catch and log any exceptions in client handling.
        std::cerr << "Exception in handleClient: " << e.what() << std::endl;
    }

    // Perform graceful shutdown of SSL and closing of socket.
    gracefulSSLShutdown(ssl, clientSocket, criticalError);
}

void CrioSSLServer::gracefulSSLShutdown(std::shared_ptr<SSL> ssl, int clientSocket, bool isCriticalError) 
{

    if (ssl && ssl.get() && !isCriticalError) 
    {
        // Shutdown SSL connection gracefully.
        int shutdownResult = SSL_shutdown(ssl.get()); // First call to SSL_shutdown.
        if (shutdownResult == 0) {
            // Call SSL_shutdown again if the peer hasn't sent "close notify".
            SSL_shutdown(ssl.get());
        }
    }
    if (clientSocket >= 0) {
        // Close the client socket.
        close(clientSocket);
    }

    // Lock mutex for thread safety when modifying client list.
    std::lock_guard<std::mutex> lock(clientMutex_);
    // Remove the client from the list.
    m_clientIPs.erase(clientSocket);
}



std::string CrioSSLServer::parseRequest(const std::string& request, std::shared_ptr<SSL> ssl) 
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
                        return std::string("ERR: deviceModule is nullptr");
                    }
                }
                catch (const std::bad_alloc& e)
                {
                    // Handle memory allocation failure
                    return std::string("ERR: Memory allocation failed") + e.what();
                }
                
            }
            else
            {
                return ("ERR:Impossible to convert "+tokens[2]+"to unsignedInt");
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
        try
        {
            if (m_bridge->getSimulateTimer()->isActive()) 
            {
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
    else if (checkForReadCommand(tokens[0], "uploadToClient")) 
    {
        return handleFileUploadToClient(ssl, tokens);
    } 
    else if (checkForReadCommand(tokens[0], "downloadFromClient")) 
    {
        return handleFileDownloadFromClient(ssl, tokens);
    }
    else if (checkForReadCommand(tokens[0], "clientList"))
    {
        return getClientList();
    }
    else if (checkForReadCommand(tokens[0],"listInifiles"))
    {
        return getIniFilesList(); 
    }
    else
    {
        return "unknow command "+tokens[0];
    }
}

std::string CrioSSLServer::getClientList()
{
    std::string clientList;
    for (const auto& entry : m_clientIPs) 
    {
        if (!clientList.empty()) clientList += ";";
        clientList += entry.second;
    }
    return clientList;
}

std::string CrioSSLServer::getIniFilesList() {
    DIR *dir;
    struct dirent *ent;
    std::string result;

    // Regular expression to match the specific pattern
    std::regex pattern("^NI.*_[0-9]+\\.ini$");

    // Open the current directory
    dir = opendir(".");
    if (dir != nullptr) {
        // Iterate over the directory entries
        while ((ent = readdir(dir)) != nullptr) {
            // Check if the file name matches the specified pattern
            if (std::regex_match(ent->d_name, pattern)) {
                if (!result.empty()) {
                    result += ";";
                }
                result += ent->d_name;
            }
        }
        closedir(dir);
    } else {
        // Could not open directory
        perror("Could not open directory");
        return "";
    }

    return result;
}



void CrioSSLServer::tokenize(const std::string& input, std::vector<std::string>& tokens, bool &ok) 
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

bool CrioSSLServer::checkForReadCommand(const std::string& request, const std::string& command)
{
    std::string mainString      =  request;
    std::string substringToFind = command;
    size_t foundPos = mainString.find(substringToFind);
    return foundPos != std::string::npos;
}


