#include "iniObject.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include "cPosixFileHelper.h"

// IniObject Implementation
IniObject::IniObject()
{
    // TODO: Initialization if needed
}

IniObject::~IniObject()
{
    // TODO: Cleanup if needed
}

// Helper function to read a value from INI file
template <typename T>
T IniObject::readValue(const std::string& section, const std::string& key, T defaultValue, const std::string& currentFilename)
{
    // Check if the file exists
    if (isFileOk(currentFilename))
    {   
        // Create file object
        mINI::INIFile file(currentFilename);
        // Create a structure for handling data
        mINI::INIStructure ini;
        bool readOk = false;
        
        // Try to read the file
        readOk = file.read(ini);
        if (readOk)
        {
            // Get the value from the structure
            std::string& value = ini[section][key];
            if (value.empty())
            {
                // No value found
                if (!writeValue<T>(section, key, defaultValue, currentFilename))
                {
                    //TODO HANDLE ERROR
                }
                return defaultValue;
            }
            else
            {
                // Value found, return it
                try 
                {
                    std::istringstream iss(value);
                    T result;
                    iss >> result;
                    if (iss.fail())
                    {
                        return defaultValue;
                    }
                    return result;
                }
                catch (const std::exception& e) 
                {
                    return defaultValue;
                }
            }
        }
        else
        {
            // Reading failed
            return defaultValue;
        }
    }
    else
    {
        // File does not exist, return default value
        if (!writeValue<T>(section, key, defaultValue, currentFilename))
        {
            std::cout << "Impossible to generate default value: " << section << " " << key << " = " << defaultValue << std::endl;
        }
        return defaultValue;
    }
}

// Helper function to write a value to INI file
// Overload for std::string type
bool IniObject::writeValueHelper(const std::string& section, const std::string& key, const std::string& value, mINI::INIStructure& ini)
{
    ini[section][key] = value;
    return true;
}

// Overload for all other types
template <typename T>
bool IniObject::writeValueHelper(const std::string& section, const std::string& key, T value, mINI::INIStructure& ini)
{
    ini[section][key] = std::to_string(value);
    return true;
}



template <typename T>
bool IniObject::writeValue(const std::string& section, const std::string& key, T value, const std::string& currentFilename)
{
    // Check if the file exists
    if (isFileOk(currentFilename))
    {
        // Create file object
        mINI::INIFile file(currentFilename);
        // Create a structure for handling data
        mINI::INIStructure ini;
        bool readOk = false;
        
        // Try to read the existing file
        readOk = file.read(ini);
        if (readOk)
        {
            // Update the fields in the structure
            writeValueHelper(section, key, value, ini);
            
            // Update the file; check for write success
            if (file.write(ini)) 
            {
                //SUCCESS
                return true;
            } 
            else 
            {
                return false; // File write failed
            }
        }
        else
        {
            return false; // File read failed
        }
    }
    else
    {
        // The file does not exist; create an empty one
        if (createEmptyFile(currentFilename))
        {
            // Now that there's a file, retry writing the value
            return writeValue(section, key, value, currentFilename);
        }
        else
        {
            return false; // File creation failed
        }
    }
}


int IniObject::readInteger(const std::string &section, const std::string &key, int defaultValue, const std::string &currentFilename, bool &ok)
{
    try
    {
        ok = true;
        return readValue<int>(section, key, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        // Handle exception, maybe log it
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeInteger(const std::string &section, const std::string &key, int value, const std::string &currentFilename)
{
     try
    {
        return writeValue<int>(section, key, value, currentFilename);
    }
    catch (const std::exception& e)
    {
        // Handle exception, maybe log it
        return false;
    }
}

// Read and Write for Double
double IniObject::readDouble(const std::string& section, const std::string& key, double defaultValue, const std::string& currentFilename, bool &ok)
{
    try
    {
        ok = true;
        return readValue<double>(section, key, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeDouble(const std::string& section, const std::string& key, double value, const std::string& currentFilename)
{
    try
    {
        return writeValue<double>(section, key, value, currentFilename);
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

// Read and Write for String
std::string IniObject::readString(const std::string& section, const std::string& key, const std::string& defaultValue, const std::string& currentFilename, bool &ok)
{
    try
    {
        ok = true;
        return readValue<std::string>(section, key, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeString(const std::string& section, const std::string& key, const std::string& value, const std::string& currentFilename)
{
    try
    {
        return writeValue<std::string>(section, key, value, currentFilename);
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

// Read and Write for Unsigned Integer
unsigned int IniObject::readUnsignedInteger(const std::string& section, const std::string& key, unsigned int defaultValue, const std::string& currentFilename, bool &ok)
{
    try
    {
        ok = true;
        return readValue<unsigned int>(section, key, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeUnsignedInteger(const std::string& section, const std::string& key, unsigned int value, const std::string& currentFilename)
{
    try
    {
        return writeValue<unsigned int>(section, key, value, currentFilename);
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

// Read and Write for Boolean
bool IniObject::readBoolean(const std::string& section, const std::string& key, bool defaultValue, const std::string& currentFilename, bool &ok)
{
    try
    {
        std::string value = readValue<std::string>(section, key, defaultValue ? "true" : "false", currentFilename);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		ok = true;
        return (value == "true" || value == "1");
    }
    catch (const std::exception& e)
    {
        ok = false;
		return defaultValue;
    }
}

bool IniObject::writeBoolean(const std::string& section, const std::string& key, bool value, const std::string& currentFilename)
{
    try
    {
        return writeValue<std::string>(section, key, value ? "true" : "false", currentFilename);
    }
    catch (const std::exception& e)
    {
        return false;
    }
}


// Function to read a list of strings from a given section in INI file
bool IniObject::readStringVector(const std::string& section            ,
                                 const std::string& keyPrefix          ,
                                 const unsigned int& m_nbElements      ,
                                 std::vector<std::string>& vectorToFill,
                                 const std::string& currentFilename    ,
                                 bool &ok)
{
    
    std::vector<std::string> copy;
    // Clear the destination vector just to play paranoid
    copy.clear();
    // Reserve space in the destination vector to avoid reallocation
    copy.reserve(vectorToFill.size());
    // Loop through the source vector and copy each string
    for (const std::string& str : vectorToFill) {
        // Use the C-string (char*) of each string in the vector
        const char* cstr = str.c_str();
        // Create a new string in the destination vector with the same content
        copy.emplace_back(cstr);
    }

   if (isFileOk(currentFilename))
   {
       // Clear the channel names list
        vectorToFill.clear();
        // Populate the channel names list
        for (unsigned int i = 0; i < m_nbElements; ++i) 
        {
            std::string key   = keyPrefix + std::to_string(i);
            std::string value = this->readString(section,key,copy[i],currentFilename,ok);
            if (!ok)
            {
               appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,"bool IniObject::readStringVector Section:"+section+" Key:"+key+" for file "+currentFilename+" returned "+value+" it failed");  
            }
            vectorToFill.push_back(value);
        }
        ok = true;
        return true;
   }
   //if file is not ok the vector remain unchanged
   ok = false;
   return false;
}