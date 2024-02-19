#ifndef IniObject_H
#define IniObject_H

#include <string>
#include <stdexcept>
#include "ini.h"
#include "appendToFileHelper.h"
#include "../globals/globalEnumStructs.h"


class IniObject {
public:
    IniObject(); // Constructor
    ~IniObject(); // Destructor

    // Template method for reading values
    template <typename T>
    T readValue(const std::string& section, const std::string& key, T defaultValue, const std::string& currentFilename);

    // Template method for writing values
    // Helper function to write a value to INI file
    // Overload for std::string type
    bool writeValueHelper(const std::string& section, const std::string& key, const std::string& value, mINI::INIStructure& ini);
    // Overload for all other types
    template <typename T>
    bool writeValueHelper(const std::string& section, const std::string& key, T value, mINI::INIStructure& ini);
    
    template <typename T>
    bool writeValue(const std::string& section, const std::string& key, T value, const std::string& currentFilename);

    // Read and Write for Integer
    int  readInteger (const std::string& section, const std::string& key, int defaultValue, const std::string& currentFilename, bool &ok);
    bool writeInteger(const std::string& section, const std::string& key, int value, const std::string& currentFilename);

    // Read and Write for Double
    double readDouble(const std::string& section, const std::string& key, double defaultValue, const std::string& currentFilename, bool &ok);
    bool writeDouble(const std::string& section, const std::string& key, double value, const std::string& currentFilename);

    // Read and Write for String
    std::string readString(const std::string& section, const std::string& key, const std::string& defaultValue, const std::string& currentFilename, bool &ok);
    bool        writeString(const std::string& section, const std::string& key, const std::string& value, const std::string& currentFilename);

    // Read and Write for Unsigned Integer
    unsigned int readUnsignedInteger (const std::string& section, const std::string& key, unsigned int defaultValue, const std::string& currentFilename, bool &ok);
    bool         writeUnsignedInteger(const std::string& section, const std::string& key, unsigned int value, const std::string& currentFilename);

    // Read and Write for Boolean
    bool readBoolean(const std::string& section, const std::string& key, bool defaultValue, const std::string& currentFilename, bool &ok);
    bool writeBoolean(const std::string& section, const std::string& key, bool value, const std::string& currentFilename);

    bool readStringVector(const std::string& section,const std::string& keyPrefix,const unsigned int& m_nbElements,std::vector<std::string>& vectorToFill,const std::string& currentFilename, bool &ok);

protected:
    GlobalFileNamesContainer fileNamesContainer;
};

#endif // IniObject_H
