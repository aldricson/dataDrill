#include "NItoModbusBridge.h"
#include <cmath>   // for std::sin
#include <cstdlib> // for std::rand
#include <vector>

// Constructor
NItoModbusBridge::NItoModbusBridge(std::shared_ptr<AnalogicReader> analogicReader,
                                   std::shared_ptr<DigitalReader> digitalReader,
                                   std::shared_ptr<NewModbusServer> modbusServer)
    : m_simulationBuffer(),
      m_realDataBuffer(),
      m_analogicReader(analogicReader),
      m_digitalReader(digitalReader),
      m_modbusServer(modbusServer)
{

    m_simulateTimer = std::make_shared<SimpleTimer>();
    std::chrono::milliseconds mss(250);
    m_simulateTimer->setInterval(mss);
    m_simulateTimer->stop();
    // Wire up the signals and slots
    m_simulateTimer->setSlotFunction([this]()
                                     { this->onSimulationTimerTimeOut(); });

    m_dataAcquTimer = std::make_shared<SimpleTimer>();
    std::chrono::milliseconds msr(500);
    m_dataAcquTimer->setInterval(msr);
    m_dataAcquTimer->stop();
    // Wire up the signals and slots
    m_dataAcquTimer->setSlotFunction([this]()
                                     { this->onDataAcquisitionTimerTimeOut(); });
}

// Getters and setters for AnalogicReader
std::shared_ptr<AnalogicReader> NItoModbusBridge::getAnalogicReader() const
{
    return m_analogicReader;
}

void NItoModbusBridge::setAnalogicReader(std::shared_ptr<AnalogicReader> analogicReader)
{
    m_analogicReader = analogicReader;
    if (onAnalogicReaderChanged)
    {
        onAnalogicReaderChanged();
    }
}

// Getters and setters for DigitalReader
std::shared_ptr<DigitalReader> NItoModbusBridge::getDigitalReader() const
{
    return m_digitalReader;
}

void NItoModbusBridge::setDigitalReader(std::shared_ptr<DigitalReader> digitalReader)
{
    m_digitalReader = digitalReader;
    if (onDigitalReaderChanged)
    {
        onDigitalReaderChanged();
    }
}


void NItoModbusBridge::loadMapping()
{
    // Open the mapping file
    std::ifstream file("mapping.csv");
    
    // Check if the file is open successfully
    if (!file.is_open())
    {
        std::cerr << "Failed to open mapping.csv file" << std::endl;
        return; // Exit the function if file opening fails
    }

    std::string line;

    // Read and process each line of the file
    while (getline(file, line))
    {
        // Create a string stream to tokenize the line
        std::istringstream iss(line);
        MappingConfig config;
        std::string token;

        // Tokenize each line and assign values to config

        // Tokenize and parse 'index' field
        if (getline(iss, token, ';'))
        {
            try
            {
                config.index = std::stoi(token);
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Failed to parse 'index' value: " << e.what() << std::endl;
                continue; // Skip this line and proceed to the next one
            }
        }
        else
        {
            std::cerr << "Missing 'index' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'moduleType' field
        if (getline(iss, token, ';'))
        {
            try
            {
                config.moduleType = static_cast<ModuleType>(std::stoi(token));
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Failed to parse 'moduleType' value: " << e.what() << std::endl;
                continue; // Skip this line and proceed to the next one
            }
        }
        else
        {
            std::cerr << "Missing 'moduleType' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'module' field
        if (getline(iss, token, ';'))
        {
            config.module = token;
        }
        else
        {
            std::cerr << "Missing 'module' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'channel' field
        if (getline(iss, token, ';'))
        {
            config.channel = token;
        }
        else
        {
            std::cerr << "Missing 'channel' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'minSource' field
        if (getline(iss, token, ';'))
        {
            try
            {
                config.minSource = std::stof(token);
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Failed to parse 'minSource' value: " << e.what() << std::endl;
                continue; // Skip this line and proceed to the next one
            }
        }
        else
        {
            std::cerr << "Missing 'minSource' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'maxSource' field
        if (getline(iss, token, ';'))
        {
            try
            {
                config.maxSource = std::stof(token);
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Failed to parse 'maxSource' value: " << e.what() << std::endl;
                continue; // Skip this line and proceed to the next one
            }
        }
        else
        {
            std::cerr << "Missing 'maxSource' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'minDest' field
        if (getline(iss, token, ';'))
        {
            try
            {
                config.minDest = static_cast<uint16_t>(std::stoi(token));
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Failed to parse 'minDest' value: " << e.what() << std::endl;
                continue; // Skip this line and proceed to the next one
            }
        }
        else
        {
            std::cerr << "Missing 'minDest' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'maxDest' field
        if (getline(iss, token, ';'))
        {
            try
            {
                config.maxDest = static_cast<uint16_t>(std::stoi(token));
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Failed to parse 'maxDest' value: " << e.what() << std::endl;
                continue; // Skip this line and proceed to the next one
            }
        }
        else
        {
            std::cerr << "Missing 'maxDest' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Tokenize and parse 'modbusChannel' field
        if (getline(iss, token, ';'))
        {
            try
            {
                config.modbusChannel = std::stoi(token);
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Failed to parse 'modbusChannel' value: " << e.what() << std::endl;
                continue; // Skip this line and proceed to the next one
            }
        }
        else
        {
            std::cerr << "Missing 'modbusChannel' value in mapping.csv" << std::endl;
            continue; // Skip this line and proceed to the next one
        }

        // Add the parsed config to the m_mappingData vector
        m_mappingData.push_back(config);
    }
}


bool NItoModbusBridge::startModbusSimulation()
{
    // Check if the simulateTimer is already active
    if (m_simulateTimer->isActive())
    {
        return true; // Simulation is already running, return true
    }

    try
    {
        // Stop the data acquisition timer to avoid conflicts
        m_dataAcquTimer->stop();

        // Clear the simulation buffer to start with a clean slate
        m_simulationBuffer.clear();

        // Start the simulation timer
        m_simulateTimer->start();

        return true; // Successfully started the simulation
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during timer operations
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;

        // Return false to indicate that simulation start failed
        return false;
    }
}



void NItoModbusBridge::stopModbusSimulation()
{
    try
    {
        // Stop the simulation timer
        m_simulateTimer->stop();
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during timer operations
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}


bool NItoModbusBridge::startAcquisition()
{
    // Check if the data acquisition timer is already active
    if (m_dataAcquTimer->isActive())
    {
        return true; // Data acquisition is already running, return true
    }

    try
    {
        // Clear the realDataBuffer to start with a clean slate
        m_realDataBuffer.clear();

        // Calculate the buffer size based on SRU mapping size without alarms
        int bufferSize = m_modbusServer->getSRUMappingSizeWithoutAlarms();

        // Clear the realDataBufferLine to start with a clean slate
        m_realDataBufferLine.clear();

        // Resize and initialize the realDataBufferLine with zeros
        m_realDataBufferLine.resize(bufferSize, 0);

        // Stop the simulation timer to avoid conflicts
        m_simulateTimer->stop();

        // Start the data acquisition timer
        m_dataAcquTimer->start();

        return true; // Successfully started data acquisition
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during timer operations
        // Log the error message for debugging purposes
        std::cerr << e.what() << '\n';

        // Return false to indicate that data acquisition start failed
        return false;
    }
}



void NItoModbusBridge::stopAcquisition()
{
    try
    {
        // Stop the data acquisition timer
        m_dataAcquTimer->stop();
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during timer operations
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}


uint16_t NItoModbusBridge::linearInterpolation16Bits(double value, double minSource, double maxSource, uint16_t minDestination, uint16_t maxDestination)
{
    try
    {
        // Calculate the scaling factor
        double scale = (maxDestination - minDestination) / (maxSource - minSource);

        // Perform the linear interpolation
        double mappedValue = minDestination + scale * (value - minSource);

        // Clamp the value within the destination boundaries
        if (mappedValue < static_cast<double>(minDestination))
        {
            mappedValue = static_cast<double>(minDestination);
        }
        else if (mappedValue > static_cast<double>(maxDestination))
        {
            mappedValue = static_cast<double>(maxDestination);
        }

        // Cast the mapped value to uint16_t and return it
        return static_cast<uint16_t>(mappedValue);
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during the interpolation
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;

        // Return the minimum destination value to indicate failure
        return minDestination;
    }
}


void NItoModbusBridge::onSimulationTimerTimeOut()
{
    try
    {
        constexpr uint64_t maxCounterValue = 18446744073709551615ULL; // Max counter value for wrap-around;

        // Vector to hold simulated data for each channel
        std::vector<uint16_t> analogChannelsResult;
        
        // Simulate analogic inputs
        simulateAnalogicInputs(analogChannelsResult);

        // Simulate counters
        simulateCounters(analogChannelsResult);

        // Simulate coders
        simulateCoders(analogChannelsResult);

        std::string str = "crio simulated values:\n";
        for (std::size_t i = 0; i < analogChannelsResult.size(); ++i)
        {
            str += std::to_string(analogChannelsResult[i]) + ";";
        }

        // Remap input register values for analogics
        m_modbusServer->reMapInputRegisterValuesForAnalogics(analogChannelsResult);

        // Wrap around the simulation counter
        m_simulationCounter = (m_simulationCounter + 1) % maxCounterValue;
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during simulation
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}


void NItoModbusBridge::simulateAnalogicInputs(std::vector<uint16_t> &analogChannelsResult)
{
    try
    {
        // Constants for simulating sine wave data
        constexpr double amplitude = 50.0;                                  // Amplitude of the sine wave
        constexpr double offset = 50.0;                                     // DC offset to shift the sine wave
        constexpr double omega = 2.0 * M_PI / 1000.0;                       // Frequency component for sine wave
        int numChannels = m_modbusServer->getSRUMapping().m_nbSRUAnalogsIn; // Number of channels to simulate
        bool isExlogCompatible = m_modbusServer->getSRUMapping().m_modeSRU; // To ensure compatibility with exlog
        
        // Calculate the sine value for the current simulation counter
        double sineValue = amplitude * std::sin(omega * m_simulationCounter) + offset;
        
        // Loop through each channel to generate simulated data
        if (isExlogCompatible)
        {
            // Emulate the pnf config "particularity of 16-bit shift" by adding an empty 16-bit unsigned integer
            analogChannelsResult.push_back(static_cast<uint16_t>(0));
        }
        
        for (int i = 0; i < numChannels; ++i)
        {
            // Generate random noise in the range of -0.1 to 0.1
            double noise = ((std::rand() % 21) - 10) / 100.0;
            
            // Add noise to the sine value
            double noisySineValue = sineValue * (1.0 + noise);
            
            // Map the noisy sine value to a 16-bit unsigned integer
            uint16_t mappedValue = linearInterpolation16Bits(noisySineValue, 0.0, 100.0, 0, 65535);
            
            // Add the mapped value to the result vector
            analogChannelsResult.push_back(mappedValue);
        }
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during simulation
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}


void NItoModbusBridge::simulateCounters(std::vector<uint16_t> &analogChannelsResult)
{
    try
    {
        for (int i = 0; i < m_modbusServer->getSRUMapping().m_nbSRUCounters; ++i)
        {
            // Push initial frequency value (16 bits) for SRU counters
            analogChannelsResult.push_back(32768); // 0=0 ----> 65535=3000 for SRU
            
            // Increment the simulated counter value
            m_simulatedCounterValue++;
            
            // Extract high and low 16-bit values from the simulated counter value (int 32)
            uint16_t highValue = static_cast<uint16_t>((m_simulatedCounterValue >> 16) & 0xFFFF); // High 16 bits
            uint16_t lowValue  = static_cast<uint16_t>(m_simulatedCounterValue & 0xFFFF);          // Low 16 bits
            
            // Push the high and low 16-bit values for the counter
            analogChannelsResult.push_back(highValue); // 16 bits for high (int 32)
            analogChannelsResult.push_back(lowValue);  // 16 bits for low (int 32)
        }
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during simulation
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}

void NItoModbusBridge::simulateCoders(std::vector<uint16_t> &analogChannelsResult)
{
    try
    {
        for (int i = 0; i < m_modbusServer->getSRUMapping().m_nbSRUCoders; ++i)
        {
            // Increment the simulated coders value every 4 simulation cycles
            if (m_simulationCounter % 4 == 0)
                m_simulatedCodersValue++;
            
            // Extract high and low 16-bit values from the simulated coders value (int 32)
            uint16_t highValue = static_cast<uint16_t>((m_simulatedCodersValue >> 16) & 0xFFFF); // High 16 bits
            uint16_t lowValue = static_cast<uint16_t>(m_simulatedCodersValue & 0xFFFF);          // Low 16 bits
            
            // Push the high and low 16-bit values for the coders
            analogChannelsResult.push_back(highValue); // 16 bits for high (int 32)
            analogChannelsResult.push_back(lowValue);  // 16 bits for low (int 32)
        }
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during simulation
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}

void NItoModbusBridge::acquireData()
{
    try
    {
        std::cout << "Enter acquire data" << std::endl;
        
        // Iterate through the mapping data
        for (std::size_t i = 0; i < m_mappingData.size(); ++i)
        {
            // Retrieve the configuration for the current mapping line
            MappingConfig lineCfg = m_mappingData[i];
            
            // Initialize variables
            double result = 0.0;
            double minInput = lineCfg.minSource;
            double maxInput = lineCfg.maxSource;
            uint16_t minOutput = lineCfg.minDest;
            uint16_t maxOutput = lineCfg.maxDest;
            int destinationRegister = lineCfg.modbusChannel;
            
            switch (lineCfg.moduleType)
            {
                // We have the same function  process  whether Moule Type is  AnalogicInputCurrent or AnalogicInputVoltage.
                case ModuleType::isAnalogicInputCurrent:
                case ModuleType::isAnalogicInputVoltage:
                {
                    // Read analog data
                    m_analogicReader->manualReadOneShot(lineCfg.module, lineCfg.channel, result);
                    
                    // Perform linear interpolation
                    uint16_t interpolatedResult = linearInterpolation16Bits(result, minInput, maxInput, minOutput, maxOutput);
                    
                    // Update the real data buffer line
                    m_realDataBufferLine[destinationRegister] = interpolatedResult;
                    
                    // Print acquired data for debugging
                    std::cout << lineCfg.module.c_str() << " " << lineCfg.channel.c_str() << " " << interpolatedResult << std::endl;
                    break;
                }
                case ModuleType::isCoder:
                {
                    // Handle coder data (Add your implementation here if needed)
                    break;
                }
                case ModuleType::isCounter:
                {
                    // Handle counter data (Add your implementation here if needed)
                    break;
                }
                case ModuleType::isDigitalInput:
                {
                    // Handle digital input data (Add your implementation here if needed)
                    break;
                }
            }
        }
        
        // Remap the input register values for analogics
        m_modbusServer->reMapInputRegisterValuesForAnalogics(m_realDataBufferLine);
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during data acquisition
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}

void NItoModbusBridge::onDataAcquisitionTimerTimeOut()
{
    try
    {
        // Trigger data acquisition
        acquireData();
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions that may occur during timer timeout
        // Log the error message for debugging purposes
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}


ThreadSafeCircularBuffer<std::vector<uint16_t>>& NItoModbusBridge::getSimulationBuffer() {
    return m_simulationBuffer;
}

// Getter for m_simulateTimer
std::shared_ptr<SimpleTimer> NItoModbusBridge::getSimulateTimer() const 
{
    return m_simulateTimer;
}

// Getter for m_dataAcquTimer
std::shared_ptr<SimpleTimer> NItoModbusBridge::getDataAcquTimer() const 
{
    return m_dataAcquTimer;
}

// Getter for m_modbusServer
std::shared_ptr<NewModbusServer> NItoModbusBridge::getModbusServer() const 
{
    return m_modbusServer;
}


// Getter for m_mappingData
const std::vector<MappingConfig>& NItoModbusBridge::getMappingData() const 
{
    return m_mappingData;
}