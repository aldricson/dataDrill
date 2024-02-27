#include "NIDeviceModule.h"

NIDeviceModule::NIDeviceModule()

{
    m_ini = std::make_shared<IniObject>();
}

bool NIDeviceModule::loadChannels(const std::string &filename, const ModuleType &aModuleType)
{
    bool ok = false;    
    bool result = true;
    // Reads and sets the number of channels from the specified file.
    unsigned int numChannels = m_ini->readUnsignedInteger("channels",
                                                          "numberofchannels",
                                                          m_nbChannel,
                                                          filename,
                                                          ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadChannels(std::string filename) read 'Channels' 'NumberOfChannels' failed");
        result = false;
    }

    if (numChannels <= 0)
    {
        if (aModuleType == ModuleType::isAnalogicInputCurrent || aModuleType == ModuleType::isAnalogicInputVoltage)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in bool NIDeviceModule::loadChannels(std::string filename) Channel count <= 0");
            result = false;
        }
        else
        {
            std::cout << "No channel for " << filename << " it's ok" << std::endl;
        }
    }

    setNbChannel(numChannels);
    // Reads and sets the maximum analog channel value from the file.
    double analogMax = m_ini->readDouble("channels",
                                         "max",
                                         m_analogChanMax,
                                         filename,
                                         ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadChannels(std::string filename) read 'channels' 'max' failed for " + filename);
        result = false;
    }

    double analogMin = m_ini->readDouble("channels",
                                         "min",
                                         m_analogChanMax,
                                         filename,
                                         ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadChannels(std::string filename) read 'Channels' 'min' failed for " + filename);
        result = false;
    }

    if ((aModuleType == ModuleType::isAnalogicInputCurrent || aModuleType == ModuleType::isAnalogicInputVoltage) && (analogMax <= analogMin))
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadChannels(std::string filename) analog max value <= analog min value for " + filename);
        result = false;
    }

    setChanMax(analogMax);
    // Reads and sets the minimum analog channel value from the file.
    setChanMin(analogMin);

    // Reads a vector of channel names from the file and checks if the operation was successful.
    bool responded = m_ini->readStringVector("channels",
                                             "channel",
                                             m_nbChannel,
                                             m_chanNames,
                                             filename,
                                             ok);
    // if all is true that might be the returned result else ... result will be false
    return (responded && result && ok);
}

bool NIDeviceModule::loadCounters(const std::string &filename, const ModuleType &aModuleType)
{
    bool ok = false;
    // Check if the filename is empty
    if (filename.empty())
    {
        std::cerr << "Error: Filename is empty in loadCounters." << std::endl;
        return false;
    }

    // Load the number of counters from the file
    unsigned int numCounters = m_ini->readUnsignedInteger("counters",
                                                          "numberofcounters",
                                                          m_nbCounters,
                                                          filename,
                                                          ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadCounters(const std::string &filename, const ModuleType &aModuleType) read 'COUNTERS' 'NUMBEROFCOUNTERS' failed");
    }

    if (numCounters == 0 && aModuleType==ModuleType::isCounter)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadCounters(const std::string &filename, const ModuleType &aModuleType)  No counters for "+
                                    filename);
        setNbCounters(4);
        return false;
    }

    if (numCounters < 0  && aModuleType==ModuleType::isCounter)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadCounters(const std::string &filename) read 'COUNTERS' 'NUMBEROFCOUNTERS' \n"
                                   "numCounters < 0: the file is corrupted.");
        setNbCounters(0);
        return false;
    }

    // Load the counter names
    bool namesOk = m_ini->readStringVector("counters",
                                           "counter",
                                           numCounters,
                                           m_counterNames,
                                           filename,
                                           ok);
    if ((!namesOk || m_counterNames.empty()) && aModuleType==ModuleType::isCounter)
    {
        std::cout << "counter names or list is empty for" << filename << std::endl;
        // This might not be a critical error, depends on application requirements
    }

    // Load and set counter edge counting mode
    int edgeMode = m_ini->readInteger("counters",
                                      "edgecountingmode",
                                      static_cast<int>(m_counterCountingEdgeMode),
                                      filename,
                                      ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadCounters(const std::string &filename) read 'Counters' 'edgeCountingMode' failed");
    }

    // Additional validation can be added here based on the expected range of edgeMode
    setcounterCountingEdgeMode(static_cast<moduleCounterEdgeConfig>(edgeMode));

    // Load and set counter counting direction mode
    int countDirection = m_ini->readInteger("counters",
                                            "countingdirection",
                                            static_cast<int>(m_counterCountDirectionMode),
                                            filename,
                                            ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadCounters(const std::string &filename) read 'Counters' 'countingDirection' failed");
    }
    // Additional validation can be added here based on the expected range of countDirection
    setCounterCountDirectionMode(static_cast<moduleCounterMode>(countDirection));

    // Load and set counter max and min ensuring max is greater than or equal to min
    unsigned int counterMax = m_ini->readUnsignedInteger("counters",
                                                         "countingmax",
                                                         4294967295,
                                                         filename,
                                                         ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadCounters(const std::string &filename) read 'Counters' 'countingMax' failed");
    }

    unsigned int counterMin = m_ini->readUnsignedInteger("counters",
                                                         "countingmin",
                                                         0,
                                                         filename,
                                                         ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadCounters(const std::string &filename) read 'Counters' 'countingMin' failed");
    }

    if (counterMin > counterMax)
    {
        std::cerr << "Error: Counter minimum value is greater than the maximum value." << std::endl;
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "Error counter min value is > counter max value.");
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "set min/max to defaults");
        setCounterMax(counterMax);
        setCounterMin(4294967295);
        return false;
    }
    setCounterMax(counterMax);
    setCounterMin(counterMin);

    // If this point is reached, all data is successfully loaded
    return true;
}

bool NIDeviceModule::loadOutputs(const std::string &filename, const ModuleType &aModuleType)
{
    // TODO
    return false;
}

bool NIDeviceModule::loadModules(const std::string &filename, ModuleType &aModuleType)
{
    bool ok = false;
    // Ensure the filename is not empty
    if (filename.empty())
    {
        std::cerr << "Error: Filename is empty in loadModules." << std::endl;
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool bool NIDeviceModule::loadModules(const std::string &filename) file name is empty");
        return false;
    }

    // Read and set the module type
    int moduleType = m_ini->readInteger("modules",
                                        "type",
                                        static_cast<int>(m_moduleType),
                                        filename,
                                        ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadModules(const std::string &filename) read 'Modules' 'type' failed");
    }
    // Directly casting to ModuleType enum. Ensure this cast is safe according to your enum definition.
    aModuleType = static_cast<ModuleType>(moduleType);
    setModuleType(aModuleType);

    // Read and set the module name
    std::string moduleName = m_ini->readString("modules",
                                               "modulename",
                                               m_moduleName,
                                               filename,
                                               ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadModules(const std::string &filename) read 'Modules' 'moduleName' failed for file " + filename);
    }

    if (moduleName.empty())
    {
        std::cerr << "Error: Module name is empty." << std::endl;
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadModules(const std::string &filename) module name is empty");
        return false;
    }
    setModuleName(moduleName);

    // Read and set the module alias
    std::string moduleAlias = m_ini->readString("modules",
                                                "alias",
                                                m_alias,
                                                filename,
                                                ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadModules(const std::string &filename) read 'Modules' 'Alias' failed for file " + filename);
    }
    setAlias(moduleAlias);

    // Read and set the module shunt location
    int shuntLocation = m_ini->readInteger("modules",
                                           "shuntlocation",
                                           static_cast<int>(m_shuntLocation),
                                           filename,
                                           ok);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadModules(const std::string &filename) read 'Modules' 'shuntLocation' failed for file " + filename);
    }

    // Directly casting to moduleShuntLocation enum. Ensure this cast is safe according to your enum definition.
    setModuleShuntLocation(static_cast<moduleShuntLocation>(shuntLocation));

    // Read and set the module shunt value
    double shuntValue = m_ini->readDouble("modules",
                                          "shuntvalue",
                                          m_shuntValue,
                                          filename,
                                          ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in bool NIDeviceModule::loadModules(const std::string &filename) read 'Modules' 'shuntValue' failed for file " + filename);
    }
    setModuleShuntValue(shuntValue);

    // Read and set the module terminal configuration
    int terminalConfig = m_ini->readInteger("modules",
                                            "terminalconfig",
                                            static_cast<int>(m_moduleTerminalConfig),
                                            filename,
                                            ok);

    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in\n"
                                   "bool NIDeviceModule::loadModules(const std::string &filename) read 'Modules' 'terminalConfig' \nfailed for file:\n" + filename);
    }
    // Directly casting to moduleTerminalConfig enum. Ensure this cast is safe according to your enum definition.
    setModuleTerminalCfg(static_cast<moduleTerminalConfig>(terminalConfig));

    // Omitted setting 'moduleUnit' as it's not defined

    return true;
}

void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)
{
    // Check if the filename is empty
    if (filename.empty())
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in \n"
                                   "void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)\n" 
                                   "failed to save: file name is empty ");
        return;
    }

    // Check if the number of channels is valid
    if (m_nbChannel == 0 || m_chanNames.size() != m_nbChannel) 
    {
        if (aModuleType==ModuleType::isAnalogicInputCurrent || aModuleType==ModuleType::isAnalogicInputVoltage)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: Number of channels is zero or mismatch with channel names vector.\n"
                                       "file name:\n"+filename);
        }
        return;
    }

    // Check if max channel value is greater than min channel value
    if (m_analogChanMax <= m_analogChanMin)
    {
        if (aModuleType==ModuleType::isAnalogicInputCurrent || aModuleType==ModuleType::isAnalogicInputVoltage)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: Max channel value is less than or equal to min channel value.\n"
                                       "file name:\n"+filename);
            return;
        }
    }


    if (aModuleType==ModuleType::isAnalogicInputCurrent || aModuleType==ModuleType::isAnalogicInputVoltage)
    {
        bool ok;
        // Write the number of channels, max and min values to the file
        ok = m_ini->writeUnsignedInteger("channels", "numberofchannels", m_nbChannel, filename);
        if (!ok)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: impossible to write 'channels' 'numberofchannels'\n"
                                       "file name:\n"+filename);
        }
        ok = m_ini->writeDouble         ("channels", "max"             , m_analogChanMax, filename);
        if (!ok)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: impossible to write 'channels' 'max'\n"
                                       "file name:\n"+filename);
        }
        ok = m_ini->writeDouble         ("channels", "min"             , m_analogChanMin, filename);
        if (!ok)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: impossible to write 'channels' 'min'\n"
                                       "file name:\n"+filename);           
        }

        // Save the channel names
        for (unsigned int i = 0; i < m_nbChannel; ++i)
        {
            std::string key = "channel" + std::to_string(i);

            // Check if channel name is not empty
            if (m_chanNames[i].empty())
            {
                appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                           "in\n"
                                           "void NIDeviceModule::saveChannels(const std::string &filename, const ModuleType &aModuleType)\n"
                                           "Error: Channel name at index "+std::to_string(i)+" is empty.\n"
                                           "file name:\n"+filename); 
                continue; // Skipping empty channel names
            }

            m_ini->writeString("channels", key.c_str(), m_chanNames[i], filename);
        }
    }
}

void NIDeviceModule::saveCounters(const std::string &filename, const ModuleType &aModuleType)
{
    // Ensure the filename is not empty
    if (filename.empty())
    {
        std::cerr << "Error: Filename is empty in saveCounters." << std::endl;
        return;
    }

    // Validate the number of counters
    if ((m_nbCounters == 0 || m_counterNames.size() != m_nbCounters) && aModuleType == ModuleType::isCounter)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in\n"
                                   "void NIDeviceModule::saveCounters(const std::string &filename, const ModuleType &aModuleType)\n"
                                   "Error: Number of counters is zero or mismatch with counter names vector.\n"
                                   "file name:\n"+filename);
        return;
    }

    // Write the number of counters to the file
    m_ini->writeUnsignedInteger("counters", "numberofcounters", m_nbCounters, filename);

    // Iterate through counter names and save them
    for (unsigned int i = 0; i < m_nbCounters; ++i)
    {
        std::string key = "counter" + std::to_string(i);

        // Skip saving empty counter names
        if (m_counterNames[i].empty())
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveCounters(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: Counter name at index "+std::to_string(i)+" is empty.\n"
                                       "file name:\n"+filename);
            continue;
        }
        else
        {
            m_ini->writeString("counters", key.c_str(), m_counterNames[i], filename);
        }
    }

    if (m_nbCounters > 0 && aModuleType == ModuleType::isCounter)
    {
        bool ok;
        // Write counter edge counting mode
        ok = m_ini->writeInteger("counters", "edgecountingmode", static_cast<int>(m_counterCountingEdgeMode), filename);
        if (!ok)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveCounters(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: impossible to write 'counters', 'edgecountingmode'\n"
                                       "file name:\n"+filename);
        }

        // Write counter counting direction mode
        m_ini->writeInteger("counters", "countingdirection", static_cast<int>(m_counterCountDirectionMode), filename);
        if (!ok)
        {
            appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveCounters(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: impossible to write 'counters', 'countingdirection'\n"
                                       "file name:\n"+filename);
        }

        // Validate and write counter max and min values
        if (m_counterMax < m_counterMin)
        {
           appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                       "in\n"
                                       "void NIDeviceModule::saveCounters(const std::string &filename, const ModuleType &aModuleType)\n"
                                       "Error: Counter maximum value is less than the minimum value.'\n"
                                       "file name:\n"+filename);
            return;
        }
        m_ini->writeUnsignedInteger("counters", "countingmax", m_counterMax, filename);
        m_ini->writeUnsignedInteger("counters", "countingmin", m_counterMin, filename);

    }
}

void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType)
{
    // Check if the filename is empty
    if (filename.empty())
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to save: file name is empty ");
        return;
    }

    // Write the module type to the file
    try
    {
        aModuleType = m_moduleType;
        m_ini->writeInteger("modules", "type", static_cast<int>(m_moduleType), filename);
    }
    catch (const std::exception &e)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to write 'MODULES' 'TYPE' in " +
                                       filename +
                                       " with exception: " +
                                       std::string(e.what()));
        return;
    }

    // Write the module name to the file, checking for emptiness
    if (m_moduleName.empty())
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) Error: m_moduleName is empty");
        return;
    }
    bool ok;
    ok = m_ini->writeString("modules", "modulename", m_moduleName, filename);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to write 'MODULES' 'MODULENAME' for " +
                                       filename);
    }

    // Write the alias to the file, allowing empty alias as it might not be critical
    ok = m_ini->writeString("modules", "alias", m_alias, filename);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to write 'MODULES' 'ALIAS' for " +
                                       filename);
    }

    // Write the shunt location to the file
    ok = m_ini->writeInteger("modules", "shuntlocation", static_cast<int>(m_shuntLocation), filename);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to write 'MODULES' 'SHUNTLOCATION' for " +
                                       filename);
    }

    // Write the shunt value to the file, assuming no specific validation required
    ok = m_ini->writeDouble("modules", "shuntvalue", m_shuntValue, filename);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to write 'MODULES' 'SHUNTVALUE' for " +
                                       filename);
    }

    // Write the terminal configuration to the file
    ok = m_ini->writeInteger("modules", "terminalconfig", static_cast<int>(m_moduleTerminalConfig), filename);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to write 'MODULES' 'TERMINALCONFIG' for " +
                                       filename);
    }

    // Write the module unit to the file
    ok = m_ini->writeInteger("modules", "moduleunit", static_cast<int>(m_moduleUnit), filename);
    if (!ok)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::saveModules(const std::string &filename, ModuleType &aModuleType) failed to write 'MODULES' 'MODULEUNIT' for " +
                                       filename);
    }

    // Optionally, log that modules are saved
    // std::cout << "Modules saved for " << m_moduleName << std::endl;
}

void NIDeviceModule::loadFromFile(const std::string &filename)
{
    // Check if the filename is empty before proceeding
    if (filename.empty())
    {
        std::cerr << "Error: Filename is empty in loadFromFile." << std::endl;
        return;
    }

    // Boolean flags to track loading status
    ModuleType modType;
    bool modulesLoaded = loadModules(filename, modType);
    bool channelsLoaded = loadChannels(filename, modType);
    bool countersLoaded = loadCounters(filename, modType);

    // Logging success or failure of each loading function
    if (channelsLoaded)
    {
        std::cout << "Channels information successfully loaded from the ini file." << std::endl;
    }
    else
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::loadFromFile(const std::string &filename) Failed to load channel information from the ini file: " +
                                       filename);
    }

    if (countersLoaded)
    {
        std::cout << "Counter information successfully loaded from the ini file." << std::endl;
    }
    else
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::loadFromFile(const std::string &filename) Failed to load counters information from the ini file" +
                                       filename);
    }

    if (modulesLoaded)
    {
        std::cout << "Modules information successfully loaded from the ini file." << std::endl;
    }
    else
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::loadFromFile(const std::string &filename) Failed to load modules information from the ini file" +
                                       filename);
    }

    // If any of the load functions failed, handle accordingly
    if (!channelsLoaded || !countersLoaded || !modulesLoaded)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void NIDeviceModule::loadFromFile(const std::string &filename) One or more components failed to load properly." +
                                       filename);
    }

    // Optionally, log overall success if all components loaded successfully
    if (channelsLoaded && countersLoaded && modulesLoaded)
    {
        std::cout << "All components successfully loaded from " << filename << std::endl;
    }
}

void NIDeviceModule::saveToFile(const std::string &filename)
{
    // Check if the filename is empty before proceeding
    if (filename.empty())
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void void NIDeviceModule::saveToFile(const std::string &filename) filename is empty");
        return;
    }

    // Boolean flags to track saving status
    bool channelsSaved = true, countersSaved = true, modulesSaved = true;
    ModuleType modType;

    try
    {
        saveModules(filename, modType);
    }
    catch (const std::exception &e)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void void NIDeviceModule::saveToFile(const std::string &filename) Exception occurred in saveModules for "+
                                   filename+
                                   " Exception: "+
                                   std::string(e.what()));
        modulesSaved = false;
    }

    try
    {
        saveChannels(filename,modType);
    }
    catch (const std::exception &e)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void void NIDeviceModule::saveToFile(const std::string &filename) Exception occurred in saveChannels for "+
                                   filename+
                                   " Exception: "+
                                   std::string(e.what()));
        channelsSaved = false;
    }

    try
    {
        saveCounters(filename,modType);
    }
    catch (const std::exception &e)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void void NIDeviceModule::saveToFile(const std::string &filename) Exception occurred in saveCounters for "+
                                   filename+
                                   " Exception: "+
                                   std::string(e.what()));
        countersSaved = false;
    }

    // If any of the save functions failed, handle accordingly
    if (!channelsSaved || !countersSaved || !modulesSaved)
    {
        appendCommentWithTimestamp(m_fileNamesContainer.niDeviceModuleLogFile,
                                   "in void void NIDeviceModule::saveToFile(const std::string &filename) one or more components failed to save properly");
    }

    // Optionally, log overall success if all components saved successfully
    if (channelsSaved && countersSaved && modulesSaved)
    {
        std::cout << "All components successfully saved to " << filename << std::endl;
    }
}

std::string NIDeviceModule::getAlias()
{
    return m_alias;
}

void NIDeviceModule::setAlias(const std::string &newAlias)
{
    m_alias = newAlias;
}

void NIDeviceModule::setChanNames(const std::vector<std::string> &names)
{
    m_chanNames = names;
    if (chanNamesChangedSignal)
    {
        chanNamesChangedSignal(m_chanNames, this);
    }
}

void NIDeviceModule::setCounterNames(const std::vector<std::string> &names)
{
    m_counterNames = names;
    if (counterNamesChangedSignal)
    {
        counterNamesChangedSignal(m_counterNames, this);
    }
}

void NIDeviceModule::setModuleType(ModuleType newType)
{
    m_moduleType = newType;
}

void NIDeviceModule::setChanMin(double newChanMin)
{
    if (newChanMin < m_analogChanMax)
    {
        m_analogChanMin = newChanMin;
        if (chanMinChangedSignal)
        {
            chanMinChangedSignal(m_analogChanMin, this);
        }
    }
}

void NIDeviceModule::setcounterCountingEdgeMode(moduleCounterEdgeConfig newCounterCountingEdgeMode)
{
    m_counterCountingEdgeMode = newCounterCountingEdgeMode;
    if (counterEdgeConfigChangedSignal)
    {
        counterEdgeConfigChangedSignal(newCounterCountingEdgeMode, this);
    }
}

void NIDeviceModule::setCounterCountDirectionMode(moduleCounterMode newCounterCountMode)
{
    m_counterCountDirectionMode = newCounterCountMode;
    if (counterModeChangedSignal)
    {
        counterModeChangedSignal(newCounterCountMode, this);
    }
}

void NIDeviceModule::setChanMax(double newChanMax)
{
    if (newChanMax > m_analogChanMin)
    {
        m_analogChanMax = newChanMax;
        if (chanMinChangedSignal)
        {
            chanMinChangedSignal(m_analogChanMax, this);
        }
    }
}

void NIDeviceModule::setCounterMin(unsigned int newCountersMin)
{
    if (newCountersMin < m_counterMax)
    {
        m_counterMin = newCountersMin;
        if (countersMinChangedSignal)
        {
            countersMinChangedSignal(m_counterMin, this);
        }
    }
}

void NIDeviceModule::setCounterMax(unsigned int newCountersMax)
{
    if (newCountersMax > m_counterMin)
    {
        m_counterMax = newCountersMax;
        // perfectly equivalent to the Q_EMIT
        if (countersMaxChangedSignal)
        {
            countersMaxChangedSignal(m_counterMax, this);
        }
    }
}

void NIDeviceModule::setNbDigitalOutputs(unsigned int newNbDigitalOutputs)
{
    m_nbDigitalOutputs = newNbDigitalOutputs;
    if (nbDigitalOutputsChangedSignal)
    {
        nbDigitalOutputsChangedSignal(newNbDigitalOutputs, this);
    }
}

void NIDeviceModule::setModuleUnit(moduleUnit newUnit)
{
    m_moduleUnit = newUnit;
    if (chanUnitChangedSignal)
    {
        chanUnitChangedSignal(m_moduleUnit, this);
    }
}

std::string NIDeviceModule::getModuleName() const
{
    return m_moduleName;
}

unsigned int NIDeviceModule::getNbChannel() const
{
    return m_nbChannel;
}

unsigned int NIDeviceModule::getNbCounters() const
{
    return m_nbCounters;
}

unsigned int NIDeviceModule::getSlotNb() const
{
    return m_slotNumber;
}

unsigned int NIDeviceModule::getNbDigitalIOPorts() const
{
    return m_nbDigitalIoPort;
}

std::string NIDeviceModule::getModuleInfo() const
{
    return m_moduleInfo;
}

std::vector<std::string> NIDeviceModule::getChanNames() const
{
    return m_chanNames;
}

std::vector<std::string> NIDeviceModule::getCounterNames() const
{
    return m_counterNames;
}

ModuleType NIDeviceModule::getModuleType() const
{
    return m_moduleType;
}

moduleShuntLocation NIDeviceModule::getModuleShuntLocation() const
{
    return m_shuntLocation;
}

moduleCounterEdgeConfig NIDeviceModule::getcounterCountingEdgeMode() const
{
    return m_counterCountingEdgeMode;
}

double NIDeviceModule::getModuleShuntValue() const
{
    return m_shuntValue;
}

moduleCounterMode NIDeviceModule::getCounterCountDirectionMode() const
{
    return m_counterCountDirectionMode;
}

unsigned int NIDeviceModule::getNbDigitalOutputs() const
{
    return m_nbDigitalOutputs;
}

moduleTerminalConfig NIDeviceModule::getModuleTerminalCfg() const
{
    return m_moduleTerminalConfig;
}

double NIDeviceModule::getChanMin() const
{
    return m_analogChanMin;
}

double NIDeviceModule::getChanMax() const
{
    return m_analogChanMax;
}

unsigned int NIDeviceModule::getminCounters() const
{
    return m_counterMin;
}

unsigned int NIDeviceModule::getmaxCounters() const
{
    return m_counterMax;
}

moduleUnit NIDeviceModule::getModuleUnit() const
{
    return m_moduleUnit;
}

void NIDeviceModule::setModuleName(const std::string &newModuleName)
{
    m_moduleName = newModuleName;
    if (moduleNameChangedSignal)
    {
        moduleNameChangedSignal(m_moduleName, this);
    }
}

void NIDeviceModule::setNbChannel(unsigned int newNbChannels)
{
    m_nbChannel = newNbChannels;
    // emit signal
    if (nbChannelsChangedSignal)
    {
        nbChannelsChangedSignal(newNbChannels, this);
    }
}

void NIDeviceModule::setNbCounters(unsigned int newNbCounters)
{
    m_nbCounters = newNbCounters;
    if (nbCountersChangedSignal)
    {
        nbCountersChangedSignal(m_nbCounters, this);
    }
}

void NIDeviceModule::setNbDigitalIOPorts(unsigned int newNbPorts)
{
    m_nbDigitalIoPort = newNbPorts;
    // emit signal
    if (nbDigitalIoPortsChangedSignal)
    {
        nbDigitalIoPortsChangedSignal(newNbPorts, this);
    }
}

void NIDeviceModule::setModuleInfo(std::string newModuleInfo)
{
    m_moduleInfo = newModuleInfo;
    // emit signal
    if (moduleInfoChangedSignal)
    {
        moduleInfoChangedSignal(newModuleInfo, this);
    }
}

void NIDeviceModule::setModuleShuntLocation(moduleShuntLocation newLocation)
{
    m_shuntLocation = newLocation;
    if (moduleShuntLocationChangedSgnal)
    {
        moduleShuntLocationChangedSgnal(m_shuntLocation, this);
    }
}

void NIDeviceModule::setModuleShuntValue(double newValue)
{
    m_shuntValue = newValue;
    if (moduleShuntValueChangedSignal)
    {
        moduleShuntValueChangedSignal(m_shuntValue, this);
    }
}

void NIDeviceModule::setModuleTerminalCfg(moduleTerminalConfig newTerminalConfig)
{
    m_moduleTerminalConfig = newTerminalConfig;
    if (moduleTerminalConfigChangedSignal)
    {
        moduleTerminalConfigChangedSignal(m_moduleTerminalConfig, this);
    }
}

void NIDeviceModule::setSlotNb(unsigned int newSlot)
{
    m_slotNumber = newSlot;
    // if the signal is connected then emit it
    if (moduleSlotNumberChangedSignal)
    { // Check if the signal is connected to a slot
        moduleSlotNumberChangedSignal(newSlot, this);
    }
}
