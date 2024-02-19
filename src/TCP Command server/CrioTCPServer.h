#ifndef CRIO_TCPSERVER_H
#define CRIO_TCPSERVER_H

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include "../NiWrappers/QNiSysConfigWrapper.h"
#include "../channelReaders/analogicReader.h"
#include "../channelReaders/digitalReader.h"
#include "../Bridge/niToModbusBridge.h"
#include "../stringUtils/stringUtils.h"
#include "../NiWrappers/QNiDaqWrapper.h"


class CrioTCPServer {
public:
    CrioTCPServer(unsigned short port,
                  std::shared_ptr<QNiSysConfigWrapper> aConfigWrapper,
                  std::shared_ptr<QNiDaqWrapper>          aDaqWrapper,
                  std::shared_ptr<AnalogicReader>      anAnalogicReader,
                  std::shared_ptr<DigitalReader>       aDigitalReader,
                  std::shared_ptr<NItoModbusBridge>    aBridge);
    ~CrioTCPServer();

    void startServer();
    void stopServer();

private:
    unsigned short port_;
    std::shared_ptr<QNiSysConfigWrapper> m_cfgWrapper;
    std::shared_ptr<QNiDaqWrapper>       m_daqWrapper;
    std::shared_ptr<AnalogicReader>      m_analogicReader;
    std::shared_ptr<DigitalReader>       m_digitalReader;
    std::shared_ptr<NItoModbusBridge>    m_bridge;
    
    
    std::atomic<bool> serverRunning_;
    std::vector<std::thread> clientThreads_;
    std::mutex clientMutex_;
    std::condition_variable clientCondition_;

    void acceptClients();
    void handleClient(int clientSocket);
    std::string parseRequest(const std::string& request);
    void tokenize(const std::string& input, std::vector<std::string>& tokens, bool &ok); 
    bool checkForReadCommand(const std::string& request, const std::string& command);

};

#endif // CRIO_TCPSERVER_H