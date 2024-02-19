#include "QNiDaqWrapper.h"
#include <iostream>
#include <cstring>
#include <random>
#include <iomanip>
#include "../NiModulesDefinitions/NIDeviceModule.h"



static int32 CVICALLBACK CurrentDoneCallback(TaskHandle taskHandle, 
                                             int32 status,
                                             void *callbackData)
{
    // Check for null pointer before casting
    if (!callbackData) {
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
    // Initialize taskHandle to 0. This indicates that the task is not yet created or initialized.
    taskHandle = 0;

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
    // Before attempting to clear the task, check if taskHandle is a valid handle.
    // A value of 0 indicates either an uninitialized or already cleared task.
    if (taskHandle != 0) {
        // Attempt to clear the DAQmx task.
        // DAQmxClearTask function cleans up the task and releases any resources.
        // If DAQmxClearTask fails, it's important to log this as it might indicate a deeper issue.
        int32 error = DAQmxClearTask(taskHandle);
        if (error) {
            // Error handling: Log the error code or message.
            // In production code, consider a more sophisticated logging mechanism.
            std::cerr << "Error clearing DAQmx task: " << error << std::endl;
        }

        // After clearing the task, set taskHandle back to 0 to indicate that the task is no longer valid.
        // This is important to prevent double-clearing in case of multiple destructor calls.
        taskHandle = 0;
    }

    // Additional cleanup code can go here.
    // This may include releasing other resources like memory, file handles, etc.
}


int32 QNiDaqWrapper::GetNumberOfModules() 
{
    char devNames[512]; // Buffer to hold device names

    // Retrieve the list of device names
    int32 error = DAQmxGetSystemInfoAttribute(DAQmx_Sys_DevNames, devNames, sizeof(devNames));
    
    // Check for errors in retrieving device names
    if (error) {
        std::cerr << "Error retrieving device names: " << error << std::endl;
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
// Create a new task for device enumeration
    error = DAQmxCreateTask("getDevice", &taskHandle);

    if (error < 0) {
        std::cerr << "Unable to create new task" << std::endl;
        return devices;
    }

    // First call to get the required buffer size. Note the use of &bufferSize to pass a pointer.
    error = DAQmxGetSysDevNames(nullptr, bufferSize);
    if (error < 0) {
        std::cerr << "Error determining buffer size for device names." << std::endl;
        DAQmxClearTask(taskHandle);
        return devices;
    }

    if (bufferSize == 0) {
        std::cerr << "Buffer size is 0!" << std::endl;
        DAQmxClearTask(taskHandle);
        return devices;
    }

    // Allocate buffer for device names
    std::vector<char> devNames(bufferSize);

    // Second call to actually get the device names
    error = DAQmxGetSysDevNames(devNames.data(), bufferSize);
    if (error < 0) {
        std::cerr << "Error retrieving device names." << std::endl;
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

    // Check for null pointer
    if (!deviceModule) {
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    int32 error;
    float64 readValue;

    // Extract necessary information from NIDeviceModule
    const char* deviceName = deviceModule->getAlias().c_str();
    moduleShuntLocation shuntLoc = deviceModule->getModuleShuntLocation();
    if (shuntLoc == noShunt) {
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
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to create task for reading current.");
        }

        // Register the Done callback
        error = DAQmxRegisterDoneEvent(taskHandle, 0, CurrentDoneCallback, this);
        if (error) {
            handleErrorAndCleanTask();
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
            handleErrorAndCleanTask();
            if (++retryCount >= maxRetries) {
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
        handleErrorAndCleanTask();
        throw std::runtime_error("Failed to starttask for reading current.");
}// Read the current value
error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
if (error) {
    handleErrorAndCleanTask();  // Handle error and clean up
    throw std::runtime_error("Failed to read current value.");
}

// Stop and clear the task
error = DAQmxStopTask(taskHandle);
if (error) {
    handleErrorAndCleanTask();  // Handle error and clean up
    throw std::runtime_error("Failed to stop task.");
}
DAQmxClearTask(taskHandle);

// Convert the read value to the appropriate unit and return it
double result = static_cast<double>(readValue);
if (autoConvertTomAmps) {
    result = ampsTomAmps(result);
}
setLastSingleCurrentChannelValue(result);
return result;
}


double QNiDaqWrapper::readCurrent(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries, bool autoConvertTomAmps)
{
    std::lock_guard<std::mutex> lock(currentMutex); // Ensure thread safety

    // Check for null pointer
    if (!deviceModule) {
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    int32 error;
    float64 readValue;

    // Extract necessary information from NIDeviceModule
    const char* deviceName = deviceModule->getAlias().c_str();
    moduleShuntLocation shuntLoc = deviceModule->getModuleShuntLocation();
    if (shuntLoc == noShunt) {
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
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to create task for reading current.");
        }

        // Register the Done callback
        error = DAQmxRegisterDoneEvent(taskHandle, 0, CurrentDoneCallback, this);
        if (error) {
            handleErrorAndCleanTask();
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
            handleErrorAndCleanTask();
            if (++retryCount >= maxRetries) {
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
        handleErrorAndCleanTask();
        throw std::runtime_error("Failed to start task for reading current.");
    }

    // Read the current value
    error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
    if (error) {
        handleErrorAndCleanTask();
        throw std::runtime_error("Failed to read current value.");
    }

    // Stop and clear the task
    error = DAQmxStopTask(taskHandle);
    if (error) {
        handleErrorAndCleanTask();
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
    if (!deviceModule) {
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

    // Construct the full channel name
    const char* channelName = chanName.c_str();
    std::string fullChannelName = std::string(deviceName) + channelName;

    unsigned int retryCount = 0;

    while (true) {
        // Generate a unique task key
        std::string uniqueKey = "getVoltageValue" + generate_hex(8);
        error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
        if (error) {
            handleErrorAndCleanTask(); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to create task for reading voltage.");
        }

        error = DAQmxRegisterDoneEvent(taskHandle, 0, VoltageDoneCallback, this);
        if (error) {
            handleErrorAndCleanTask();
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
            handleErrorAndCleanTask();
            if (++retryCount >= maxRetries) {
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Add sample clock timing configuration (NEW)
        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to set sample clock timing.");
        }

        // Start the task
        error = DAQmxStartTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to start task for reading voltage.");
        }

        // Read a voltage value
        error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to read voltage value.");
        }

        // Stop the task
        error = DAQmxStopTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask();
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

    if (!deviceModule) {
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
            handleErrorAndCleanTask(); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to create task for reading voltage.");
        }

        error = DAQmxRegisterDoneEvent(taskHandle, 0, VoltageDoneCallback, this);
        if (error) {
            handleErrorAndCleanTask();
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
            handleErrorAndCleanTask();
            if (++retryCount >= maxRetries) {
                throw std::runtime_error("Failed to create channel after max retries.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        error = DAQmxCfgSampClkTiming(taskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to set sample clock timing.");
        }

        error = DAQmxStartTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to start task for reading voltage.");
        }

        error = DAQmxReadAnalogScalarF64(taskHandle, 10.0, &readValue, nullptr);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to read voltage value.");
        }

        error = DAQmxStopTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask();
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

unsigned int QNiDaqWrapper::readCounter(NIDeviceModule *deviceModule, unsigned int chanIndex, unsigned int maxRetries)
{
    // Check for null pointer
    if (!deviceModule) {
        throw std::invalid_argument("Null pointer passed for deviceModule.");
    }

    int32 error;
    uInt32 readValue; // Counter values are usually integers
    TaskHandle taskHandle = 0;

    const char* deviceName = deviceModule->getAlias().c_str();
    const char* channelName = deviceModule->getChanNames()[chanIndex].c_str();
    std::string fullChannelName = std::string(deviceName) + channelName;

    unsigned int retryCount = 0;

    while (true) {
        // Generate a unique task key
        std::string uniqueKey = "getCounterValue" + generate_hex(8);
        error = DAQmxCreateTask(uniqueKey.c_str(), &taskHandle);
        if (error) {
            handleErrorAndCleanTask(); // Custom function to handle errors and clean up
            throw std::runtime_error("Failed to create task for reading counter.");
        }

        error = DAQmxRegisterDoneEvent(taskHandle, 0, CounterDoneCallback, this);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to register Done callback for counter.");
        }

        // Create a counter input channel
        error = DAQmxCreateCICountEdgesChan(taskHandle,
                                            fullChannelName.c_str(),
                                            "",
                                            DAQmx_Val_Rising,  // Count rising edges (customize as needed)
                                            0,                 // Initial count
                                            DAQmx_Val_CountUp);// Counting direction
        if (error) {
            handleErrorAndCleanTask();
            if (++retryCount >= maxRetries) {
                throw std::runtime_error("Failed to create counter channel after max retries.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Start the task
        error = DAQmxStartTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to start task for reading counter.");
        }

        // Read a counter value
        error = DAQmxReadCounterScalarU32(taskHandle, 10.0, &readValue, nullptr);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to read counter value.");
        }

        // Stop the task
        error = DAQmxStopTask(taskHandle);
        if (error) {
            handleErrorAndCleanTask();
            throw std::runtime_error("Failed to stop counter task.");
        }

        // Clear the task to free up resources
        DAQmxClearTask(taskHandle);

        break;
    }

    // Convert to appropriate unit (if necessary)
    unsigned int result = readValue;
    setLastSingleCounterValue(result); // Assuming you have a similar function for counter

    // Emit signal when counter data is ready
    if (channelCounterDataReadySignal) {
        channelCounterDataReadySignal(result, this);
    }

    return result; // Return the read value
}


void QNiDaqWrapper::handleErrorAndCleanTask()
{
    char errBuff[2048] = {'\0'};

    // Get extended error information
    DAQmxGetExtendedErrorInfo(errBuff, 2048);

    // Print the extended error information to the standard error stream
    std::cerr << "Extended Error Info: " << errBuff << std::endl;

    // Check if taskHandle is valid before attempting to stop and clear the task
    if (taskHandle) {
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
        std::cerr << "Extended Error Info: " << errBuff << std::endl;
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
        std::cerr << "Extended Error Info: " << errBuff << std::endl;
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
        std::cerr << "Extended Error Info: " << errBuff << std::endl;
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
