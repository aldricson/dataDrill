#ifndef CRIO_SSLSERVER_H
#define CRIO_SSLSERVER_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <map>
#include "../NiWrappers/QNiSysConfigWrapper.h"
#include "../channelReaders/analogicReader.h"
#include "../channelReaders/digitalReader.h"
#include "../channelWriters/digitalWriter.h"
#include "../Bridge/niToModbusBridge.h"
#include "../stringUtils/stringUtils.h"
#include "../NiWrappers/QNiDaqWrapper.h"
#include "../globals/globalEnumStructs.h"

#define maxNbClient 100

class CrioSSLServer {
public:
    CrioSSLServer(unsigned short port,
                  std::shared_ptr<QNiSysConfigWrapper> aConfigWrapper,
                  std::shared_ptr<QNiDaqWrapper> aDaqWrapper,
                  std::shared_ptr<AnalogicReader> anAnalogicReader,
                  std::shared_ptr<DigitalReader> aDigitalReader,
                  std::shared_ptr<DigitalWriter> aDigitalWriter,
                  std::shared_ptr<NItoModbusBridge> aBridge);
    ~CrioSSLServer();

    void startServer();
    void stopServer();

private:
    unsigned short port_;
    

    std::map<int, std::string> m_clientIPs; // Map to store client sockets and their IP addresses


    std::shared_ptr<QNiSysConfigWrapper> m_cfgWrapper;
    std::shared_ptr<QNiDaqWrapper>       m_daqWrapper;
    std::shared_ptr<AnalogicReader>      m_analogicReader;
    std::shared_ptr<DigitalReader>       m_digitalReader;
    std::shared_ptr<DigitalWriter>       m_digitalWriter;
    std::shared_ptr<NItoModbusBridge>    m_bridge;

    
    std::string certFile = "/home/dataDrill//dataDrill.crt";
    std::string keyFile  = "/home/dataDrill/dataDrill.key";

    std::atomic<bool> serverRunning_;
    std::vector<std::thread> clientThreads_;
    std::mutex clientMutex_;
    std::condition_variable clientCondition_;

    SSL_CTX* sslContext_;


    void initializeSSLContext();
    void cleanupSSLContext();
    void acceptClients();
    void handleClient(int clientSocket, std::shared_ptr<SSL> ssl); 
    std::string parseRequest(const std::string& request, std::shared_ptr<SSL> ssl);
    void tokenize(const std::string& input, std::vector<std::string>& tokens, bool& ok); 
    bool checkForReadCommand(const std::string& request, const std::string& command);
    void logSslErrors(const std::string& message); 
    std::string handleFileUploadToClient(std::shared_ptr<SSL> ssl, const std::vector<std::string>& tokens);
    std::string handleFileDownloadFromClient(std::shared_ptr<SSL> ssl, const std::vector<std::string>& tokens);
    std::string getClientList();
    std::string getIniFilesList();
    void gracefulSSLShutdown(std::shared_ptr<SSL> ssl, int clientSocket, bool isCriticalError);

 
};

#endif // CRIO_SSLSERVER_H
