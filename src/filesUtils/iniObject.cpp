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
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
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
            std::string& value = ini[tempSection][tempKey];
            if (value.empty())
            {
                // No value found
                if (!writeValue<T>(tempSection, tempKey, defaultValue, currentFilename))
                {
                    appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                              "in:\n"
                                              "template <typename T> \n"
                                              "T IniObject::readValue(const std::string& section, const std::string& key, T defaultValue, const std::string& currentFilename) \nSection:\n"+
                                              tempSection+
                                              "\nKey:\n"+
                                              tempKey+
                                              "\nfor file:\n"+
                                              currentFilename+
                                              "\nvalue is empty, writeValue failed.");  

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
        if (!writeValue<T>(tempSection, tempKey, defaultValue, currentFilename))
        {
           appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                         "in:\n"
                         "template <typename T> \n"
                         "T IniObject::readValue(const std::string& section, const std::string& key, T defaultValue, const std::string& currentFilename) \nSection:\n"+
                         tempSection+
                         "\nKey:\n"+
                         tempKey+
                         "\nfor file:\n"+
                         currentFilename+
                         "\nwriteValue failed.");  

        }
        return defaultValue;
    }
}

// Helper function to write a value to INI file
// Overload for std::string type
bool IniObject::writeValueHelper(const std::string& section, const std::string& key, const std::string& value, mINI::INIStructure& ini)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    ini[tempSection][tempKey] = value;
    return true;
}

// Overload for all other types
template <typename T>
bool IniObject::writeValueHelper(const std::string& section, const std::string& key, T value, mINI::INIStructure& ini)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    ini[section][key] = std::to_string(value);
    return true;
}



template <typename T>
bool IniObject::writeValue(const std::string& section, const std::string& key, T value, const std::string& currentFilename)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
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
            writeValueHelper(tempSection, tempKey, value, ini);
            
            // Update the file; check for write success
            if (file.write(ini)) 
            {
                //SUCCESS
                return true;
            } 
            else 
            {
                appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                            "in:\n"
                                            "template <typename T> \n"
                                            "bool IniObject::writeValue(const std::string& section, const std::string& key, T value, const std::string& currentFilename) \nSection:\n"+
                                            tempSection+
                                            "\nKey:\n"+
                                            tempKey+
                                            "\nfor file:\n"+
                                            currentFilename+
                                            "\nwriteValue failed."); 

                return false; // File write failed
            }
        }
        else
        {
            appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                      "in:\n"
                                      "template <typename T> \n"
                                      "bool IniObject::writeValue(const std::string& section, const std::string& key, T value, const std::string& currentFilename) \nSection:\n"+
                                      tempSection+
                                      "\nKey:\n"+
                                      tempKey+
                                      "\nfor file:\n"+
                                      currentFilename+
                                      "\nread Value before write failed."); 
            return false; // File read failed
        }
    }
    else
    {
        // The file does not exist; create an empty one
        if (createEmptyFile(currentFilename))
        {
            // Now that there's a file, retry writing the value
            return writeValue(tempSection, tempKey, value, currentFilename);
        }
        else
        {
            return false; // File creation failed
        }
    }
}


int IniObject::readInteger(const std::string &section, const std::string &key, int defaultValue, const std::string &currentFilename, bool &ok)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        ok = true;
        return readValue<int>(tempSection, tempKey, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        
        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "int IniObject::readInteger(const std::string &section, const std::string &key, int defaultValue, const std::string &currentFilename, bool &ok) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to read integer. Set to default: "+std::to_string(defaultValue)+
                                    "\nException :\n"+
                                    std::string(e.what())); 
    
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeInteger(const std::string &section, const std::string &key, int value, const std::string &currentFilename)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        return writeValue<int>(tempSection, tempKey, value, currentFilename);
    }
    catch (const std::exception& e)
    {
        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "bool IniObject::writeInteger(const std::string &section, const std::string &key, int value, const std::string &currentFilename) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to write integer: "+
                                   std::to_string(value)+
                                   "\nException :\n"+
                                   std::string(e.what()));   
        
        return false;
    }
}

// Read and Write for Double
double IniObject::readDouble(const std::string& section, const std::string& key, double defaultValue, const std::string& currentFilename, bool &ok)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        ok = true;
        return readValue<double>(tempSection, tempKey, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "double IniObject::readDouble(const std::string& section, const std::string& key, double defaultValue, const std::string& currentFilename, bool &ok) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to read double. Set to default: "+std::to_string(defaultValue)+
                                    "\nException :\n"+
                                    std::string(e.what())); 
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeDouble(const std::string& section, const std::string& key, double value, const std::string& currentFilename)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        
        return writeValue<double>(tempSection, tempKey, value, currentFilename);
    }
    catch (const std::exception& e)
    {
        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "bool IniObject::writeDouble(const std::string& section, const std::string& key, double value, const std::string& currentFilename) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to write double: "+
                                   std::to_string(value)+
                                   "\nException :\n"+
                                   std::string(e.what()));   
        return false;
    }
}

// Read and Write for String
std::string IniObject::readString(const std::string& section, const std::string& key, const std::string& defaultValue, const std::string& currentFilename, bool &ok)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        ok = true;
        return readValue<std::string>(tempSection, tempKey, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "std::string IniObject::readString(const std::string& section, const std::string& key, const std::string& defaultValue, const std::string& currentFilename, bool &ok) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to read string. Set to default: "+defaultValue+
                                    "\nException :\n"+
                                    std::string(e.what()));
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeString(const std::string& section, const std::string& key, const std::string& value, const std::string& currentFilename)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        return writeValue<std::string>(tempSection, tempKey, value, currentFilename);
    }
    catch (const std::exception& e)
    {

        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "bool IniObject::writeString(const std::string& section, const std::string& key, const std::string& value, const std::string& currentFilename) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to write string: "+
                                   value+
                                   "\nException :\n"+
                                   std::string(e.what())); 
        return false;
    }
}

// Read and Write for Unsigned Integer
unsigned int IniObject::readUnsignedInteger(const std::string& section, const std::string& key, unsigned int defaultValue, const std::string& currentFilename, bool &ok)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        ok = true;
        return readValue<unsigned int>(tempSection, tempKey, defaultValue, currentFilename);
    }
    catch (const std::exception& e)
    {
        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "unsigned int IniObject::readUnsignedInteger(const std::string& section, const std::string& key, unsigned int defaultValue, const std::string& currentFilename, bool &ok) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to read unsigned integer. Set to default: "+std::to_string(defaultValue)+
                                    "\nException :\n"+
                                    std::string(e.what()));
        
        ok = false;
        return defaultValue;
    }
}

bool IniObject::writeUnsignedInteger(const std::string& section, const std::string& key, unsigned int value, const std::string& currentFilename)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        return writeValue<unsigned int>(tempSection, tempKey, value, currentFilename);
    }
    catch (const std::exception& e)
    {
        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "bool IniObject::writeUnsignedInteger(const std::string& section, const std::string& key, unsigned int value, const std::string& currentFilename) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to write unsigned integer:\n"+
                                   std::to_string(value)+
                                   "\nException :\n"+
                                   std::string(e.what())); 
        return false;
    }
}

// Read and Write for Boolean
bool IniObject::readBoolean(const std::string& section, const std::string& key, bool defaultValue, const std::string& currentFilename, bool &ok)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    try
    {
        std::string value = readValue<std::string>(tempSection, tempKey, defaultValue ? "true" : "false", currentFilename);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		ok = true;
        return (value == "true" || value == "1");
    }
    catch (const std::exception& e)
    {
       std::string boolStr;
       defaultValue ? boolStr = true : boolStr = false;
       appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "bool IniObject::readBoolean(const std::string& section, const std::string& key, bool defaultValue, const std::string& currentFilename, bool &ok) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to read boolean. Set to default: "+boolStr+
                                    "\nException :\n"+
                                    std::string(e.what()));
        ok = false;
		return defaultValue;
    }
}

bool IniObject::writeBoolean(const std::string& section, const std::string& key, bool value, const std::string& currentFilename)
{
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKey = key;
    std::transform(tempKey.begin(), tempKey.end(), tempKey.begin(), ::tolower);
    std::string valueAsString;
    value ?  valueAsString = "true" : valueAsString = "false";
    try
    {
        return writeValue<std::string>(tempSection, tempKey, valueAsString, currentFilename);
    }
    catch (const std::exception& e)
    {
        std::string boolStr;
        value ? boolStr = true : boolStr = false;

        appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                   "in:\n"
                                   "bool IniObject::writeBoolean(const std::string& section, const std::string& key, bool value, const std::string& currentFilename) \nSection:\n"+
                                   tempSection+
                                   "\nKey:\n"+
                                   tempKey+
                                   "\nfor file:\n"+
                                   currentFilename+
                                   "\nimpossible to write boolean:\n"+
                                   boolStr+
                                   "\nException :\n"+
                                   std::string(e.what())); 
        
        
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
    
    std::string tempSection = section;
    std::transform(tempSection.begin(), tempSection.end(), tempSection.begin(), ::tolower);
    std::string tempKeyPrefix = keyPrefix;
    std::transform(tempKeyPrefix.begin(), tempKeyPrefix.end(), tempKeyPrefix.begin(), ::tolower);
    
    
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
            std::string key   = tempKeyPrefix + std::to_string(i);
            std::string value = this->readString(tempSection,key,copy[i],currentFilename,ok);
            if (!ok)
            {
               value = "failed";
              appendCommentWithTimestamp(fileNamesContainer.iniObjectLogFile,
                                         "in:\n"
                                         "bool IniObject::readStringVector(const std::string& section            ,\n"
                                         "                                 const std::string& keyPrefix          ,\n"
                                         "                                 const unsigned int& m_nbElements      ,\n"
                                         "                                 std::vector<std::string>& vectorToFill,\n"
                                         "                                 const std::string& currentFilename    ,\n"
                                         "                                 bool &ok) \nSection:\n"+
                                         tempSection+
                                         "\nKey:\n"+
                                         key+
                                         "\nfor file:\n"+
                                         currentFilename+
                                         "\nimpossible to set value. Item set to 'failed'\n");


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