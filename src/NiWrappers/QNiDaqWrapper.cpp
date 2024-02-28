#include "QNiDaqWrapper.h"
#include <iostream>
#include <cstring>
#include <random>
#include <iomanip>
#include <memory>
#include "../NiModulesDefinitions/NIDeviceModule.h"



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
        // Uncomment for debugging: std::cout << "Found device: " << token << std::endl;
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
    while (true) {
        // Create a new task
        std::string unicKey = "getCurrentValue" + generate_hex(8);
        error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
        if (error) 
        {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to create task for reading current.");
            throw std::runtime_error("Failed to create task for reading current.");
        }

        // Register the Done callback
        error = DAQmxRegisterDoneEvent(taskHandle, 0, CurrentDoneCallback, this);
        if (error)
        {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                      "In\n"
                                      "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                      "Error: Failed to register Done callback.");
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

        if (error) {
            handleErrorAndCleanTask(taskHandle);
            if (++retryCount >= maxRetries) 
            {
                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                           "In\n"
                                           "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                           "Error: Failed to create channel after max retries.");
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        break;
    }

    // Start the task
    error = DAQmxStartTask(taskHandle);
    if (error) 
    {
        handleErrorAndCleanTask(taskHandle);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to starttask for reading current.");
        throw std::runtime_error("Failed to starttask for reading current.");
    }
    // Read the current value

    error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
    if (error) 
    {
        handleErrorAndCleanTask(taskHandle);  // Handle error and clean up
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to read current current value.");
        throw std::runtime_error("Failed to read current value.");
    }

    // Stop and clear the task
    error = DAQmxStopTask(taskHandle);
    if (error) 
    {
        handleErrorAndCleanTask(taskHandle);  // Handle error and clean up
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to stop task");
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
    while (true) {
        // Create a new task
        std::string unicKey = "getCurrentValue" + generate_hex(8);
        error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
        if (error) 
        {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to create task for reading current.");
            throw std::runtime_error("Failed to create task for reading current.");
        }

        // Register the Done callback
        error = DAQmxRegisterDoneEvent(taskHandle, 0, CurrentDoneCallback, this);
        if (error) 
        {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to register Done callback.");
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

        if (error) {
            handleErrorAndCleanTask(taskHandle);
            if (++retryCount >= maxRetries) 
            {
                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to create channel after max retries.");
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        break;
    }

    // Start the task
    error = DAQmxStartTask(taskHandle);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to start task for reading current.");
        throw std::runtime_error("Failed to start task for reading current.");
    }

    // Read the current value
    error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to read current value.");
        throw std::runtime_error("Failed to read current value.");
    }

    // Stop and clear the task
    error = DAQmxStopTask(taskHandle);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)\n"
                                   "Error: Failed to stop task.");
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


double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)
{
    std::lock_guard<std::mutex> lock(voltageMutex); // Ensure thread safety

    // Check for null pointer
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Null pointer passed for deviceModule.");
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    int32 error;
    float64 readValue;
    TaskHandle taskHandle = 0;

    const char* deviceName = deviceModule->getAlias().c_str    ();
    double minRange        = deviceModule->getChanMin          ();
    double maxRange        = deviceModule->getChanMax          ();
    int32 termCfg          = deviceModule->getModuleTerminalCfg();
    int32 unit             = deviceModule->getModuleUnit       ();

    // Construct the full channel name
    const char* channelName = chanName.c_str();
    std::string fullChannelName = std::string(deviceName) + channelName;

    unsigned int retryCount = 0;

    while (true) 
    {
        // Generate a unique task key
        std::string uniqueKey = "getVoltageValue" + generate_hex(8);
        error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
        if (error) {
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to create task for reading voltage.");
            throw std::runtime_error("Failed to create task for reading voltage.");
        }

        error = DAQmxRegisterDoneEvent(taskHandle, 0, VoltageDoneCallback, this);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to register Done callback.");
            throw std::runtime_error("Failed to register Done callback.");
        }

        // Create an analog input voltage channel
        error = DAQmxCreateAIVoltageChan(taskHandle,
                                         fullChannelName.c_str(),
                                         "",
                                         termCfg,
                                         minRange,
                                         maxRange,
                                         unit,
                                         NULL);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            if (++retryCount >= maxRetries) 
            {
                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to create channel after max retries."); 
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Add sample clock timing configuration (NEW)
        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
        if (error) 
        {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to set sample clock timing.");
            throw std::runtime_error("Failed to set sample clock timing.");
        }

        // Start the task
        error = DAQmxStartTask(taskHandle);
        if (error) 
        {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to start task for reading voltage.");
            throw std::runtime_error("Failed to start task for reading voltage.");
        }

        // Read a voltage value
        error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
        if (error) 
        {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to read voltage value.");
            throw std::runtime_error("Failed to read voltage value.");
        }

        // Stop the task
        error = DAQmxStopTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: Failed to stop task.");
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

    while (true) {
        std::string unicKey = "getVoltageValue" + generate_hex(8);
        error = DAQmxCreateTask(unicKey.c_str(), &taskHandle);
        if (error) {
            handleErrorAndCleanTask(taskHandle); // Custom function to handle errors and clean up
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to create task for reading voltage.");
            throw std::runtime_error("Failed to create task for reading voltage.");
        }

        error = DAQmxRegisterDoneEvent(taskHandle, 0, VoltageDoneCallback, this);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to register Done callback.");
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
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            if (++retryCount >= maxRetries) {
                appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to create channel after max retries.");
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to set sample clock timing.");
            throw std::runtime_error("Failed to set sample clock timing.");
        }

        error = DAQmxStartTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to start task for reading voltage.");
            throw std::runtime_error("Failed to start task for reading voltage.");
        }

        error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to read voltage value.");
            throw std::runtime_error("Failed to read voltage value.");
        }

        error = DAQmxStopTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask(taskHandle);
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "double QNiDaqWrapper::readVoltage(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)\n"
                                   "Error: Failed to stop task.");
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




unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries) {
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
    while (retryCount < maxRetries) {
        int32 error = DAQmxReadCounterScalarU32(taskHandle, 10.0, &readValue, nullptr);
        if (error) {
            char errBuff[2048] = {'\0'};
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



unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries) {
    if (!deviceModule) 
    {
        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                   "In\n"
                                   "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                   "Error: deviceModule is null.");
        throw std::invalid_argument("readCounter: deviceModule is null.");
    }

    const std::string fullChannelName = deviceModule->getAlias() + chanName;
    std::cout<<"in readCounter fullChannelName = "<<fullChannelName.c_str()<<std::endl;
    // Check if a task for this channel already exists, create if not
    TaskHandle& taskHandle = counterTasksMap[fullChannelName];
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
        else
        {
            std::cout<<"in readCounter task created successfully "<<taskHandle<<std::endl;
        }

        error = DAQmxCreateCICountEdgesChan(taskHandle, fullChannelName.c_str(), "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp);
        if (error) 
        {
            std::cout<<"in read counter Failed to create counter channel: "<<fullChannelName.c_str()<<std::endl;
            char errBuff[2048];
            DAQmxGetErrorString(error, errBuff, sizeof(errBuff));
            appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
                                       "In\n"
                                       "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
                                       "Failed to create counter channel. Error: " + std::string(errBuff));
            handleErrorAndCleanTask(taskHandle);
            taskHandle = nullptr; // Ensure the map entry is reset after cleanup
            throw std::runtime_error("Failed to create counter channel.");
        }
        else
        {
            std::cout<<"in readCounter channel created ok"<<std::endl;
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
        else
        {
            std::cout<<"in read counter: startTask Success"<<std::endl;
        }
    }

    uInt32 readValue = 0;
    unsigned int retryCount = 0;
    int32 error;
    //while (retryCount < maxRetries) 
    //{
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
            retryCount++;
            //continue; // Attempt to read again if under the retry limit
        }
        else
        {
            std::cout<<"in Read counter DAQmxReadCounterScalarU32 seems ok"<<std::endl;
        }
        //break; // Successful read
    //}



    //if (retryCount >= maxRetries) 
    //{
    //    appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
    //                               "In\n"
    //                               "unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, std::string chanName, unsigned int maxRetries)\n"
    //                               "Error: Failed to read counter after maximum retries.");
    //    handleErrorAndCleanTask(taskHandle); // Ensures that resources are freed and task handle is reset
    //    taskHandle = nullptr; // Reset the handle in the map after cleanup
    //    throw std::runtime_error("Failed to read counter after maximum retries.");
    //}

    std::cout<<"value is "<<readValue<<std::endl;
    return readValue; // Return the successfully read value
}


unsigned int QNiDaqWrapper::testReadCounter()
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
}


//void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)
//{
//    if(relayIndex > 3) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState relayIndex out of range. Valid range is 0-3 for Mod6."<<std::endl;
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: relayIndex out of range. Valid range is 0-3 for Mod6.");
//        throw std::invalid_argument("relayIndex out of range. Valid range is 0-3 for Mod6.");
//    }
//
//    // Relay control: port0/line[relayIndex] on Mod6
//    std::string relayChannel = "Mod6/port0/line" + std::to_string(relayIndex);
//    
//    // Assuming the corresponding LED control is done through "Mod6/ctr[relayIndex]"
//    std::string ledChannel = "Mod6/ctr" + std::to_string(relayIndex);
//    
//    TaskHandle taskHandleRelay = 0, taskHandleLED = 0;
//    std::string uniqueKeyRelay = "relayControl" + generate_hex(8);
//    std::string uniqueKeyLED = "ledControl" + generate_hex(8);
//    int32 error;
//
//    // Relay task creation and setup
//    error = DAQmxCreateTask(uniqueKeyRelay.c_str(), &taskHandleRelay);
//    if(error) 
//    {
//       std::cout<<"Failed to create DAQmx task for relay."<<std::endl;
//       appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to create DAQmx task for relay. Error: " + std::to_string(error));
//        handleErrorAndCleanTask(taskHandleRelay);
//        throw std::runtime_error("Failed to create DAQmx task for relay.");
//    }
//
//    error = DAQmxCreateDOChan(taskHandleRelay, relayChannel.c_str(), "", DAQmx_Val_ChanPerLine);
//    if(error) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState: Failed to create digital output channel for relay."<<std::endl;
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to create digital output channel for relay. Error: " + std::to_string(error));
//        handleErrorAndCleanTask(taskHandleRelay);
//        throw std::runtime_error("Failed to create digital output channel for relay.");
//    }
//
//    // LED task creation and setup (counter for LED as a digital output emulation)
//    error = DAQmxCreateTask(uniqueKeyLED.c_str(), &taskHandleLED);
//    if(error) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState: Failed to create digital output channel for relay."<<std::endl;
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to create DAQmx task for LED. Error: " + std::to_string(error));
//        handleErrorAndCleanTask(taskHandleLED);
//        handleErrorAndCleanTask(taskHandleLED);
//        throw std::runtime_error("Failed to create DAQmx task for LED.");
//    }
//
//    error = DAQmxCreateDOChan(taskHandleLED, ledChannel.c_str(), "", DAQmx_Val_ChanPerLine);
//    if(error) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState: Failed to create digital output channel for LED."<<std::endl;
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to create digital output channel for LED. Error: " + std::to_string(error));
//        handleErrorAndCleanTask(taskHandleLED);
//        throw std::runtime_error("Failed to create digital output channel for LED.");
//    }
//
//    // Starting tasks
//    error = DAQmxStartTask(taskHandleRelay);
//    if(error) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState: Failed to start relay control task."<<std::endl;
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to start relay control task. Error: " + std::to_string(error));
//        handleErrorAndCleanTask(taskHandleRelay);
//        throw std::runtime_error("Failed to start relay control task.");
//    }
//
//    error = DAQmxStartTask(taskHandleLED);
//    if(error) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState: Failed to start LED control task."<<std::endl;
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to start LED control task. Error: " + std::to_string(error));
//        handleErrorAndCleanTask(taskHandleLED);
//        throw std::runtime_error("Failed to start LED control task.");
//    }
//
//    // Setting state for relay and LED
//    uInt8 data = state ? 1 : 0; // 1 for ON, 0 for OFF
//    int32 written;
//
//    error = DAQmxWriteDigitalLines(taskHandleRelay, 1, true, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
//    if(error) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState: Failed to set relay state."<<std::endl;
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to set relay state. Error: " + std::to_string(error));
//        handleErrorAndCleanTask(taskHandleRelay);
//        throw std::runtime_error("Failed to set relay state.");
//    }
//
//    error = DAQmxWriteDigitalLines(taskHandleLED, 1, true, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
//    if(error) 
//    {
//        std::cout<<"Error in testSetRelayAndLEDState: Failed to set LED state."<<std::endl;
//        handleErrorAndCleanTask(taskHandleLED);
//        appendCommentWithTimestamp(fileNamesContainer.QNiDaqWrapperLogFile,
//                                   "In\n"
//                                   "void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)\n"
//                                   "Error: Failed to set LED state. Error: " + std::to_string(error));
//        throw std::runtime_error("Failed to set LED state.");
//    }
//
//    // Cleaning up tasks
//    DAQmxClearTask(taskHandleRelay);
//    DAQmxClearTask(taskHandleLED);
//}


void QNiDaqWrapper::testSetRelayAndLEDState(unsigned int relayIndex, const bool &state)
{
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
    if (!deviceModule) {
        throw std::invalid_argument("deviceModule is null");
    }

    TaskHandle taskHandle = 0;
    int32 error;

    // Construct the full channel name including the device alias
    const char* fullChanName = (deviceModule->getAlias() + "/" + chanName).c_str();

    // Create a new task for setting the relay state
    std::string uniqueKey = "setRelayState" + generate_hex(8);
    error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to create task for setting relay state.");
    }

    // Create a digital output channel
    error = DAQmxCreateDOChan(taskHandle, fullChanName, "", DAQmx_Val_ChanPerLine);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to create digital output channel.");
    }

    // Start the task
    error = DAQmxStartTask(taskHandle);
    if (error) {
        handleErrorAndCleanTask(taskHandle);
        throw std::runtime_error("Failed to start task.");
    }

    // Write the relay state to the channel
    uInt8 data = state ? 1 : 0; // Convert boolean state to uInt8
    int32 written;
    error = DAQmxWriteDigitalLines(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
    if (error) {
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
