#include "QNiDaqWrapper.h"
#include <iostream>
#include <cstring>
#include <random>
#include <iomanip>
#include <memory>
#include "../NiModulesDefinitions/NIDeviceModule.h"
#include "../mathUtils/mathUtils.h"




static int32 CVICALLBACK CurrentDoneCallback(TaskHandle taskHandle, 
                                             int32 status,
                                             void *callbackData)
{
    // Check for null pointer before casting
    if (!callbackData) 
    {
        std::cerr << "CurrentDoneCallback: callbackData is null." << std::endl;
        return -1;  // Return an error code to indicate failure
    }

    // Safe casting of callbackData to QNiDaqWrapper*
    QNiDaqWrapper *daqWrapper = static_cast<QNiDaqWrapper*>(callbackData);

    // Handle the completion of the read current task
    daqWrapper->handleReadCurrentCompletion(status);

    // Return 0 to indicate successful execution
    return 0;
}

static int32 CVICALLBACK VoltageDoneCallback(TaskHandle taskHandle, 
                                             int32 status,
                                             void *callbackData)
{
    // Check for null pointer before casting
    if (!callbackData) {
        std::cerr << "VoltageDoneCallback: callbackData is null." << std::endl;
        return -1;  // Return an error code to indicate a null pointer was received
    }

    // Safely cast callbackData to QNiDaqWrapper*
    QNiDaqWrapper *daqWrapper = static_cast<QNiDaqWrapper*>(callbackData);

    // Execute the handler for voltage read completion
    daqWrapper->handleReadVoltageCompletion(status);

    // Return 0 to indicate successful execution of the callback
    return 0;
}

// Add a new callback for counter completion


static int32 CVICALLBACK CounterDoneCallback(TaskHandle taskHandle, 
                                             int32 status,
                                             void *callbackData)
{
    // Check for null pointer before casting
    if (!callbackData) {
        std::cerr << "CounterDoneCallback: callbackData is null." << std::endl;
        return -1;  // Return an error code to indicate a null pointer was received
    }

    // Safely cast callbackData to QNiDaqWrapper*
    QNiDaqWrapper *daqWrapper = static_cast<QNiDaqWrapper*>(callbackData);

    // Execute the handler for counter read completion
    daqWrapper->handleReadCounterCompletion(status);

    // Return 0 to indicate successful execution of the callback
    return 0;
}



QNiDaqWrapper::QNiDaqWrapper() 
{

    // Store initial values for current and voltage readings.
    // It's crucial to initialize these variables to prevent undefined behavior.
    // Using std::atomic for thread-safe operations on these values.
    m_lastSingleCurrentChannelValue.store(0.0);
    m_lastSingleVoltageChannelValue.store(0.0);

    // Further initialization code can go here.
    // This may include setting up additional resources, configuring hardware, etc.,
    // with appropriate error checking and handling.
}


QNiDaqWrapper::~QNiDaqWrapper() {

}


int32 QNiDaqWrapper::GetNumberOfModules() 
{
    char devNames[512]; // Buffer to hold device names

    // Retrieve the list of device names
    int32 error = DAQmxGetSystemInfoAttribute(DAQmx_Sys_DevNames, devNames, sizeof(devNames));
    
    // Check for errors in retrieving device names
    if (error) 
    {
        char errBuff[2048] = {'\0'}; // Buffer for the error description
        // Retrieve a human-readable error message
        DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "int32 QNiDaqWrapper::GetNumberOfModules()\n"
                                   "Error: unable to get system info attributes\n"+
                                   std::to_string(error)+
                                   "\n"+std::string(errBuff));

        return -1;  // Return -1 to indicate an error
    }

    // Ensure the string is null-terminated to prevent buffer overrun
    devNames[sizeof(devNames) - 1] = '\0';

    // Use strtok_r for a thread-safe alternative to strtok
    char *context = nullptr;
    char *token = strtok_r(devNames, ", ", &context);
    int32 moduleCount = 0;

    // Iterate through tokens to count modules
    while (token != NULL) {
        moduleCount++;
        token = strtok_r(NULL, ", ", &context);
    }
   
    return moduleCount; // Return the count of modules
}


std::vector<std::string> QNiDaqWrapper::GetDevicesList() 
{
    std::vector<std::string> devices;
    int32 error;
    uInt32 bufferSize = 0;
    TaskHandle taskHandle = 0;
    // Create a new task for device enumeration
    error = DAQmxCreateTask("getDevice", &taskHandle);
    char errBuff[2048] = {'\0'}; // Buffer for the error description
    if (error < 0) 
    {
        // Retrieve a human-readable error message
        DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "std::vector<std::string> QNiDaqWrapper::GetDevicesList()\n"
                                   "Error: unable to create task\n"+
                                   std::to_string(error)+
                                   "\n"+std::string(errBuff));
        return devices;
    }

    // First call to get the required buffer size. Note the use of &bufferSize to pass a pointer.
    error = DAQmxGetSysDevNames(nullptr, bufferSize);
    if (error < 0) 
    {
        DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "std::vector<std::string> QNiDaqWrapper::GetDevicesList()\n"
                                   "Error: unable to determinate buffer size for device names\n"+
                                   std::to_string(error)+
                                   "\n"+std::string(errBuff));
        DAQmxClearTask(taskHandle);
        return devices;
    }

    if (bufferSize == 0) 
    {
       appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "std::vector<std::string> QNiDaqWrapper::GetDevicesList()\n"
                                   "Error: Buffer size is 0");
        DAQmxClearTask(taskHandle);
        return devices;
    }

    // Allocate buffer for device names
    std::vector<char> devNames(bufferSize);

    // Second call to actually get the device names
    error = DAQmxGetSysDevNames(devNames.data(), bufferSize);
    if (error < 0) 
    {
        DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "std::vector<std::string> QNiDaqWrapper::GetDevicesList()\n"
                                   "Error: unable to  retrieve device names.\n"+
                                   std::to_string(error)+
                                   "\n"+std::string(errBuff));
        DAQmxClearTask(taskHandle);
        return devices;
    }

    // Convert buffer to string and parse it
    std::string devNamesStr(devNames.begin(), devNames.end());
    std::istringstream iss(devNamesStr);
    std::string name;
    while (std::getline(iss, name, ',')) {
        devices.push_back(name);
    }

    DAQmxClearTask(taskHandle);
    return devices;
}


unsigned char QNiDaqWrapper::random_char() 
{
    // Using thread_local to ensure that each thread has its own instance of the random engine and device.
    // This is more efficient than creating a new instance on every function call and thread-safe.
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd()); 

    // Uniform integer distribution to generate a number between 0 and 255 inclusive.
    // Using a static distribution object for efficiency.
    static std::uniform_int_distribution<> dis(0, 255);

    // Generate and return a random character
    // static_cast is used to convert the integer to an unsigned char.
    return static_cast<unsigned char>(dis(gen));
}


std::string QNiDaqWrapper::generate_hex(const unsigned int len) 
{
    std::stringstream ss;

    // Loop through the specified length
    for(unsigned int i = 0; i < len; i++) {
        // Generate a random character
        auto rc = random_char();

        // Use std::hex and std::setw to convert the character to a hex string of length 2.
        // std::setfill('0') ensures that a leading zero is added for single digit hex values.
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(rc);
    }        
    return ss.str(); // Return the hex string
}

std::vector<double> QNiDaqWrapper::lowPassFilterDatas(const std::vector<double> &dataBuffer, float deltaTime, float cutOffFrequency)
{
    std::vector<double> filteredBuffer(dataBuffer.size());
    lpf.reconfigureFilter(deltaTime, cutOffFrequency); // Adjusting the filter based on the current deltaTime and a fixed cutoff frequency
    
    for (size_t i = 0; i < dataBuffer.size(); ++i) 
    {
        filteredBuffer[i] = lpf.update(dataBuffer[i]);
    }
    
    return filteredBuffer;
}



/*double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)
{
    std::lock_guard<std::mutex> lock(currentMutex);
    int32 error = 0;
    char errBuff[2048] = {'\0'};
    double readValue = 0.0;
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: deviceModule is null.");
        throw std::invalid_argument("readCurrent: deviceModule is null.");
    }

    const std::string fullChannelName = deviceModule->getAlias() + chanName;
    // Check if a task for this channel already exists, create if not
    TaskHandle& taskHandle = currentTaskMap[fullChannelName];
    if (taskHandle == nullptr) 
    {
        std::string uniqueKey = "readCurrent" + generate_hex(8); // Generate a unique key for the task
        error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);

       if (error) 
        {
            DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                       "Error: Failed to create DAQmx task. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Ensure the map entry is reset after cleanup
            throw std::runtime_error("Failed to create DAQmx task.");
        }


        float64 shuntVal             = deviceModule -> getModuleShuntValue ();
        double  minRange             = deviceModule -> getChanMin          ();
        double  maxRange             = deviceModule -> getChanMax          ();
        int32   termCfg              = deviceModule -> getModuleTerminalCfg();
        int32   unit                 = deviceModule -> getModuleUnit       ();
        moduleShuntLocation shuntLoc = deviceModule->getModuleShuntLocation();

               // Create an analog input current channel
        error = DAQmxCreateAICurrentChan(taskHandle, 
                                         fullChannelName.c_str(), 
                                         "", 
                                         termCfg, 
                                         minRange, 
                                         maxRange, 
                                         unit, 
                                         shuntLoc, 
                                         shuntVal, 
                                         NULL); 

        if (error) 
        {
            std::cout<<"in read current, failed to create current channel: "<<fullChannelName.c_str()<<std::endl;
            char errBuff[2048];
            DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                       "Failed to create current channel. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Ensure the map entry is reset after cleanup
            throw std::runtime_error("Failed to create current channel.");
        }


                // Start the task.
        error = DAQmxStartTask(taskHandle);
        if (error) 
        {
            std::cout<<"in read current Failed to start task."<<std::endl;
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                       "Failed to start task.");
            this->handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to start task.");
        }

    }


    error = DAQmxReadAnalogScalarF64(taskHandle, 10.0 , &readValue, nullptr);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to read current current value."
                                   "\n"+std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);  // Handle error and clean up
        throw std::runtime_error("Failed to read current value.");
    } 

    return readValue; // Return the successfully read value
}*/




double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)
{
    std::lock_guard<std::mutex> lock(currentMutex); // Ensure thread safety
    TaskHandle taskHandle = 0;

    // Check for null pointer
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Null pointer passed for deviceModule.");

        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    int32 error;
    float64 readValue;

    // Extract necessary information from NIDeviceModule
    const char* deviceName = deviceModule->getAlias().c_str();
    moduleShuntLocation shuntLoc = deviceModule->getModuleShuntLocation();
    if (shuntLoc == noShunt) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: current modules must have a shunt.");
        throw std::runtime_error("Failed current modules must have a shunt.");
    }
    float64 shuntVal = deviceModule->getModuleShuntValue();
    double minRange = deviceModule->getChanMin();
    double maxRange = deviceModule->getChanMax();
    int32 termCfg = deviceModule->getModuleTerminalCfg();
    int32 unit = deviceModule->getModuleUnit();

    // Construct full channel name
    std::string fullChannelName = std::string(deviceName) + chanName;
    unsigned int retryCount = 0;
    std::string unicKey = "getCurrentValue" + generate_hex(8);
    char errBuff[2048] = {'\0'};
    while (true) 
    {
        // Create a new task
        //std::string unicKey = "getCurrentValue" + generate_hex(8);
        error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                       "In\n"
                       "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                       "Error: Failed to create task for reading current.\n"+
                       std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to create task for reading current.");
        }

        // Register the Done callback
       error = DAQmxRegisterDoneEvent(taskHandle, 0, CurrentDoneCallback, this);
        if (error)
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                          "In\n"
                          "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                          "Error: Failed to register Done callback.\n"+
                          std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);

            throw std::runtime_error("Failed to register Done callback.");
        }

        // Create an analog input current channel
        error = DAQmxCreateAICurrentChan(taskHandle, 
                                         fullChannelName.c_str(), 
                                         "", 
                                         termCfg, 
                                         minRange, 
                                         maxRange, 
                                         unit, 
                                         shuntLoc, 
                                         shuntVal, 
                                         NULL);

        if (error) 
        {           
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            if (++retryCount >= maxRetries) 
            {
                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                           "In\n"
                                           "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                           "Error: Failed to create channel after max retries.");
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                        "Error: failed to create Current Channel"
                                        "\n"+std::string(errBuff));
                  
            handleErrorAndCleanTask(taskHandle);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        break;
    }

    // Start the task
    error = DAQmxStartTask(taskHandle);
    if (error) 
    {       
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                    "In\n"
                                    "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                    "Error: failed to start task to read current."
                                    "\n"+std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to starttask for reading current.");
    }
    // Read the current value

    //error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
    error = DAQmxReadAnalogScalarF64(taskHandle, 0.1 , &readValue, nullptr);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to read current current value."
                                   "\n"+std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);  // Handle error and clean up
        throw std::runtime_error("Failed to read current value.");
    } 

    // Stop and clear the task
    error = DAQmxStopTask(taskHandle);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                      "In\n"
                      "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                      "Error: Failed to stop task.\n"+
                      std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);  // Handle error and clean up
        throw std::runtime_error("Failed to stop task.");
    }

    DAQmxClearTask(taskHandle);

    // Convert the read value to the appropriate unit and return it
    double result = static_cast<double>(readValue);
    if (autoConvertTomAmps) 
    {
        result = ampsTomAmps(result);
    }
    setLastSingleCurrentChannelValue(result);
    return result;
}

/*void QNiDaqWrapper::readMod1()
{
    // Hardcoded device name for Mod1 module
    const char* deviceName = "Mod1";
    // Initialize task handle to 0, which will be used to identify our task with DAQmx functions
    TaskHandle taskHandle = readCurrentMod1Task;
    // Error code returned by DAQmx functions
    int32 error;
    // Buffer for error information
    char errBuff[2048] = {'\0'};
    // Total number of channels to read from
    const int32 channelsCount = 16;
    // Specify the range of current to measure
    const float64 minRange = 0.004, maxRange = 0.02;
    // Timeout for DAQmxReadAnalogF64 function in seconds
    const float64 timeout = 1.0;
    // We will request 2 samples per channel to meet minimum sample requirement but only use the last sample
    const int32 samplesPerChannel = 7;
    // Vector to store the last sample from each channel
    std::vector<double> lastSamples(channelsCount); 
    // Number of samples actually read
    int32 read;

    // Construct full channel names for the device, spanning from /ai0 to /ai15
    std::string fullChannelNames = std::string(deviceName) + "/ai0:" + std::to_string(channelsCount-1);

    // Generate a unique task name using a helper function (assumed to be defined elsewhere)
    std::string unicKey = "ReadCurrentMod1" + generate_hex(8);
    // Create a task
    if (taskHandle == nullptr)
    {
         error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             throw std::runtime_error("Failed to create task: " + std::string(errBuff));
         }
     
         // Create analog input current channels for the specified range on the device
         error = DAQmxCreateAICurrentChan(taskHandle, fullChannelNames.c_str(), "", DAQmx_Val_Cfg_Default, minRange, maxRange, DAQmx_Val_Amps, DAQmx_Val_Default, 0.0, NULL);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to create channels: " + std::string(errBuff));
         }
     
         // Configure the sampling clock with a rate of 30 Hz, finite samples, and rising edge timing
         error = DAQmxCfgSampClkTiming(taskHandle, "", 31.25, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, samplesPerChannel);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to configure timing: " + std::string(errBuff));
         }
    }

    
    
    while (true)
    {
        // Start the task to begin measuring
         error = DAQmxStartTask(taskHandle);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to start task: " + std::string(errBuff));
         }

     
         // Prepare a buffer to read the samples into, sized for all channels and samples
         std::vector<double> dataBuffer(channelsCount * samplesPerChannel);
         // Read the samples from all channels into the buffer
         error = DAQmxReadAnalogF64(taskHandle, samplesPerChannel, timeout, DAQmx_Val_GroupByChannel, dataBuffer.data(), dataBuffer.size(), &read, NULL);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to read data: " + std::string(errBuff));
         }
     
        std::string result;
         // Extract the last sample for each channel from the buffer
         for (int i = 0; i < channelsCount; ++i) 
         {
             lastSamples[i] = dataBuffer[i * samplesPerChannel + samplesPerChannel - 1];
             result += std::to_string(lastSamples[i]) + ";";
         }

        Mod1Buffer.restore(lastSamples);


        //Acquisition goes here
        DAQmxStopTask(taskHandle);
    }
}*/

void QNiDaqWrapper::readMod1()
{

    // Hardcoded device name for Mod1 module
    const char* deviceName = "Mod1";
    // Initialize task handle to 0, which will be used to identify our task with DAQmx functions
    TaskHandle taskHandle = readCurrentMod1Task;
    // Error code returned by DAQmx functions
    int32 error;
    // Buffer for error information
    char errBuff[2048] = {'\0'};
    // Total number of channels to read from
    const int32 channelsCount = 16;
    // Specify the range of current to measure
    const float64 minRange = 0.004, maxRange = 0.02;
    // Timeout for DAQmxReadAnalogF64 function in seconds
    const float64 timeout = 1.0;
    // We will request 2 samples per channel to meet minimum sample requirement but only use the last sample
    const int32 samplesPerChannel = 7;
    // Number of samples actually read
    int32 read;

    // Construct full channel names for the device, spanning from /ai0 to /ai15
    std::string fullChannelNames = std::string(deviceName) + "/ai0:" + std::to_string(channelsCount-1);

    // Generate a unique task name using a helper function (assumed to be defined elsewhere)
    std::string unicKey = "ReadCurrentMod1" + generate_hex(8);
    // Create a task
    if (taskHandle == nullptr)
    {
         error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             throw std::runtime_error("Failed to create task: " + std::string(errBuff));
         }
     
         // Create analog input current channels for the specified range on the device
         error = DAQmxCreateAICurrentChan(taskHandle, fullChannelNames.c_str(), "", DAQmx_Val_Cfg_Default, minRange, maxRange, DAQmx_Val_Amps, DAQmx_Val_Default, 0.0, NULL);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to create channels: " + std::string(errBuff));
         }
     
         // Configure the sampling clock with a rate of 30 Hz, finite samples, and rising edge timing
         error = DAQmxCfgSampClkTiming(taskHandle, "", 31.25, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, samplesPerChannel);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to configure timing: " + std::string(errBuff));
         }
    }

    
    
    while (true)
    {
        // Start the task to begin measuring
         error = DAQmxStartTask(taskHandle);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to start task: " + std::string(errBuff));
         }
     
         // Prepare a buffer to read the samples into, sized for all channels and samples
         std::vector<double> dataBuffer(channelsCount * samplesPerChannel);
         // Read the samples from all channels into the buffer
         error = DAQmxReadAnalogF64(taskHandle, samplesPerChannel, timeout, DAQmx_Val_GroupByChannel, dataBuffer.data(), dataBuffer.size(), &read, NULL);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to read data: " + std::string(errBuff));
         }

        std::vector<double> averages(channelsCount, 0.0); 
        // Compute the average for each channel
        for (int i = 0; i < channelsCount; ++i) 
        {
            double sum = 0.0;
            // Sum all samples for the current channel
            for (int j = 0; j < samplesPerChannel; ++j) 
            {
                sum += dataBuffer[i * samplesPerChannel + j];
            }
            // Compute the average and store it in the vector
            averages[i] = sum / samplesPerChannel;
        }

        if (m_rollingWindowFilterActiv && (Mod1OldValuesBuffer.size()==Mod1Buffer.size()) && (Mod1OldValuesBuffer.size()>0))
        {
            for (int i = 0; i < channelsCount; ++i)
            {
                double v1 = averages[i];
                double v2 = Mod1OldValuesBuffer[i];
                averages[i] = (v1+v2)/2;
            }
        } 
        
        Mod1Buffer.restore(averages);


        if (m_rollingWindowFilterActiv) Mod1OldValuesBuffer = Mod1Buffer.copy();
        
        //std::cout<<std::endl<<result.c_str()<<std::endl;
        DAQmxStopTask(taskHandle);

    }
}

void QNiDaqWrapper::readMod2()
{

    // Hardcoded device name for Mod1 module
    const char* deviceName = "Mod2";
    // Initialize task handle to 0, which will be used to identify our task with DAQmx functions
    TaskHandle taskHandle = readCurrentMod2Task;
    // Error code returned by DAQmx functions
    int32 error;
    // Buffer for error information
    char errBuff[2048] = {'\0'};
    // Total number of channels to read from
    const int32 channelsCount = 16;
    // Specify the range of current to measure
    const float64 minRange = 0.004, maxRange = 0.02;
    // Timeout for DAQmxReadAnalogF64 function in seconds
    const float64 timeout = 1.0;
    // We will request 2 samples per channel to meet minimum sample requirement but only use the last sample
    const int32 samplesPerChannel = 7;
    // Number of samples actually read
    int32 read;

    // Construct full channel names for the device, spanning from /ai0 to /ai15
    std::string fullChannelNames = std::string(deviceName) + "/ai0:" + std::to_string(channelsCount-1);

    // Generate a unique task name using a helper function (assumed to be defined elsewhere)
    std::string unicKey = "ReadCurrentMod2" + generate_hex(8);
    // Create a task
    if (taskHandle == nullptr)
    {
         error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             throw std::runtime_error("Failed to create task: " + std::string(errBuff));
         }
     
         // Create analog input current channels for the specified range on the device
         error = DAQmxCreateAICurrentChan(taskHandle, fullChannelNames.c_str(), "", DAQmx_Val_Cfg_Default, minRange, maxRange, DAQmx_Val_Amps, DAQmx_Val_Default, 0.0, NULL);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to create channels: " + std::string(errBuff));
         }
     
         // Configure the sampling clock with a rate of 30 Hz, finite samples, and rising edge timing
         error = DAQmxCfgSampClkTiming(taskHandle, "", 31.25, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, samplesPerChannel);
         if (error) {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to configure timing: " + std::string(errBuff));
         }
    }

    
    
    while (true)
    {
        // Start the task to begin measuring
         error = DAQmxStartTask(taskHandle);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to start task: " + std::string(errBuff));
         }
     
         // Prepare a buffer to read the samples into, sized for all channels and samples
         std::vector<double> dataBuffer(channelsCount * samplesPerChannel);
         // Read the samples from all channels into the buffer
         error = DAQmxReadAnalogF64(taskHandle, samplesPerChannel, timeout, DAQmx_Val_GroupByChannel, dataBuffer.data(), dataBuffer.size(), &read, NULL);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to read data: " + std::string(errBuff));
         }

        std::vector<double> averages(channelsCount, 0.0); 
        // Compute the average for each channel
        for (int i = 0; i < channelsCount; ++i) 
        {
            double sum = 0.0;
            // Sum all samples for the current channel
            for (int j = 0; j < samplesPerChannel; ++j) 
            {
                sum += dataBuffer[i * samplesPerChannel + j];
            }
            // Compute the average and store it in the vector
            averages[i] = sum / samplesPerChannel;
        }

        if (m_rollingWindowFilterActiv && (Mod2OldValuesBuffer.size()==Mod2Buffer.size()) && (Mod2OldValuesBuffer.size()>0))
        {
            for (int i = 0; i < channelsCount; ++i)
            {
                double v1 = averages[i];
                double v2 = Mod2OldValuesBuffer[i];
                averages[i] = (v1+v2)/2;
            }
        } 
        
        Mod2Buffer.restore(averages);


        if (m_rollingWindowFilterActiv) Mod2OldValuesBuffer = Mod2Buffer.copy();
        
        //std::cout<<std::endl<<result.c_str()<<std::endl;
        DAQmxStopTask(taskHandle);

    }
}

//void QNiDaqWrapper::readMod3()
//{
//    // Hardcoded device name for Mod1 module
//    const char* deviceName = "Mod3";
//    // Initialize task handle to 0, which will be used to identify our task with DAQmx functions
//    TaskHandle taskHandle = readVoltageMod3Task;
//    // Error code returned by DAQmx functions
//    int32 error;
//    // Buffer for error information
//    char errBuff[2048] = {'\0'};
//    // Total number of channels to read from
//    const int32 channelsCount = 4;
//    // Specify the range of current to measure
//    const float64 minRange = 0.0, maxRange = 10.0;
//    // Timeout for DAQmxReadAnalogF64 function in seconds
//    const float64 timeout = 0.1;
//    // We will request 2 samples per channel to meet minimum sample requirement but only use the last sample
//    const int32 samplesPerChannel = 1;
//    // Vector to store the last sample from each channel
//    std::vector<double> lastSamples(channelsCount); 
//    int32 read;
//    // Construct full channel names for the device, spanning from /ai0 to /ai15
//    std::string fullChannelNames = std::string(deviceName) + "/ai0:" + std::to_string(channelsCount-1);
//
//    // Generate a unique task name using a helper function (assumed to be defined elsewhere)
//    std::string unicKey = "ReadVoltageMod3" + generate_hex(8);
//    // Create a task
//    if (taskHandle == nullptr)
//    {
//         error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
//         if (error) {
//             DAQmxGetExtendedErrorInfo(errBuff, 2048);
//             throw std::runtime_error("Failed to create task: " + std::string(errBuff));
//         }
//     
//
//            // Create an analog input voltage channel
//        error = DAQmxCreateAIVoltageChan(taskHandle,
//                                         fullChannelNames.c_str(),
//                                         "",
//                                         differencial,
//                                         minRange,
//                                         maxRange,
//                                         Val_Volts,
//                                         NULL);
//
//
//         if (error) {
//             DAQmxGetExtendedErrorInfo(errBuff, 2048);
//             DAQmxClearTask(taskHandle);
//             throw std::runtime_error("Failed to create channels: " + std::string(errBuff));
//         }
//     
//         // Configure the sampling clock with a rate of 1000 Hz, finite samples, and rising edge timing
//        // Add sample clock timing configuration (NEW)
//        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
//        if (error) 
//        {
//            DAQmxGetExtendedErrorInfo(errBuff, 2048);
//            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                       "In\n"
//                                       "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
//                                       "Error: Failed to set sample clock timing.\n"+
//                                       std::string(errBuff));
//            handleErrorAndCleanTask(taskHandle);
//            throw std::runtime_error("Failed to set sample clock timing.");
//        }
//
//    }
//    
//    while (true)
//    {
//        // Start the task to begin measuring
//         error = DAQmxStartTask(taskHandle);
//         if (error) 
//         {
//             DAQmxGetExtendedErrorInfo(errBuff, 2048);
//             DAQmxClearTask(taskHandle);
//             throw std::runtime_error("Failed to start task: " + std::string(errBuff));
//         }
//     
//         // Prepare a buffer to read the samples into, sized for all channels and samples
//         std::vector<double> dataBuffer(channelsCount * samplesPerChannel);
//         // Read the samples from all channels into the buffer
//         error = DAQmxReadAnalogF64(taskHandle, samplesPerChannel, timeout, DAQmx_Val_GroupByChannel, dataBuffer.data(), dataBuffer.size(), &read, NULL);
//         if (error) 
//         {
//             DAQmxGetExtendedErrorInfo(errBuff, 2048);
//             DAQmxClearTask(taskHandle);
//             throw std::runtime_error("Failed to read data: " + std::string(errBuff));
//         }
//     
//
//        Mod3Buffer.restore(dataBuffer);
//        DAQmxStopTask(taskHandle);
//    }
//}

void QNiDaqWrapper::initMod3(const std::string& deviceName, double minRange, double maxRange, int32 samplesPerChannel, double samplingRate, int32 channelsCount) 
{
    char errBuff[2048] = {'\0'};
    int32 error;


    // Create a task
     error = DAQmxCreateTask("ReadVoltageMod3", &readVoltageMod3Task);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        throw std::runtime_error("Failed to create task: " + std::string(errBuff));
    }

    // Construct full channel names for the device
    std::string fullChannelNames = deviceName + "/ai0:" + std::to_string(channelsCount - 1);

    // Create analog input voltage channels
    error = DAQmxCreateAIVoltageChan(readVoltageMod3Task, fullChannelNames.c_str(), "",
                                         DAQmx_Val_Diff, minRange, maxRange, DAQmx_Val_Volts, NULL);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        DAQmxClearTask(readVoltageMod3Task);
        throw std::runtime_error("Failed to create channels: " + std::string(errBuff));
    }

        // Configure the sampling clock
    error = DAQmxCfgSampClkTiming(readVoltageMod3Task, "", samplingRate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, samplesPerChannel);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        DAQmxClearTask(readVoltageMod3Task);
        throw std::runtime_error("Failed to set sample clock timing: " + std::string(errBuff));
    } 
    else 
    {
        std::cout << "OverSampling hack applied with success." << std::endl;
    }
    
}

void QNiDaqWrapper::averageWindow(std::vector<double> &averages, const std::vector<double> &oldValues)
{
    for (size_t i = 0; i < averages.size(); ++i) 
    {
        averages[i] = (averages[i] + oldValues[i]) / 2.0;
    }
}

std::vector<double> QNiDaqWrapper::readMod3Samples(int32 channelsCount, int32 samplesPerChannel, float64 timeOut, bool &inError) 
{
    inError = false;
    // Start the task to begin measuring
    int32 error = DAQmxStartTask(readVoltageMod3Task);
    if (error) 
    {
        inError = true;
        char errBuff[2048];
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        DAQmxClearTask(readVoltageMod3Task);
        throw std::runtime_error("Failed to start task: " + std::string(errBuff));
    }

    const int32 totalSamples = channelsCount * samplesPerChannel;
    std::vector<double> dataBuffer(totalSamples);

    // Read the samples from all channels into the buffer
    error = DAQmxReadAnalogF64(readVoltageMod3Task, samplesPerChannel, timeOut, DAQmx_Val_GroupByChannel, dataBuffer.data(), totalSamples, nullptr, nullptr);
    if (error) 
    {
        inError = true;
        char errBuff[2048];
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        DAQmxClearTask(readVoltageMod3Task);
        throw std::runtime_error("Failed to read data: " + std::string(errBuff));
    }

    // Stop the task once the data is read
    DAQmxStopTask(readVoltageMod3Task);

    return dataBuffer;
}

void QNiDaqWrapper::applyMod3LowPassFilter(const int32 channelsCount, const int32 samplesPerChannel, std::vector<double> &dataBuffer, std::vector<double> &averages, float deltaTime)
{
     std::cout<<"apply low pass filter"<<std::endl;
    // Filter the data for each channel
    std::vector<double> filteredData(channelsCount * samplesPerChannel);
    for (int channel = 0; channel < channelsCount; ++channel) 
    {
        // Extract samples for the current channel
        std::vector<double> channelSamples(samplesPerChannel);
        for (int sample = 0; sample < samplesPerChannel; ++sample) 
        {
            channelSamples[sample] = dataBuffer[channel * samplesPerChannel + sample];
        }
        // Apply the low-pass filter to the samples of the current channel
        std::vector<double> filteredChannelSamples = lowPassFilterDatas(channelSamples, deltaTime, m_cutOffFrequency);
        // Store the filtered samples back into the filteredData vector
        for (int sample = 0; sample < samplesPerChannel; ++sample) 
        {
            filteredData[channel * samplesPerChannel + sample] = filteredChannelSamples[sample];
        }
    }
    
    for (int i = 0; i < channelsCount; ++i) 
    {
        double sum = 0.0;
        for (int j = 0; j < samplesPerChannel; ++j) 
        {
            sum += filteredData[i * samplesPerChannel + j];
        }
        averages[i] = roundToNbSignificativDigits ((sum / samplesPerChannel),4);
    }


}

void QNiDaqWrapper::readMod3()
{
    auto lastCycleTime = std::chrono::steady_clock::now(); 

    const char* deviceName = "Mod3";
    TaskHandle taskHandle = readVoltageMod3Task;
    const int32 channelsCount = 4;
    const float64 minRange = 0.0, maxRange = 10.0;
    const float64 timeout = 2.0; // Adjusted for longer reads
    const int32 samplesPerChannel = 5581; // Number of samples to average, 5581 is prime, so it may help to filter both 50 hz and 60 hz power lines noise
    std::vector<double> averages(channelsCount);
    const float samplingRate = 50000.0f;

    std::string fullChannelNames = std::string(deviceName) + "/ai0:" + std::to_string(channelsCount-1);

    std::string unicKey = "ReadVoltageMod3" + generate_hex(8);
    if (taskHandle == nullptr)
    {
        initMod3(deviceName, minRange, maxRange, samplesPerChannel, samplingRate, channelsCount); 
    }
    
    while (true)
    {

           // Start measuring time
           auto currentCycleTime = std::chrono::steady_clock::now();
           auto start            = std::chrono::high_resolution_clock::now();
           bool inError;
           //Fill the raw data buffer with readed values
           std::vector<double> dataBuffer = readMod3Samples(channelsCount,samplesPerChannel,timeout,inError);
           if (!inError)
           {
                //post treatment
                float deltaTime = std::chrono::duration<float>(currentCycleTime - lastCycleTime).count();
                lastCycleTime = currentCycleTime;
                
                if (m_lowPassFilterActiv)
                {
                    std::cout<<"apply low pass filter"<<std::endl;
                    applyMod3LowPassFilter(channelsCount,samplesPerChannel,dataBuffer,averages, deltaTime);
              
                }
                else
                {
                    // Calculate averages for each channel this is where the oversampling results are made
                    for (int i = 0; i < channelsCount; ++i) 
                    {
                        double sum = 0.0;
                        for (int j = 0; j < samplesPerChannel; ++j) 
                        {
                            sum += dataBuffer[i * samplesPerChannel + j];
                        }
                        averages[i] = roundToNbSignificativDigits ((sum / samplesPerChannel),4);
                    }
                }
           
            if (m_rollingWindowFilterActiv && (Mod3OldValuesBuffer.size()==Mod3Buffer.size()) && (Mod3OldValuesBuffer.size()>0))
            {
                //this is a 2 point floating window average
                averageWindow(averages,Mod3OldValuesBuffer);
            } 
        


            if (m_rollingWindowFilterActiv) Mod3OldValuesBuffer = Mod3Buffer.copy();

            // Use the averages to update the buffer instead
            Mod3Buffer.restore(averages);
            DAQmxStopTask(taskHandle);
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            std::cout << "Duration: " << duration.count() << "ms" << std::endl;
        }
    }
    
}



void QNiDaqWrapper::readMod4()
{
    // Hardcoded device name for Mod1 module
    const char* deviceName = "Mod4";
    // Initialize task handle to 0, which will be used to identify our task with DAQmx functions
    TaskHandle taskHandle = readCounterMod4Task;
    // Error code returned by DAQmx functions
    int32 error;
    // Buffer for error information
    char errBuff[2048] = {'\0'};
    // Total number of channels to read from
    const int32 countersCount = 4;
    // Timeout for DAQmxReadAnalogF64 function in seconds
    const float64 timeout = 0.1;
    // We will request 2 samples per channel to meet minimum sample requirement but only use the last sample
    const int32 samplesPerChannel = 1;
    // Vector to store the last sample from each channel
    std::vector<double> lastSamples(countersCount); 
    int32 read;
    // Construct full channel names for the device, spanning from /ai0 to /ai15
    std::string fullChannelNames = std::string(deviceName) + "/ctr0:" + std::to_string(countersCount-1);

    // Generate a unique task name using a helper function (assumed to be defined elsewhere)
    std::string unicKey = "ReadCounterMod4" + generate_hex(8);
    // Create a task
    if (taskHandle == nullptr)
    {
         error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             throw std::runtime_error("Failed to create task: " + std::string(errBuff));
         }
     

        // Create counters channels
        error = DAQmxCreateCICountEdgesChan(taskHandle, fullChannelNames.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) 
        {
            std::cout<<"in read counter Failed to create counter channel: "<<fullChannelNames.c_str()<<std::endl;
            char errBuff[2048];
            DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "void QNiDaqWrapper::readMod4()\n"
                                       "Failed to create counter channel. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Ensure the map entry is reset after cleanup
            throw std::runtime_error("Failed to create counter channel.");
        }

         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to create channels: " + std::string(errBuff));
         }
     
         // Configure the sampling clock with a rate of 1000 Hz, finite samples, and rising edge timing
        // Add sample clock timing configuration (NEW)
        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                       "Error: Failed to set sample clock timing.\n"+
                                       std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to set sample clock timing.");
        }

    }
    
    while (true)
    {
        // Start the task to begin measuring
         error = DAQmxStartTask(taskHandle);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to start task: " + std::string(errBuff));
         }
     
         // Prepare a buffer to read the samples into, sized for all channels and samples
         std::vector<uInt32> dataBuffer(countersCount * samplesPerChannel);
         // Read the samples from all channels into the buffer
         error = DAQmxReadCounterU32(taskHandle, samplesPerChannel, timeout, dataBuffer.data(), static_cast<uInt32>(dataBuffer.size()), &read, NULL);
         if (error) 
         {
             DAQmxGetExtendedErrorInfo(errBuff, 2048);
             DAQmxClearTask(taskHandle);
             throw std::runtime_error("Failed to read data: " + std::string(errBuff));
         }


        Mod4Buffer.restore(dataBuffer);
        DAQmxStopTask(taskHandle);
    }
}

double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)
{
    std::lock_guard<std::mutex> lock(currentMutex); // Ensure thread safety
    TaskHandle taskHandle = 0;
    // Check for null pointer
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Null pointer passed for deviceModule.");
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    int32 error;
    float64 readValue;

    // Extract necessary information from NIDeviceModule
    const char* deviceName = deviceModule->getAlias().c_str();
    moduleShuntLocation shuntLoc = deviceModule->getModuleShuntLocation();
    if (shuntLoc == noShunt) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: current modules must have a shunt.");
        throw std::runtime_error("Failed current modules must have a shunt");
    }
    float64 shuntVal = deviceModule->getModuleShuntValue();
    double minRange = deviceModule->getChanMin();
    double maxRange = deviceModule->getChanMax();
    int32 termCfg = deviceModule->getModuleTerminalCfg();
    int32 unit = deviceModule->getModuleUnit();

    // Construct full channel name using the channel index
    const char* channelName = deviceModule->getChanNames()[chanIndex].c_str();
    std::string fullChannelName = std::string(deviceName) + std::string(channelName);

    unsigned int retryCount = 0;
    char errBuff[2048] = {'\0'};
    while (true) 
    {
        // Create a new task
        std::string unicKey = "getCurrentValue" + generate_hex(8);
        error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                       "Error: Failed to create task for reading current.\n"+
                                       std::string(errBuff));

            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to create task for reading current.");
        }

        // Register the Done callback
        error = DAQmxRegisterDoneEvent(taskHandle, 0, CurrentDoneCallback, this);
        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                       "Error: Failed to register Done callback.\n"+
                                        std::string(errBuff));
    
            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to register Done callback.");
        }

        // Create an analog input current channel
        error = DAQmxCreateAICurrentChan(taskHandle, 
                                         fullChannelName.c_str(), 
                                         "", 
                                         termCfg, 
                                         minRange, 
                                         maxRange, 
                                         unit, 
                                         shuntLoc, 
                                         shuntVal, 
                                         NULL);

        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            if (++retryCount >= maxRetries) 
            {
                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                           "In\n"
                                           "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                           "Error: Failed to create channel after max retries.");
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                       "Error: Failed to create channel fore read current\n"+
                                       std::string(errBuff));

            handleErrorAndCleanTask(taskHandle);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        break;
    }

    // Start the task
    error = DAQmxStartTask(taskHandle);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to start task for reading current.\n"+
                                   std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to start task for reading current.");
    }

    // Read the current value
    error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to read current value.\n"+
                                   std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to read current value.");
    }

    // Stop and clear the task
    error = DAQmxStopTask(taskHandle);
    if (error)
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to stop task.\n"+
                                   std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to stop task.");
    }
    DAQmxClearTask(taskHandle);

   
   // Convert the read value to the appropriate unit (if necessary) and return it
   double result = static_cast<double>(readValue);
   if (autoConvertTomAmps) {
       result = ampsTomAmps(result); // Convert amperes to milliamperes if required
   }
   setLastSingleCurrentChannelValue(result); // Store the latest value
   
   return result; // Return the final result
}

// PATCH CHANTIER BEGIN:

          //double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)
          //{
          //    std::lock_guard<std::mutex> lock(voltageMutex); // Ensure thread safety
          //
          //    // Check for null pointer
          //    if (!deviceModule) 
          //    {
          //        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                   "In\n"
          //                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                   "Error: Null pointer passed for deviceModule.");
          //        throw std::invalid_argument("Null pointer passed for deviceModule.");
          //    }
          //
          //    int32 error;
          //    float64 readValue;
          //    TaskHandle taskHandle = 0;
          //
          //    const char* deviceName = deviceModule->getAlias().c_str    ();
          //    double minRange        = deviceModule->getChanMin          ();
          //    double maxRange        = deviceModule->getChanMax          ();
          //    int32 termCfg          = deviceModule->getModuleTerminalCfg();
          //    int32 unit             = deviceModule->getModuleUnit       ();
          //
          //    // Construct the full channel name
          //    const char* channelName = chanName.c_str();
          //    std::string fullChannelName = std::string(deviceName) + channelName;
          //
          //    unsigned int retryCount = 0;
          //    char errBuff[2048] = {'\0'};
          //    while (true) 
          //    {
          //        // Generate a unique task key
          //        std::string uniqueKey = "getVoltageValue" + generate_hex(8);
          //        error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
          //        if (error) 
          //        {
          //            DAQmxGetExtendedErrorInfo(errBuff, 2048);
          //            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                        "In\n"
          //                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                        "Error: Failed to create task for reading voltage.\n"+
          //                                        std::string(errBuff));
          //            
          //            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
          //            throw std::runtime_error("Failed to create task for reading voltage.");
          //        }
          //
          //        error = DAQmxRegisterDoneEvent(taskHandle, 0, VoltageDoneCallback, this);
          //        if (error) 
          //        {
          //            DAQmxGetExtendedErrorInfo(errBuff, 2048);
          //            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                        "In\n"
          //                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                        "Error: Failed to register Done callback.\n"+
          //                                        std::string(errBuff));
          //            
          //            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
          //            throw std::runtime_error("Failed to register Done callback.");
          //        }
          //
          //        // Create an analog input voltage channel
          //        error = DAQmxCreateAIVoltageChan(taskHandle,
          //                                         fullChannelName.c_str(),
          //                                         "",
          //                                         termCfg,
          //                                         minRange,
          //                                         maxRange,
          //                                         unit,
          //                                         NULL);
          //        if (error)
          //        {
          //            
          //            if (++retryCount >= maxRetries) 
          //            {
          //                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                   "In\n"
          //                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                   "Error: Failed to create channel after max retries."); 
          //                throw std::runtime_error("Failed to create channel after max retries.");
          //            }
          //            DAQmxGetExtendedErrorInfo(errBuff, 2048);
          //            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                       "In\n"
          //                                       "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                       "Error: Failed to create channel:\n"+
          //                                       fullChannelName+"\n"+ 
          //                                       std::string(errBuff));
          //            handleErrorAndCleanTask(taskHandle);
          //            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          //            continue;
          //        }
          //
          //        // Add sample clock timing configuration (NEW)
          //        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
          //        if (error) 
          //        {
          //            DAQmxGetExtendedErrorInfo(errBuff, 2048);
          //            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                       "In\n"
          //                                       "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                       "Error: Failed to set sample clock timing.\n"+
          //                                       std::string(errBuff));
          //            handleErrorAndCleanTask(taskHandle);
          //            throw std::runtime_error("Failed to set sample clock timing.");
          //        }
          //
          //        // Start the task
          //        error = DAQmxStartTask(taskHandle);
          //        if (error) 
          //        {
          //            DAQmxGetExtendedErrorInfo(errBuff, 2048);
          //            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                       "In\n"
          //                                       "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                       "Error: Failed to start task for reading voltage.\n"+
          //                                       std::string(errBuff));
          //            handleErrorAndCleanTask(taskHandle);
          //            throw std::runtime_error("Failed to start task for reading voltage.");
          //        }
          //
          //        // Read a voltage value
          //        error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
          //        if (error) 
          //        {
          //            DAQmxGetExtendedErrorInfo(errBuff, 2048);
          //            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                       "In\n"
          //                                       "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                       "Error: Failed to read voltage value.\n"+
          //                                       std::string(errBuff));
          //            handleErrorAndCleanTask(taskHandle);
          //            throw std::runtime_error("Failed to read voltage value.");
          //        }
          //
          //        // Stop the task
          //        error = DAQmxStopTask(taskHandle);
          //        if (error) {
          //            DAQmxGetExtendedErrorInfo(errBuff, 2048);
          //            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
          //                                       "In\n"
          //                                       "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
          //                                       "Error: Failed to stop task.\n"+
          //                                       std::string(errBuff));
          //            handleErrorAndCleanTask(taskHandle);
          //            throw std::runtime_error("Failed to stop task.");
          //        }
          //
          //        // Clear the task to free up resources
          //        DAQmxClearTask(taskHandle);
          //        break;
          //    }
          //
          //    // Convert to appropriate unit (if necessary)
          //    double result = static_cast<double>(readValue);
          //    setLastSingleVoltageChannelValue(result); // Assuming this is a custom function
          //
          //    return result; // Return the read value
          //}

double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)
{
    const int32 numSamples = 1600; // Define the number of samples to average
    std::lock_guard<std::mutex> lock(voltageMutex); // Lock for thread safety

    // Validate the deviceModule pointer
    if (!deviceModule)
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "Error: Null pointer passed for deviceModule.");
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    // Initialization of variables used in the function
    int32 error;
    TaskHandle taskHandle = 0;
    const char* deviceName = deviceModule->getAlias().c_str();
    double minRange = deviceModule->getChanMin();
    double maxRange = deviceModule->getChanMax();
    int32 termCfg = deviceModule->getModuleTerminalCfg();
    int32 unit = deviceModule->getModuleUnit();
    std::string fullChannelName = std::string(deviceName) + chanName;
    unsigned int retryCount = 0;
    char errBuff[2048] = {'\0'};

    while (retryCount < maxRetries)
    {
        std::string uniqueKey = "getVoltageValue" + generate_hex(8); // Unique task key for each attempt
        error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
        if (error)
        {
            // Log error and cleanup if task creation fails
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "Error: Failed to create task for reading voltage. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to create task for reading voltage.");
        }

        // Task setup and start
        error = DAQmxStartTask(taskHandle);
        if (error)
        {
            // Log error and cleanup if starting the task fails
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "Error: Failed to start task for reading voltage. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to start task for reading voltage.");
        }

        // Configuration of the sample clock timing for finite samples
        error = DAQmxCfgSampClkTiming(taskHandle, "", 20000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, numSamples);
        if (error)
        {
            // Log error and cleanup if configuring the sample clock timing fails
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "Error: Failed to configure sample clock timing. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to configure sample clock timing.");
        }

        float64 data[numSamples]; // Array to store readings
        int32 read; // Variable to store the number of samples actually read

        // Read operation for multiple samples
        error = DAQmxReadAnalogF64(taskHandle, numSamples, 0.5, DAQmx_Val_GroupByChannel, data, numSamples, &read, nullptr);
        if (error)
        {
            // Log error and cleanup if reading the samples fails
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "Error: Failed to read voltage values. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to read voltage values.");
        }

        // Compute the average of the read samples
        std::cout<<std::endl<<"nb samples:"<<std::to_string(read)<<std::endl;
        double sum = 0;
        for (int i = 0; i < read; ++i)
        {
            sum += data[i];
        }
        double average = sum / read; // Ensure division by the actual number of read samples

        // Stop and clear the task after successful read operation
        DAQmxStopTask(taskHandle);
        DAQmxClearTask(taskHandle);

        // Update and return the average voltage value
        setLastSingleVoltageChannelValue(average); // Assuming this updates the last read voltage value
        return average;
    }

    // Throw an error if the function exits the loop without reading successfully
    throw std::runtime_error("Failed to read voltage after maximum retries.");
}

// ------------------  PATCH CHANTIER END -------------------------------------

double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)
{
    std::lock_guard<std::mutex> lock(voltageMutex); // Ensure thread safety

    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Null pointer passed for deviceModule.");
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    int32 error;
    float64 readValue;
    TaskHandle taskHandle = 0;

    const char* deviceName = deviceModule->getAlias().c_str();
    double minRange = deviceModule->getChanMin();
    double maxRange = deviceModule->getChanMax();
    int32 termCfg = deviceModule->getModuleTerminalCfg();
    int32 unit = deviceModule->getModuleUnit();

    // Extract channelName with its index
    const char* channelName = deviceModule->getChanNames()[chanIndex].c_str();
    std::string fullChannelName = std::string(deviceName) + channelName;

    unsigned int retryCount = 0;
    char errBuff[2048] = {'\0'};
    while (true) {
        std::string unicKey = "getVoltageValue" + generate_hex(8);
        error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                        "Error: Failed to create task for reading voltage.\n"+
                                        std::string(errBuff));
            
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to create task for reading voltage.");
        }

        error = DAQmxRegisterDoneEvent(taskHandle, 0, VoltageDoneCallback, this);
        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                        "Failed to register Done callback.\n"+
                                        std::string(errBuff));
            
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to register Done callback.");
        }

        error = DAQmxCreateAIVoltageChan(taskHandle,
                                         fullChannelName.c_str(),
                                         "",
                                         termCfg,
                                         minRange,
                                         maxRange,
                                         unit,
                                         NULL);
        if (error)
        {
            if (++retryCount >= maxRetries) 
            {
                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to create channel after max retries.");
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                        "Error: Failed to register Done callback.\n"+
                                        std::string(errBuff));
            
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
        if (error) {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                        "Error: Failed to set sample clock timing.\n"+
                                        std::string(errBuff));
            
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to set sample clock timing.");
        }

        error = DAQmxStartTask(taskHandle);
        if (error) {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                        "Error: Failed to start task for reading voltage.\n"+
                                        std::string(errBuff));
            
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to start task for reading voltage.");
        }

        error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
        if (error) 
        {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                        "Error: Failed to read voltage value.\n"+
                                        std::string(errBuff));
            
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to read voltage value.");
        }

        error = DAQmxStopTask(taskHandle);
        if (error) {
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                        "In\n"
                                        "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                        "Error: Failed to stop task.\n"+
                                        std::string(errBuff));
            
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to stop task.");
        }

        // Clear the task to free up resources
        DAQmxClearTask(taskHandle);
        break;
    }

    // Convert to appropriate unit (if necessary)
    double result = static_cast<double>(readValue);
    setLastSingleVoltageChannelValue(result); // Assuming this is a custom function

    return result; // Return the read value
}

void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const unsigned int &index) 
{
    std::lock_guard<std::mutex> lock(countersMutex);
    if (!deviceModule) {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const unsigned int &index)\n"
                                   "Error: Null pointer passed for deviceModule.");
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    const std::string &channelName = deviceModule->getChanNames().at(index);
    if (channelName.empty()) {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const unsigned int &index)\n"
                                   "Error: Channel name is empty.");
        throw std::runtime_error("Channel name is empty.");
    }

    std::string fullChannelName = deviceModule->getAlias() + "/" + channelName;

    // Check if a task for this channel exists in the map
    auto taskIter = counterTasksMap.find(fullChannelName);
    if (taskIter != counterTasksMap.end()) {
        // If task exists, clear it to reset the counter
        DAQmxClearTask(taskIter->second);
        counterTasksMap.erase(taskIter); // Remove the task from the map
    } else {
        // If no task exists for this channel, create and then immediately clear it to reset the counter
        TaskHandle counterTask = 0;
        std::string uniqueKey = "resetCounter" + generate_hex(8);
        int32 error = DAQmxCreateTask(uniqueKey.c_str(), &counterTask);
        if (error) {
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const unsigned int &index)\n"
                                       "Error creating DAQmx task: " + std::string(errBuff));
            throw std::runtime_error("Failed to create task for resetting counter.");
        }

        error = DAQmxCreateCICountEdgesChan(counterTask, fullChannelName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) {
            DAQmxClearTask(counterTask); // Ensure task is cleared even if channel creation fails
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, the unsigned int &index)\n"
                                       "Error creating counter input channel: " + std::string(errBuff));
            throw std::runtime_error("Failed to create counter input channel.");
        }

        // Clear the task to reset the counter
        DAQmxClearTask(counterTask);
    }
}



void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const std::string &chanName) 
{
    std::lock_guard<std::mutex> lock(countersMutex);
    if (!deviceModule) {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const std::string &chanName)\n"
                                   "Error: deviceModule is null.");
        throw std::invalid_argument("deviceModule is null.");
    }

    if (chanName.empty()) {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const std::string &chanName)\n"
                                   "Error: Channel name is empty.");
        throw std::invalid_argument("Channel name is empty.");
    }

    std::string fullChanName = deviceModule->getAlias() + "/" + chanName;

    // Check if a task for this channel exists in the map
    auto taskIter = counterTasksMap.find(fullChanName);
    if (taskIter != counterTasksMap.end()) {
        // If task exists, clear it to reset the counter
        DAQmxClearTask(taskIter->second);
        counterTasksMap.erase(taskIter); // Remove the task from the map
    } else {
        // If no task exists for this channel, create and then immediately clear it to reset the counter
        TaskHandle counterTask = 0;
        std::string uniqueKey = "resetCounter" + generate_hex(8);
        int32 error = DAQmxCreateTask(uniqueKey.c_str(), &counterTask);
        if (error) {
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const std::string &chanName)\n"
                                       "Error creating DAQmx task: " + std::string(errBuff));
            throw std::runtime_error("Failed to create task for resetting counter.");
        }

        error = DAQmxCreateCICountEdgesChan(counterTask, fullChanName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) {
            DAQmxClearTask(counterTask); // Ensure task is cleared even if channel creation fails
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "void QNiDaqWrapper::resetCounter(NIDeviceModule *deviceModule, const std::string &chanName)\n"
                                       "Error creating counter input channel: " + std::string(errBuff));
            throw std::runtime_error("Failed to create counter input channel.");
        }

        // Clear the task to reset the counter
        DAQmxClearTask(counterTask);
    }
}




unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries) 
{
    std::lock_guard<std::mutex> lock(countersMutex);
    if (!deviceModule) {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Null pointer passed for deviceModule.");
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    const std::string deviceName = deviceModule->getAlias();
    const std::string channelName = deviceModule->getChanNames()[chanIndex];
    const std::string fullChannelName = deviceName + "/" + channelName;

    // Initialize the task handle if not already done
    TaskHandle& taskHandle = counterTasksMap[fullChannelName]; // Reference to the task handle in the map
    if (taskHandle == nullptr) { // If no task exists for this channel, create one
        std::string uniqueKey = "readCounter" + generate_hex(8);
        int32 error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);

        if (error) {
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                       "Error creating DAQmx task: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Reset task handle in map to nullptr after cleanup
            throw std::runtime_error("Failed to create DAQmx task.");
        }

        error = DAQmxCreateCICountEdgesChan(taskHandle, fullChannelName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) {
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                       "Error creating counter input channel: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Reset task handle in map to nullptr after cleanup
            throw std::runtime_error("Failed to create counter input channel.");
        }

        // It is assumed that starting the task is not necessary every time for continuous measurement,
        // in fact it would result in an unwanted counter reset 
    }

    // Attempt to read the counter value, retrying as necessary
    uInt32 readValue = 0;
    unsigned int retryCount = 0;
    char errBuff[2048] = {'\0'};
    while (retryCount < maxRetries) 
    {
        int32 error = DAQmxReadCounterScalarU32(taskHandle, 10.0, &readValue, nullptr);
        if (error) {
            
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                       "Error reading counter value: " + std::string(errBuff));
            retryCount++;
            continue; // Retry reading
        } else {
            break; // Successful read
        }
    }

    if (retryCount >= maxRetries) {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to read counter after maximum retries.");
        throw std::runtime_error("Failed to read counter after maximum retries.");
    }

    // Update the last known counter value
    setLastSingleCounterValue(readValue);

    // Emit signal or log the read value
    appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                               "Counter read successfully: Value = " + std::to_string(readValue));

    return readValue; // Return the read counter value
}



unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName) 
{
    std::lock_guard<std::mutex> lock(countersMutex);
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: deviceModule is null.");
        throw std::invalid_argument("readCounter: deviceModule is null.");
    }

    int index = extractNumberFromEnd(chanName);
    //const std::string fullChannelName = deviceModule->getAlias() + chanName;
    const std::string fullChannelName = deviceModule->getAlias() + "/ctr"+std::to_string(index);


    // Check if a task for this channel already exists, create if not
    TaskHandle& taskHandle = counterTasksMap[chanName];
    if (taskHandle == nullptr) 
    {
       
        std::string uniqueKey = "readCounter" + generate_hex(8); // Generate a unique key for the task
        int32 error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);

        if (error) 
        {
            char errBuff[2048];
            DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                       "Error: Failed to create DAQmx task. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Ensure the map entry is reset after cleanup
            throw std::runtime_error("Failed to create DAQmx task.");
        }


        std::cout<<"Create channel : "<<fullChannelName.c_str()<<std::endl;
        error = DAQmxCreateCICountEdgesChan(taskHandle, fullChannelName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) 
        {
            char errBuff[2048];
            DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
            std::cout<<"in read counter Failed to create counter channel: "<<fullChannelName.c_str()<<std::endl<<errBuff<<std::endl;
            
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                       "Failed to create counter channel. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Ensure the map entry is reset after cleanup
            throw std::runtime_error("Failed to create counter channel.");
        }

        // After creating the counter channel we link it to a front source
        error = DAQmxSetCICountEdgesTerm(taskHandle, fullChannelName.c_str(), chanName.c_str());
        if (error) 
        {   
            char errBuff[2048];
            DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
            std::cout<<"in read counter Failed to link counter channel to front source: "<<fullChannelName.c_str()<<" "<<chanName.c_str()<<std::endl<<errBuff<<std::endl;
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                       "in read counter Failed to link counter channel to front source:\n"+
                                       fullChannelName+"\n"+
                                       chanName+"\n"+
                                       std::string(errBuff));
        }


        // Start the task.
        error = DAQmxStartTask(taskHandle);
        if (error) 
        {
            std::cout<<"in read counter Failed to start counter task."<<std::endl;
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                       "Failed to start counter task.");
            this->handleErrorAndCleanTask(taskHandle);
            throw std::runtime_error("Failed to start counter task.");
        }

    }
    uInt32 readValue = 0;
    int32 error;
    error = DAQmxReadCounterScalarU32(taskHandle, 10.0, &readValue, nullptr);
    if (error) 
    {
        std::cout<<"in read counter DAQmxReadCounterScalarU32 Failed to read counter value"<<std::endl;
        char errBuff[2048];
        DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to read counter value. Error: " + std::string(errBuff));

    }
    return readValue; // Return the successfully read value
}


/*unsigned int QNiDaqWrapper::testReadCounter()
{
    const std::string fullChannelName = "Mod4/ctr0";
    int32 error;
    uInt32 readValue = 0; // Initialize to 0 or an appropriate default value.

    if (this->counterHandle == nullptr) 
    {
        this->counterHandle = new TaskHandle(0); // Proper initialization of TaskHandle.
        std::string uniqueKey = "readCounter" + this->generate_hex(8); // Assuming generate_hex is a method to generate a hexadecimal string.
        error = DAQmxCreateTask(uniqueKey.c_str(), &(this->counterHandle));
        if (error) {
            this->handleErrorAndCleanTask(this->counterHandle); // Assuming this method properly cleans up and sets counterHandle to nullptr.
            throw std::runtime_error("Failed to create DAQmx task.");
        }
    
        // Create the counter channel. Assuming this setup should be done each time the function is called.
        error = DAQmxCreateCICountEdgesChan(this->counterHandle, fullChannelName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) 
        {
            this->handleErrorAndCleanTask(this->counterHandle);
            throw std::runtime_error("Failed to create counter channel.");
        }

        // Start the task.
        error = DAQmxStartTask(this->counterHandle);
        if (error) 
        {
            this->handleErrorAndCleanTask(this->counterHandle);
            throw std::runtime_error("Failed to start counter task.");
        }
    
    }

    // Attempt to read the counter value.
    error = DAQmxReadCounterScalarU32(this->counterHandle, 10.0, &readValue, nullptr);
    if (error) {
        this->handleErrorAndCleanTask(this->counterHandle);
        throw std::runtime_error("Failed to read counter value.");
    }

    return readValue;
}*/


unsigned int QNiDaqWrapper::testReadCounter1()
{
    // Adjust the fullChannelName to specify using PFI2 for counting.
    const std::string fullChannelName = "Mod4/ctr0"; // Example, adjust "Dev1" to your actual device name.
    int32 error;
    uInt32 readValue = 0; // Initialize to 0 or an appropriate default value.

    if (this->counterHandle1 == nullptr) 
    {
        this->counterHandle1 = new TaskHandle(0); // Proper initialization of TaskHandle.
        std::string uniqueKey = "readCounter" + this->generate_hex(8);
        error = DAQmxCreateTask(uniqueKey.c_str(), &(this->counterHandle1));
        if (error) {
            this->handleErrorAndCleanTask(this->counterHandle1);
            throw std::runtime_error("Failed to create DAQmx task.");
        }
        


        // Adjust the call to create the counter channel for PFI2.
        error = DAQmxCreateCICountEdgesChan(this->counterHandle1, fullChannelName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) 
        {
            this->handleErrorAndCleanTask(this->counterHandle1);
            throw std::runtime_error("Failed to create counter channel.");
        }

        // After creating the counter channel
        error = DAQmxSetCICountEdgesTerm(this->counterHandle1, fullChannelName.c_str(), "PFI0");
        if (error) {
            // Handle error 
        }

        // Start the task.
         error = DAQmxStartTask(this->counterHandle1);
        if (error) 
        {
            this->handleErrorAndCleanTask(this->counterHandle1);
            throw std::runtime_error("Failed to start counter task.");
        }

    }


    // Attempt to read the counter value.
    error = DAQmxReadCounterScalarU32(this->counterHandle1, 10.0, &readValue, nullptr);
    if (error) {
        this->handleErrorAndCleanTask(this->counterHandle1);
        throw std::runtime_error("Failed to read counter value.");
    }
    return readValue;
}

unsigned int QNiDaqWrapper::testReadCounter2()
{
    // Adjust the fullChannelName to specify using PFI2 for counting.
    const std::string fullChannelName = "Mod4/ctr1"; 
    int32 error;
    uInt32 readValue = 0; // Initialize to 0 or an appropriate default value.

    if (this->counterHandle2 == nullptr) 
    {
        this->counterHandle2 = new TaskHandle(0); // Proper initialization of TaskHandle.
        std::string uniqueKey = "readCounter" + this->generate_hex(8);
        error = DAQmxCreateTask(uniqueKey.c_str(), &(this->counterHandle2));
        if (error) {
            this->handleErrorAndCleanTask(this->counterHandle2);
            throw std::runtime_error("Failed to create DAQmx task.");
        }
        


        // Adjust the call to create the counter channel for PFI2.
        error = DAQmxCreateCICountEdgesChan(this->counterHandle2, fullChannelName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) 
        {
            this->handleErrorAndCleanTask(this->counterHandle2);
            throw std::runtime_error("Failed to create counter channel.");
        }

        // After creating the counter channel
        error = DAQmxSetCICountEdgesTerm(this->counterHandle2, fullChannelName.c_str(), "PFI1");
        if (error) {
            // Handle error 
        }

        // Start the task.
        error = DAQmxStartTask(this->counterHandle2);
        if (error) 
        {
            this->handleErrorAndCleanTask(this->counterHandle2);
            throw std::runtime_error("Failed to start counter task.");
        }
    }

    // Attempt to read the counter value.
    error = DAQmxReadCounterScalarU32(this->counterHandle2, 10.0, &readValue, nullptr);
    if (error) {
        this->handleErrorAndCleanTask(this->counterHandle2);
        throw std::runtime_error("Failed to read counter value.");
    }

    return readValue;
}



void QNiDaqWrapper::testSetRelayState(unsigned int relayIndex, const bool &state)
{
    std::lock_guard<std::mutex> lock(alarmsMutex);
    if(relayIndex > 3) 
    {
        std::cout<<"Error in testSetRelayAndLEDState relayIndex out of range. Valid range is 0-3 for Mod6."<<std::endl;
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
                                   "Error: relayIndex out of range. Valid range is 0-3 for Mod6.");
        throw std::invalid_argument("relayIndex out of range. Valid range is 0-3 for Mod6.");
    }

    // Relay control: port0/line[relayIndex] on Mod6
    std::string relayChannel = "Mod6/port0/line" + std::to_string(relayIndex);
    TaskHandle taskHandleRelay = 0;
    std::string uniqueKeyRelay = "relayControl" + generate_hex(8);
    int32 error;

    // Relay task creation and setup
    error = DAQmxCreateTask(uniqueKeyRelay.c_str(), &taskHandleRelay);
    if(error) 
    {
       std::cout<<"Failed to create DAQmx task for relay."<<std::endl;
       appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
                                   "Error: Failed to create DAQmx task for relay. Error: " + std::to_string(error));
        handleErrorAndCleanTask(taskHandleRelay);
        throw std::runtime_error("Failed to create DAQmx task for relay.");
    }

    error = DAQmxCreateDOChan(taskHandleRelay, relayChannel.c_str(), "", DAQmx_Val_ChanPerLine);
    if(error) 
    {
        std::cout<<"Error in testSetRelayAndLEDState: Failed to create digital output channel for relay."<<std::endl;
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
                                   "Error: Failed to create digital output channel for relay. Error: " + std::to_string(error));
        handleErrorAndCleanTask(taskHandleRelay);
        throw std::runtime_error("Failed to create digital output channel for relay.");
    }

  

    // Starting tasks
    error = DAQmxStartTask(taskHandleRelay);
    if(error) 
    {
        std::cout<<"Error in testSetRelayAndLEDState: Failed to start relay control task."<<std::endl;
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
                                   "Error: Failed to start relay control task. Error: " + std::to_string(error));
        handleErrorAndCleanTask(taskHandleRelay);
        throw std::runtime_error("Failed to start relay control task.");
    }

    // Setting state for relay and LED
    uInt8 data = state ? 1 : 0; // 1 for ON, 0 for OFF
    int32 written;

    error = DAQmxWriteDigitalLines(taskHandleRelay, 1, true, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
    if(error) 
    {
        std::cout<<"Error in testSetRelayAndLEDState: Failed to set relay state."<<std::endl;
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
                                   "Error: Failed to set relay state. Error: " + std::to_string(error));
        handleErrorAndCleanTask(taskHandleRelay);
        throw std::runtime_error("Failed to set relay state.");
    }

    // Cleaning up tasks
    DAQmxClearTask(taskHandleRelay);
}


void QNiDaqWrapper::setRelayState(NIDeviceModule *deviceModule, unsigned int chanIndex, const bool &state) 
{
    std::lock_guard<std::mutex> lock(alarmsMutex);
    if (!deviceModule) {
        throw std::invalid_argument("deviceModule is null");
    }

    // Construct the channel name using the index
    const char* deviceName   = deviceModule->getAlias().c_str();
    const char* channelName  = deviceModule->getChanNames()[chanIndex].c_str();
    std::string fullChanName = std::string(deviceName) + channelName;
    //initialize an handle for the task
    TaskHandle taskHandle = 0;
    int32      error;
    // Unique task name for setting relay state
    std::string uniqueKey = "setRelayState" + generate_hex(8);
    error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to create task for setting relay state.");
    }

    // Create a digital output channel for the specific relay
    error = DAQmxCreateDOChan(taskHandle, fullChanName.c_str(), "", DAQmx_Val_ChanForAllLines);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to create digital output channel for relay.");
    }

    // Start the task to apply the configuration
    error = DAQmxStartTask(taskHandle);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to start task for setting relay state.");
    }

    // Data to write: 1 for ON, 0 for OFF
    uInt8 data = state ? 1 : 0;
    int32 written;

    // Write the state to the digital output channel
    error = DAQmxWriteDigitalLines(taskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to write relay state.");
    }

    // Clean up the task after setting the state
    DAQmxClearTask(taskHandle);
}


void QNiDaqWrapper::setRelayState(NIDeviceModule *deviceModule, const std::string &chanName, const bool &state)
{
    std::lock_guard<std::mutex> lock(alarmsMutex);
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::setRelayState(NIDeviceModule *deviceModule, const std::string &chanName, const bool &state)\n"
                                   "Error:  deviceModule is nullptr");
        throw std::invalid_argument("deviceModule is null");
    }


    char errBuff[2048] = {'\0'};


    TaskHandle taskHandle = 0;
    int32 error;

    // Construct the full channel name including the device alias
    std::string fullChanName = deviceModule->getAlias() + chanName.c_str();

    // Create a new task for setting the relay state
    std::string uniqueKey = "setRelayState" + generate_hex(8);
    error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
    if (error) 
    {   
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::setRelayState(NIDeviceModule *deviceModule, const std::string &chanName, const bool &state)\n"
                                   "Error:  Failed to create task for setting relay state.\n"+
                                   std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to create task for setting relay state.");
    }

    // Create a digital output channel
    error = DAQmxCreateDOChan(taskHandle, fullChanName.c_str(), "", DAQmx_Val_ChanPerLine);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::setRelayState(NIDeviceModule *deviceModule, const std::string &chanName, const bool &state)\n"
                                   "Error:  Failed to create digital output channel.\n"+
                                   std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to create digital output channel.");
    }

    // Start the task
    error = DAQmxStartTask(taskHandle);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::setRelayState(NIDeviceModule *deviceModule, const std::string &chanName, const bool &state)\n"
                                   "Error:  Failed to start task.\n"+
                                   std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to start task.");
    }

    // Write the relay state to the channel
    uInt8 data = state ? 1 : 0; // Convert boolean state to uInt8
    int32 written;
    error = DAQmxWriteDigitalLines(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
    if (error) 
    {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::setRelayState(NIDeviceModule *deviceModule, const std::string &chanName, const bool &state)\n"
                                   "Error:  Failed to set relay state.\n"+
                                   std::string(errBuff));
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to set relay state.");
    }

    // Clean up the task
    DAQmxClearTask(taskHandle);
}


void QNiDaqWrapper::handleErrorAndCleanTask(TaskHandle taskHandle)
{
    char errBuff[2048] = {'\0'};

    // Get extended error information
    DAQmxGetExtendedErrorInfo(errBuff, 2048);

    // Print the extended error information to the standard error stream
    appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                               "In\n"
                               "void QNiDaqWrapper::handleErrorAndCleanTask(TaskHandle taskHandle)\n"
                               "Extended Error Info: \n"+
                                std::string(errBuff));
    // Check if taskHandle is valid before attempting to stop and clear the task
    if (taskHandle) 
    {
        // Stop the task if it's running
        DAQmxStopTask(taskHandle);

        // Clear the task to free up resources
        DAQmxClearTask(taskHandle);
    }
}


void QNiDaqWrapper::handleReadCurrentCompletion(int32 status)
{
    // Check if the status indicates an error
    if (status != 0)
    {
        // Handle the error. You can use DAQmxGetExtendedErrorInfo here.
        char errBuff[2048] = {'\0'};
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                               "In\n"
                               "void QNiDaqWrapper::handleReadCurrentCompletion(int32 status)\n"
                               "Extended Error Info: \n"+
                                std::string(errBuff));
    }

    // Additional code to execute when the task is done.

    // Verify if the signal is set before emitting it
    if (channelCurrentDataReadySignal)
    {
        channelCurrentDataReadySignal(m_lastSingleCurrentChannelValue, this);
    }
}


void QNiDaqWrapper::handleReadVoltageCompletion(int32 status)
{
    // Check if the status indicates an error
    if (status != 0)
    {
        // Handle the error. You can use DAQmxGetExtendedErrorInfo here.
        char errBuff[2048] = {'\0'};
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                               "In\n"
                               "void QNiDaqWrapper::handleReadVoltageCompletion(int32 status)\n"
                               "Extended Error Info: \n"+
                                std::string(errBuff));
    }

    // Additional code to execute when the task is done.

    // Verify if the signal is set before emitting it
    if (channelVoltageDataReadySignal)
    {
        channelVoltageDataReadySignal(m_lastSingleVoltageChannelValue, this);
    }
}



void QNiDaqWrapper::handleReadCounterCompletion(int32 status)
{
    // Check if the status indicates an error
    if (status != 0)
    {
        // Handle the error. You can use DAQmxGetExtendedErrorInfo here.
        char errBuff[2048] = {'\0'};
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "void QNiDaqWrapper::handleReadCounterCompletion(int32 status)\n"
                                   "Extended Error Info: \n"+
                                   std::string(errBuff));
    }

    // Additional code to execute when the task is done.

    // Verify if the signal is set before emitting it
    if (channelCounterDataReadySignal)
    {
        channelCounterDataReadySignal(m_lastSingleCounter, this);
    }
}



// Getter pour m_lastSingleChannelValue
double QNiDaqWrapper::getLastSingleCurrentChannelValue() const {
    return m_lastSingleCurrentChannelValue;
}

// Setter pour m_lastSingleChannelValue
void QNiDaqWrapper::setLastSingleCurrentChannelValue(double value) {
    m_lastSingleCurrentChannelValue = value;
    //emited as soon as the data for a channel has changed, 
    if (channelCurrentDataChangedSignal)
    {
        channelCurrentDataChangedSignal(value,this);
    } 

}

double QNiDaqWrapper::getLastSingleVoltageChannelValue() const
{
    return m_lastSingleVoltageChannelValue;
}

void QNiDaqWrapper::setLastSingleVoltageChannelValue(double value)
{
    m_lastSingleVoltageChannelValue = value;
        //emited as soon as the data for a channel has changed, 
    if (channelVoltageDataChangedSignal)
    {
        channelVoltageDataChangedSignal(value,this);
    } 
}

unsigned int QNiDaqWrapper::getLastSingleCounterValue() const
{
    return m_lastSingleCounter;
}

void QNiDaqWrapper::setLastSingleCounterValue(unsigned int value)
{
    m_lastSingleCounter = value;
    if (channelCounterDataChangedSignal)
    {
        channelCounterDataChangedSignal(value,this);
    }
}

bool QNiDaqWrapper::getWindowFilterActiv() const
{
    return m_rollingWindowFilterActiv;
}

void QNiDaqWrapper::setLWindowFilterActiv(bool isActiv)
{
    m_rollingWindowFilterActiv = isActiv;
}

bool QNiDaqWrapper::getLowPassFilterActiv() const
{
    return m_lowPassFilterActiv;
}

void QNiDaqWrapper::setLowPassFilterActiv(bool isActiv)
{
    m_lowPassFilterActiv = false;
}

bool QNiDaqWrapper::getNotchFilterActiv() const
{
    return m_notchFilterActiv;
}

void QNiDaqWrapper::setNotchFilterActiv(bool isActiv)
{
    m_notchFilterActiv = isActiv;
}

float QNiDaqWrapper::getLowPassFilterCutoffFrequency() const
{
    return m_cutOffFrequency;
}

void QNiDaqWrapper::setLowPassFilterCutoffFrequency(float cutOffFrequency)
{
    m_cutOffFrequency = cutOffFrequency;
}
