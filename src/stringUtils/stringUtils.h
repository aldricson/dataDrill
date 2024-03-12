#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#include <iostream>
#include <cstring>
#include <string>
#include <algorithm> // for std::transform
#include <functional>
#include <sstream>
#include <vector>


static inline int extractNumberFromEnd(const std::string& str) {
    int i = str.size() - 1;
    // Iterate backwards to find the first non-numeric character
    while (i >= 0 && std::isdigit(str[i])) {
        --i;
    }

    // Extract the numeric part of the string
    std::string numberStr = str.substr(i + 1);

    // Convert the extracted string to an integer
    int number = 0;
    if (!numberStr.empty()) {
        number = std::stoi(numberStr);
    }

    return number;
}


static inline unsigned int extractChanIndex(const std::string& input) 
{
    // Assuming the format is always "/ai" followed by the index
    // and there are no leading zeros or spaces.
    try {
        if (input.size() > 3 && input.substr(0, 3) == "/ai") {
            std::string numberPart = input.substr(3); // Get the part after "/ai"
            return static_cast<unsigned int>(std::stoi(numberPart));
        } else {
            throw std::invalid_argument("Invalid format: " + input);
        }
    } catch (const std::invalid_argument& e) {
        // Handle case where conversion is not possible due to invalid input
        std::cerr << "Invalid argument: " << e.what() << '\n';
        throw;
    } catch (const std::out_of_range& e) {
        // Handle case where the number is too large
        std::cerr << "Out of range: " << e.what() << '\n';
        throw;
    }
}

static inline std::string removeSpacesFromCharStar(const char* str) {
    int length = strlen(str);
    std::string result;

    for (int i = 0; i < length; i++) {
        if (str[i] != ' ') {
            result += str[i];
        }
    }

    return result;
}



// Function to convert a string to lowercase
static inline std::string toLowerCase(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}



// Function to center align in text menus
static inline std::string centerAlignString(const std::string& str, unsigned int nbChars) {
    unsigned int totalSpaces = nbChars - str.length();
    unsigned int spacesBefore = totalSpaces / 2;
    unsigned int spacesAfter = totalSpaces - spacesBefore;
    std::string spacesBeforeStr(spacesBefore, ' ');
    std::string spacesAfterStr(spacesAfter, ' ');

    // Construct the result string
    std::string result =  "░" + spacesBeforeStr + str + spacesAfterStr + "░" ;

    return result;
}


static inline void displayMenu(const std::string& title,
                               const std::vector<std::string>& options)
{
        // Find the length of the longest string in options
    unsigned int maxLength = 0;
    for (const auto& option : options) {
        if (option.length() > maxLength) {
            maxLength = option.length();
        }
    }

    // Make sure maxLength is even for center alignment
    if (maxLength % 2 != 0) {
        maxLength++;
    }

    //Give some fresh air to the eyes
    maxLength+=4;

    //Create the top and bottom line
    std::string line;
    for (unsigned int i = 0; i < maxLength+2; ++i) line += "░";
    //spacer
    std::string spacer = centerAlignString(" ", maxLength);
    //1) Top line
    std::cout <<  line <<  std::endl;
    //2) spacer
    std::cout << spacer.c_str() << std::endl;
    //3) title 
    std::cout << centerAlignString(title, maxLength)<< std::endl;
    //4) spacer
    std::cout << spacer.c_str() << std::endl;
    //5) line ... Title zone done
    std::cout << line.c_str() << std::endl;
    // Output each centered option
    for (auto option : options) {
        // Append white spaces to make the option string length equal to maxLength
        option.append(maxLength - option.length(), ' ');
        std::cout << centerAlignString(option, maxLength) << std::endl;
    }
    // Bottom of the "table"
    std::cout << spacer.c_str() << std::endl;
    std::cout << line << std::endl;
}

static inline void handleUserInput(
        const std::vector<std::string>& options,
        const std::vector<std::function<void()>>& actions,
        const std::function<void()> &retryFunction)
    {
        std::string choice;
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        std::cin.clear();
        std::cin.ignore();

        unsigned int selectedChoice;
        //extract the user choice
        std::stringstream ss(choice);
        if (ss >> selectedChoice && ss.eof() && selectedChoice < actions.size()) {
            //here we have the index of the option selected, as the vector of options and actions are synchronized (with the help of god)
            //we just have to call the action with this index, it will automagically execute the function at this index. 
            actions[selectedChoice]();//<- note the () at the end.... it means we execute the function at the pointer found at the index in the vector
        } 
        else 
        {
            //if we are here or the user is stupid or he's a motha fucka, bacause he choose
            //an invalid option
            std::cout << "Invalid selection. Try again." << std::endl;
            retryFunction();
        }
    }

static inline void displayMenu(const std::string& title,
                               const std::vector<std::string>& options,
                               const std::vector<std::function<void()>>& actions,
                               const std::function<void()> &retryFunction) 
{
    displayMenu(title,options);
    handleUserInput(options,actions,retryFunction);
}

static inline void clearConsole()
{
    // ANSI escape sequence to clear screen for Unix-like systems
    std::cout << "\033[2J\033[1;1H";
}



static inline std::string concatenateRow(const std::vector<std::string>& row) {
    std::vector<std::vector<std::string>> linesInCells;

    // Split each cell into lines
    for (const auto& cell : row) {
        std::vector<std::string> lines;
        std::istringstream stream(cell);
        std::string line;
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        linesInCells.push_back(lines);
    }

    std::string result;

    // Find the maximum number of lines in any cell
    size_t maxLines = 0;
    for (const auto& lines : linesInCells) {
        maxLines = std::max(maxLines, lines.size());
    }

    // Concatenate lines from each cell
    for (size_t i = 0; i < maxLines; ++i) {
        std::string concatenatedLine;
        for (const auto& lines : linesInCells) {
            if (i < lines.size()) {
                concatenatedLine += lines[i];
            } else {
                // Fill with spaces if this cell has fewer lines
                concatenatedLine += std::string(lines[0].size(), ' ');
            }
        }
        result += concatenatedLine + "\n";
    }
    return result;
}


static inline unsigned int strToUnsignedInt(const std::string& str, bool& ok) {
    try {
        unsigned long ul = std::stoul(str);
        if (ul <= std::numeric_limits<unsigned int>::max()) {
            ok = true;  // Conversion successful
            return static_cast<unsigned int>(ul);
        } else {
            ok = false; // Value is too large for unsigned int
            return 0;   // Return a default value or handle the error as needed
        }
    } catch (const std::exception& e) {
        ok = false; // Exception occurred during conversion
        return 0;   // Return a default value or handle the error as needed
    }
}



static inline void showBanner()
{
    std::cout <<"          `^^^^^^^`              ______ _       _"<< std::endl;
  std::cout <<"      `4$$$c.$$$..e$$P\"         |  ____| |     | |            "<< std::endl;
  std::cout <<"    \"$$c.   `$b$F    .d$P\"      | |__  | |_   _| |_ ___  __ _ "<< std::endl;
  std::cout <<"  ^$$.      $$ d$\"      d$P     |  __| | | | | | __/ _ \\/ _` |"<< std::endl;
  std::cout <<" `\"$c.      4$.  $$       .$$   | |____| | |_| | ||  __/ (_| |"<< std::endl;
  std::cout <<"'$P$$$$$$$$$$$$$$$$$$$$$$$$$$   |______|_|\\__, |\\__\\___|\\__, |"<< std::endl;
  std::cout <<"$$e$P\"    $b     d$`    \"$$c$F             __/ |           | |"<< std::endl;
  std::cout <<"$b  .$$\" $$      .$$ \"4$b.  $$            |___/            |_|"<< std::endl;
  std::cout <<"$$     e$$F       4$$b.     $$ "<< std::endl;
  std::cout <<"$P     `$L$$P` `\"$$d$\"      $$ "<< std::endl;
  std::cout <<"d$     $$   z$$$e   $$     '$. "<< std::endl;
  std::cout <<" $F   *$. \"$$e.e$$\" 4$F   ^$b  "<< std::endl;
  std::cout <<"  .$$ 4$L*$$.     .$$Pd$  '$b  "<< std::endl;
  std::cout <<"   $$$$$.           .$$$*$.    "<< std::endl;
  std::cout <<"    .d$P"            "$$c      "<< std::endl;
  std::cout <<"       .d$$$******$$$$c.       "<< std::endl;
  std::cout <<"            ______             "<< std::endl << std::endl;


  std::cout << " dataDrill Headless version "<<std::endl;

}



#endif