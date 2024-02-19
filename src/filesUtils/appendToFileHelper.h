#ifndef APPENDCOMMENTWITHTIMESTAMP_H
#define APPENDCOMMENTWITHTIMESTAMP_H

#include <fstream>    // For file operations
#include <string>     // For std::string
#include <chrono>     // For system_clock
#include <iomanip>    // For put_time
#include <iostream>   // For cerr (error handling)
#include <sys/stat.h> // For stat, to check file existence in C++11 compatible way

// Function to check if a file exists
// Uses stat from sys/stat.h, which is compatible with C++11 and Linux environments
static inline bool appendCommentWithTimestampFileExists(const std::string& fileName) {
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
}

// Function to append a comment to a file, prepended with the current datetime.
// If the file does not exist, it will be created.
static inline void appendCommentWithTimestamp(const std::string& fileName, const std::string& comment) {
    // Check if the file exists
    if (!appendCommentWithTimestampFileExists(fileName)) 
    {
        // If the file does not exist, create an empty one to ensure the next operations succeed
        std::ofstream ofs(fileName, std::ios::out);
        ofs.close(); // Close the file after creating it
    }

    // Open the file in append mode
    std::ofstream file(fileName, std::ios::app);
    
    // Check if the file is open and ready for writing
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        return; // Exit the function if file cannot be opened
    }

    // Get the current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    // Format the current datetime
    std::tm bt = *std::localtime(&time);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &bt);

    // Prepend the formatted datetime to the comment
    std::string datetimeComment = std::string(buf) + " " + comment;

    // Append the datetime and comment to the file
    file << datetimeComment << std::endl;

    // Close the file
    file.close();
}

#endif // APPENDCOMMENTWITHTIMESTAMP_H
