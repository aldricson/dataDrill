
///////////////////////////////////////////////////////////////////////////////
//
//  for manipulating INI files with a straightforward  API and a minimal 
//  footprint. It conforms to the (somewhat) standard INI format.
//  sections and keys are case insensitive and all leading and
//  trailing whitespace is ignored. Comments are lines that begin with a
//  semicolon. Trailing comments are allowed on section lines.
//
//  Files are read on demand, upon which data is kept in memory and the file
//  is closed. This utility supports lazy writing, which only writes changes
//  and updates to a file and preserves custom formatting and comments. A lazy
//  write invoked by a write() call will read the output file, find what
//  changes have been made and update the file accordingly. If you only need to
//  generate files, use generate() instead. Section and key order is preserved
//  on read, write and insert.
//
///////////////////////////////////////////////////////////////////////////////
//
//  /* BASIC USAGE EXAMPLE: */
//
//  /* read from file */
//  mINI::INIFile file("myfile.ini");
//  mINI::INIStructure ini;
//  file.read(ini);
//
//  /* read value; gets a reference to actual value in the structure.
//     if key or section don't exist, a new empty value will be created */
//  std::string& value = ini["section"]["key"];
//
//  /* read value safely; gets a copy of value in the structure.
//     does not alter the structure */
//  std::string value = ini.get("section").get("key");
//
//  /* set or update values */
//  ini["section"]["key"] = "value";
//
//  /* set multiple values */
//  ini["section2"].set({
//      {"key1", "value1"},
//      {"key2", "value2"}
//  });
//
//  /* write updates back to file, preserving comments and formatting */
//  file.write(ini);
//
//  /* or generate a file (overwrites the original) */
//  file.generate(ini);
//
///////////////////////////////////////////////////////////////////////////////
//
//  Long live the INI file!!!
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MINI_INI_H_
#define MINI_INI_H_

#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <unordered_map>
#include <vector>
#include <memory>
#include <fstream>
#include <sys/stat.h>
#include <cctype>

namespace mINI
{
	namespace INIStringUtil
	{
		const char* const whitespaceDelimiters = " \t\n\r\f\v";
		inline void trim(std::string& str)
		{
			str.erase(str.find_last_not_of(whitespaceDelimiters) + 1);
			str.erase(0, str.find_first_not_of(whitespaceDelimiters));
		}
#ifndef MINI_CASE_SENSITIVE
		inline void toLower(std::string& str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](const char c) {
				return static_cast<char>(std::tolower(c));
			});
		}
#endif
		inline void replace(std::string& str, std::string const& a, std::string const& b)
		{
			if (!a.empty())
			{
				std::size_t pos = 0;
				while ((pos = str.find(a, pos)) != std::string::npos)
				{
					str.replace(pos, a.size(), b);
					pos += b.size();
				}
			}
		}
#ifdef _WIN32
		const char* const endl = "\r\n";
#else
		const char* const endl = "\n";
#endif
	}

	template<typename T>
	class INIMap
	{
	private:
		using T_DataIndexMap = std::unordered_map<std::string, std::size_t>;
		using T_DataItem = std::pair<std::string, T>;
		using T_DataContainer = std::vector<T_DataItem>;
		using T_MultiArgs = typename std::vector<std::pair<std::string, T>>;

		T_DataIndexMap dataIndexMap;
		T_DataContainer data;

		inline std::size_t setEmpty(std::string& key)
		{
			std::size_t index = data.size();
			dataIndexMap[key] = index;
			data.emplace_back(key, T());
			return index;
		}

	public:
		using const_iterator = typename T_DataContainer::const_iterator;

		INIMap() { }
//------------------
        // Copy constructor for INIMap
        INIMap(INIMap const& other)
        {
            // Get the size of the 'data' vector from the 'other' object to be copied
            std::size_t data_size = other.data.size();
            // Loop through each element in the 'data' vector of the 'other' object
            for (std::size_t i = 0; i < data_size; ++i)
            {
                // Retrieve the key and object (value) for the current element in the 'data' vector
                auto const& key = other.data[i].first;
                auto const& obj = other.data[i].second;
                // Use emplace_back to add a new element to the 'data' vector of the current object.
                // This is more efficient than push_back because it constructs the element in place.
                data.emplace_back(key, obj);
            }
            // Directly copy the 'dataIndexMap' from the 'other' object to the current object.
            // This map holds the index positions of keys in the 'data' vector.
            // Since we've copied all elements from 'other.data' to 'data', the indices should remain valid.
            dataIndexMap = T_DataIndexMap(other.dataIndexMap);
        }
//-------------------------
		// Overloaded operator[] to access the value associated with the given key in the data map
       T& operator[](std::string key)
       {
          // Trim any leading and trailing whitespaces from the key
          INIStringUtil::trim(key);
          // Convert the key to lowercase if case sensitivity is disabled
          // The macro MINI_CASE_SENSITIVE controls this behavior
          #ifndef MINI_CASE_SENSITIVE
          INIStringUtil::toLower(key);
          #endif
          // Try to find the key in the index map
          auto it = dataIndexMap.find(key);
          // Check if the key was found
          bool hasIt = (it != dataIndexMap.end());
          // Determine the index to access in the 'data' vector
          // If the key exists, use the index stored in the map
          // Otherwise, create a new entry with an empty value
          std::size_t index = (hasIt) ? it->second : setEmpty(key);
          // Return the value associated with the key
          // The value is stored as the second element of the pair in the 'data' vector
          return data[index].second;
        }
//-------------------------
		// Method to safely retrieve the value associated with a given key, if it exists
        T get(std::string key) const
        {
           // Trim any leading and trailing whitespaces from the key
           INIStringUtil::trim(key);
           // Convert the key to lowercase if case sensitivity is disabled
           // The macro MINI_CASE_SENSITIVE controls this behavior
           #ifndef MINI_CASE_SENSITIVE
           INIStringUtil::toLower(key);
           #endif
          // Try to find the key in the dataIndexMap
          auto it = dataIndexMap.find(key);
          // Check if the key was found in dataIndexMap
          if (it == dataIndexMap.end())
          {
            // If the key was not found, return a default-constructed object of type T
            return T();
          }
          // If the key was found, retrieve the associated value stored in the 'data' vector
          // The value is stored as the second element of the pair
          // A copy of the value is made by explicitly constructing an object of type T
          return T(data[it->second].second);
       }
//-------------------------------
		// Method to check if a given key exists in the data structure
        bool has(std::string key) const
        {
            // Trim any leading and trailing whitespaces from the key
            INIStringUtil::trim(key);
            // Convert the key to lowercase if case sensitivity is disabled
            // The macro MINI_CASE_SENSITIVE controls this behavior
            #ifndef MINI_CASE_SENSITIVE
                INIStringUtil::toLower(key);
            #endif
            // Use the count function to check for the key's existence in dataIndexMap
            // The count function returns 1 if the key exists, and 0 otherwise
            return (dataIndexMap.count(key) == 1);
        }

//---------------------------

        // Method to set the value of a given key in the data structure
        void set(std::string key, T obj)
        {
            // Trim any leading and trailing whitespaces from the key
            INIStringUtil::trim(key);
        
            // Convert the key to lowercase if case sensitivity is disabled
            // The macro MINI_CASE_SENSITIVE controls this behavior
        #ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
        #endif
        
            // Find the key in the dataIndexMap
            auto it = dataIndexMap.find(key);
        
            // If the key already exists in the dataIndexMap
            if (it != dataIndexMap.end())
            {
                // Update the value associated with the key in the data vector
                data[it->second].second = obj;
            }
            else
            {
                // If the key doesn't exist, add it to dataIndexMap and set its value
                // as the current size of the data vector
                dataIndexMap[key] = data.size();
        
                // Add the key-value pair to the data vector
                data.emplace_back(key, obj);
            }
        }

//---------------------------

        // Method to set multiple key-value pairs using a T_MultiArgs container
        void set(T_MultiArgs const& multiArgs)
        {
            // Loop through each key-value pair in the T_MultiArgs container
            for (auto const& it : multiArgs)
            {
                // Extract the key and value from the current iterator
                auto const& key = it.first;
                auto const& obj = it.second;        
                // Use the single key-value set function to insert or update the key-value pair
                set(key, obj);
            }
        }

//---------------------------

        // Method to remove a key-value pair by its key
        bool remove(std::string key)
        {
            // Trim any leading or trailing white spaces from the key
            INIStringUtil::trim(key);
            // If case-insensitive, convert the key to lower case
            #ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
            #endif
            // Try to find the key in the index map
            auto it = dataIndexMap.find(key);
            // If key is found
            if (it != dataIndexMap.end())
            {
                // Get the index of the key-value pair in the data vector
                std::size_t index = it->second;
                // Remove the key-value pair from the data vector
                data.erase(data.begin() + index);
                // Remove the key from the index map
                dataIndexMap.erase(it);
                // Update the indices in the index map that are greater than the removed index
                for (auto& it2 : dataIndexMap)
                {
                    auto& vi = it2.second;
                    if (vi > index)
                    {
                        vi--;
                    }
                }
                // Return true, indicating the key-value pair was successfully removed
                return true;
            }
            // If key is not found, return false
            return false;
        }

//------------------------------

		void clear()
		{
			data.clear();
			dataIndexMap.clear();
		}
		std::size_t size() const
		{
			return data.size();
		}
		const_iterator begin() const { return data.begin(); }
		const_iterator end() const { return data.end(); }
	};

	using INIStructure = INIMap<INIMap<std::string>>;

	namespace INIParser
	{
		using T_ParseValues = std::pair<std::string, std::string>;

		enum class PDataType : char
		{
			PDATA_NONE,
			PDATA_COMMENT,
			PDATA_SECTION,
			PDATA_KEYVALUE,
			PDATA_UNKNOWN
		};

		// Function to parse a single line of the INI file
       inline PDataType parseLine(std::string line, T_ParseValues& parseData)
       {
           // Clear the previous data in the pair that will hold the parsed values
           parseData.first.clear();
           parseData.second.clear();
           // Remove leading and trailing whitespaces from the line
           INIStringUtil::trim(line);
           // If the line is empty, return PDATA_NONE to indicate that the line is empty
           if (line.empty())
           {
               return PDataType::PDATA_NONE;
           }
           // Get the first character of the line to determine its type
           char firstCharacter = line[0];
           // If the first character is a semicolon, it's a comment line
           if (firstCharacter == ';')
           {
               return PDataType::PDATA_COMMENT;
           }
           // If the first character is an opening bracket, it could be a section header
           if (firstCharacter == '[')
           {
               // Find the position of a comment within the line, if any
               auto commentAt = line.find_first_of(';');
               if (commentAt != std::string::npos)
               {
                   // Remove the comment portion from the line
                   line = line.substr(0, commentAt);
               }
               // Find the position of the closing bracket for the section header
               auto closingBracketAt = line.find_last_of(']');
               if (closingBracketAt != std::string::npos)
               {
                   // Extract the section name and trim it
                   auto section = line.substr(1, closingBracketAt - 1);
                   INIStringUtil::trim(section);
                   // Assign the section name to the first part of the parsed data
                   parseData.first = section;
                   // Return PDATA_SECTION to indicate it's a section header
                   return PDataType::PDATA_SECTION;
               }
           }
           // Make a copy of the line and replace occurrences of "\=" with spaces
           // This is to handle escaped equal signs in keys
           auto lineNorm = line;
           INIStringUtil::replace(lineNorm, "\\=", "  ");
           // Find the position of the first equal sign in the normalized line
           auto equalsAt = lineNorm.find_first_of('=');
           if (equalsAt != std::string::npos)
           {
               // Extract the key and the value, then trim both
               auto key = line.substr(0, equalsAt);
               INIStringUtil::trim(key);
               INIStringUtil::replace(key, "\\=", "=");  // Replace back the escaped equal signs in the key
               auto value = line.substr(equalsAt + 1);
               INIStringUtil::trim(value);
               // Assign the key and value to the parsed data
               parseData.first = key;
               parseData.second = value;
               // Return PDATA_KEYVALUE to indicate it's a key-value pair
               return PDataType::PDATA_KEYVALUE;
           }
           // If the function has not returned by this point, return PDATA_UNKNOWN to indicate an unknown line type
           return PDataType::PDATA_UNKNOWN;
       }

	}

	class INIReader
	{
	public:
		using T_LineData = std::vector<std::string>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;

		bool isBOM = false;

	private:
		std::ifstream fileReadStream;
		T_LineDataPtr lineData;
        //function to read a whole ini file
		T_LineData readFile()
        {
            // Move the read position to the end of the file to get the file size
            fileReadStream.seekg(0, std::ios::end);
            const std::size_t fileSize = static_cast<std::size_t>(fileReadStream.tellg());
        
            // Reset the read position to the beginning of the file
            fileReadStream.seekg(0, std::ios::beg);
        
            // Check for Byte Order Mark (BOM) for UTF-8 at the beginning of the file
            if (fileSize >= 3) {
                const char header[3] = {
                    static_cast<char>(fileReadStream.get()),
                    static_cast<char>(fileReadStream.get()),
                    static_cast<char>(fileReadStream.get())
                };
                isBOM = (
                    header[0] == static_cast<char>(0xEF) &&
                    header[1] == static_cast<char>(0xBB) &&
                    header[2] == static_cast<char>(0xBF)
                );
            }
            else {
                // If the file size is less than 3 bytes, it can't have a BOM
                isBOM = false;
            }
            // Initialize a string to hold the file contents
            std::string fileContents;
            fileContents.resize(fileSize);
            // Position the read pointer based on whether a BOM exists or not
            fileReadStream.seekg(isBOM ? 3 : 0, std::ios::beg);
            // Read the file content into the string
            fileReadStream.read(&fileContents[0], fileSize);
            // Close the file stream
            fileReadStream.close();
            // Initialize the output container for the line data
            T_LineData output;
            // If the file is empty, return an empty output container
            if (fileSize == 0)
            {
                return output;
            }
            // Initialize a buffer string to construct each line
            std::string buffer;
            buffer.reserve(50);
            // Loop through each character in the file content
            for (std::size_t i = 0; i < fileSize; ++i)
            {
                char& c = fileContents[i];
                // If a newline character is found, add the buffer as a new line to the output
                if (c == '\n')
                {
                    output.emplace_back(buffer);
                    buffer.clear();
                    continue;
                }
                // Add non-null and non-carriage return characters to the buffer
                if (c != '\0' && c != '\r')
                {
                    buffer += c;
                }
            }
            // Add the last line to the output if there's any content left in the buffer
            output.emplace_back(buffer);
            // Return the line-by-line output
            return output;
        }

	public:
		INIReader(std::string const& filename, bool keepLineData = false)
		{
			fileReadStream.open(filename, std::ios::in | std::ios::binary);
			if (keepLineData)
			{
				lineData = std::make_shared<T_LineData>();
			}
		}
		~INIReader() { }

		// Overloaded operator>> function to read an INI file and populate an INIStructure object
        bool operator>>(INIStructure& data)
        {
            // Check if the file stream is open; return false if not
            if (!fileReadStream.is_open())
            {
                return false;
            }
        
            // Read the file content into a line-by-line data structure
            T_LineData fileLines = readFile();
            // Initialize variables to keep track of the current section and parsing status
            std::string section;
            bool inSection = false;
            INIParser::T_ParseValues parseData;
            // Loop through each line in the file
            for (auto const& line : fileLines)
            {
                // Parse the current line and get the result type (section, key-value, etc.)
                auto parseResult = INIParser::parseLine(line, parseData);
                // Check if the line defines a new section
                if (parseResult == INIParser::PDataType::PDATA_SECTION)
                {
                    inSection = true; // We are now inside a section
                    data[section = parseData.first]; // Initialize the section in the data structure
                }
                // Check if the line is a key-value pair inside a section
                else if (inSection && parseResult == INIParser::PDataType::PDATA_KEYVALUE)
                {
                    auto const& key = parseData.first;
                    auto const& value = parseData.second;
                    data[section][key] = value; // Add the key-value pair to the current section
                }
                // If lineData exists and the line is not of unknown type
                if (lineData && parseResult != INIParser::PDataType::PDATA_UNKNOWN)
                {
                    // Skip adding key-value lines that are outside of any section
                    if (parseResult == INIParser::PDataType::PDATA_KEYVALUE && !inSection)
                    {
                        continue;
                    }
                    // Add the line to lineData
                    lineData->emplace_back(line);
                }
            }
            // Return true to indicate successful parsing and data population
            return true;
        }
//----------------------

       //function to return lineData
		T_LineDataPtr getLines()
		{
			return lineData;
		}
	};

	class INIGenerator
	{
	private:
		std::ofstream fileWriteStream;

	public:
		bool prettyPrint = false;

		INIGenerator(std::string const& filename)
		{
			fileWriteStream.open(filename, std::ios::out | std::ios::binary);
		}
		~INIGenerator() { }

		// Overloaded operator<< function to write the INIStructure object back to an INI file
        bool operator<<(INIStructure const& data)
        {
            // Check if the file write stream is open; return false if not
            if (!fileWriteStream.is_open())
            {
                return false;
            }
            // If there's no data to write, return true
            if (!data.size())
            {
                return true;
            }
            // Initialize an iterator for the data map
            auto it = data.begin();
            // Loop through each section in the INIStructure
            for (;;)
            {
                auto const& section = it->first;
                auto const& collection = it->second;
                // Write the section header to the file
                fileWriteStream << "[" << section << "]";
                // If the section has key-value pairs, proceed to write them
                if (collection.size())
                {
                    fileWriteStream << INIStringUtil::endl;
                    // Initialize an iterator for the key-value pairs in the section
                    auto it2 = collection.begin();
                    // Loop through each key-value pair in the section
                    for (;;)
                    {
                        auto key = it2->first;
                        INIStringUtil::replace(key, "=", "\\=");
                        auto value = it2->second;
                        INIStringUtil::trim(value);
                        // Write the key-value pair to the file
                        fileWriteStream << key << ((prettyPrint) ? " = " : "=") << value;
                        // If we've reached the end of the key-value pairs, break
                        if (++it2 == collection.end())
                        {
                            break;
                        }
                        // Add a new line after each key-value pair
                        fileWriteStream << INIStringUtil::endl;
                    }
                }
                // If we've reached the end of the sections, break
                if (++it == data.end())
                {
                    break;
                }
                // Add a new line after each section
                fileWriteStream << INIStringUtil::endl;
                // If prettyPrint is enabled, add an additional new line for better readability
                if (prettyPrint)
                {
                    fileWriteStream << INIStringUtil::endl;
                }
            }
            // Return true to indicate successful writing
            return true;
        }

	};

	class INIWriter
	{
	private:
		using T_LineData = std::vector<std::string>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;

		std::string filename;

		T_LineData getLazyOutput(T_LineDataPtr const& lineData, INIStructure& data, INIStructure& original)
		{
            // Declare variables for storing output lines, parsed data, and various flags
            T_LineData output;  // Container for storing the lines that will make up the output INI file
            INIParser::T_ParseValues parseData;  // Struct to hold key-value pairs or section names parsed from each line
            std::string sectionCurrent;  // Holds the name of the current section being parsed
            bool parsingSection = false;  // Flag indicating if we are currently parsing a known section
            bool continueToNextSection = false;  // Flag indicating if we should skip lines until the next section
            bool discardNextEmpty = false;  // Flag indicating if the next empty line should be discarded
            bool writeNewKeys = false;  // Flag indicating if new keys should be written to the output
            std::size_t lastKeyLine = 0;  // Stores the index of the last line where a key was written in the output
            // Loop through each line in the input data
			for (auto line = lineData->begin(); line != lineData->end(); ++line)
            {
               // Handle normal parsing if not flagged to write new keys
              if (!writeNewKeys)
              {
                // Parse the current line
                auto parseResult = INIParser::parseLine(*line, parseData);
                // If it's a section line, handle accordingly
                if (parseResult == INIParser::PDataType::PDATA_SECTION)
                {
                // If already parsing a section, flag to write new keys
						if (parsingSection)
						{
							writeNewKeys = true;
							parsingSection = false;
							--line;
							continue;
						}
						sectionCurrent = parseData.first;
						if (data.has(sectionCurrent))
						{
							parsingSection = true;
							continueToNextSection = false;
							discardNextEmpty = false;
							output.emplace_back(*line);
							lastKeyLine = output.size();
						}
						else
						{
							continueToNextSection = true;
							discardNextEmpty = true;
							continue;
						}
				}
				else if (parseResult == INIParser::PDataType::PDATA_KEYVALUE)
				{
						if (continueToNextSection)
						{
							continue;
						}
						if (data.has(sectionCurrent))
						{
							auto& collection = data[sectionCurrent];
							auto const& key = parseData.first;
							auto const& value = parseData.second;
							if (collection.has(key))
							{
								auto outputValue = collection[key];
								if (value == outputValue)
								{
									output.emplace_back(*line);
								}
								else
                                {
                                   // Trim any leading or trailing whitespace from the new output value
                                   INIStringUtil::trim(outputValue);
                                   // Copy the original line to a new string for normalization
                                   auto lineNorm = *line;
                                   // Replace escaped equals signs for easier processing
                                   INIStringUtil::replace(lineNorm, "\\=", "  ");
                                   // Find the position of the first equals sign in the normalized line
                                   auto equalsAt = lineNorm.find_first_of('=');
                                   // Find the position of the first non-whitespace character after the equals sign
                                   auto valueAt = lineNorm.find_first_not_of(
                                       INIStringUtil::whitespaceDelimiters,
                                       equalsAt + 1
                                   );
                                   // Create a new output line, starting with the original line up to the value
                                   std::string outputLine = line->substr(0, valueAt);
                                   // If pretty printing is enabled and there's no space after the equals sign, add one
                                   if (prettyPrint && equalsAt + 1 == valueAt)
                                   {
                                      outputLine += " ";
                                   }
                                   // Append the new value to the output line
                                   outputLine += outputValue;
                                   // Add the new line to the output container
                                   output.emplace_back(outputLine);
                                }  
								lastKeyLine = output.size();
							}
						}
					}
					else
					{
						if (discardNextEmpty && line->empty())
						{
							discardNextEmpty = false;
						}
						else if (parseResult != INIParser::PDataType::PDATA_UNKNOWN)
						{
							output.emplace_back(*line);
						}
					}
				}
				if (writeNewKeys || std::next(line) == lineData->end())
				{
					T_LineData linesToAdd;
					if (data.has(sectionCurrent) && original.has(sectionCurrent))
					{
						auto const& collection = data[sectionCurrent];
						auto const& collectionOriginal = original[sectionCurrent];
						for (auto const& it : collection)
						{
							auto key = it.first;
							if (collectionOriginal.has(key))
							{
								continue;
							}
							auto value = it.second;
							INIStringUtil::replace(key, "=", "\\=");
							INIStringUtil::trim(value);
							linesToAdd.emplace_back(
								key + ((prettyPrint) ? " = " : "=") + value
							);
						}
					}
					if (!linesToAdd.empty())
					{
						output.insert(
							output.begin() + lastKeyLine,
							linesToAdd.begin(),
							linesToAdd.end()
						);
					}
					if (writeNewKeys)
					{
						writeNewKeys = false;
						--line;
					}
				}
			}
			for (auto const& it : data)
			{
				auto const& section = it.first;
				if (original.has(section))
				{
					continue;
				}
				if (prettyPrint && output.size() > 0 && !output.back().empty())
				{
					output.emplace_back();
				}
				output.emplace_back("[" + section + "]");
				auto const& collection = it.second;
				for (auto const& it2 : collection)
				{
					auto key = it2.first;
					auto value = it2.second;
					INIStringUtil::replace(key, "=", "\\=");
					INIStringUtil::trim(value);
					output.emplace_back(
						key + ((prettyPrint) ? " = " : "=") + value
					);
				}
			}
			return output;
		}

	public:
		bool prettyPrint = false;

		INIWriter(std::string const& filename)
		: filename(filename)
		{
		}
		~INIWriter() { }

		bool operator<<(INIStructure& data)
		{
			struct stat buf;
			bool fileExists = (stat(filename.c_str(), &buf) == 0);
			if (!fileExists)
			{
				INIGenerator generator(filename);
				generator.prettyPrint = prettyPrint;
				return generator << data;
			}
			INIStructure originalData;
			T_LineDataPtr lineData;
			bool readSuccess = false;
			bool fileIsBOM = false;
			{
				INIReader reader(filename, true);
				if ((readSuccess = reader >> originalData))
				{
					lineData = reader.getLines();
					fileIsBOM = reader.isBOM;
				}
			}
			if (!readSuccess)
			{
				return false;
			}
			T_LineData output = getLazyOutput(lineData, data, originalData);
			std::ofstream fileWriteStream(filename, std::ios::out | std::ios::binary);
			if (fileWriteStream.is_open())
			{
				if (fileIsBOM) {
					const char utf8_BOM[3] = {
						static_cast<char>(0xEF),
						static_cast<char>(0xBB),
						static_cast<char>(0xBF)
					};
					fileWriteStream.write(utf8_BOM, 3);
				}
				if (output.size())
				{
					auto line = output.begin();
					for (;;)
					{
						fileWriteStream << *line;
						if (++line == output.end())
						{
							break;
						}
						fileWriteStream << INIStringUtil::endl;
					}
				}
				return true;
			}
			return false;
		}
	};

	class INIFile
	{
	private:
		std::string filename;

	public:
		INIFile(std::string const& filename)
		: filename(filename)
		{ }

		~INIFile() { }

		bool read(INIStructure& data) const
		{
			if (data.size())
			{
				data.clear();
			}
			if (filename.empty())
			{
				return false;
			}
			INIReader reader(filename);
			return reader >> data;
		}


		bool generate(INIStructure const& data, bool pretty = false) const
		{
			if (filename.empty())
			{
				return false;
			}
			INIGenerator generator(filename);
			generator.prettyPrint = pretty;
			return generator << data;
		}

		bool write(INIStructure& data, bool pretty = false) const
		{
			if (filename.empty())
			{
				return false;
			}
			INIWriter writer(filename);
			writer.prettyPrint = pretty;
			return writer << data;
		}
	};
}

#endif // MINI_INI_H_