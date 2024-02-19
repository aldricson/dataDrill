#ifndef cPosixFileHelper_h
#define cPosixFileHelper_h

#include <fcntl.h>  // for open, fcntl
#include <unistd.h> // for close, access
#include <sys/stat.h> // for file permissions
#include <string>

// Function to check if a file is accessible and not locked
static inline bool isFileOk(const std::string& fileName)
{
    // First, check if the file exists and is accessible
    if (access(fileName.c_str(), F_OK) != 0) {
        // File does not exist or is not accessible
        return false;
    }

    // Open the file in read-only mode to check for other issues (like being locked)
    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd == -1) {
        // Failed to open the file, possibly locked or another error
        return false;
    }

    // Close the file descriptor
    close(fd);
    return true;
}

// Function to create an empty file with appropriate error checks and permissions
static inline bool createEmptyFile(const std::string& fileName)
{
    // Set file permissions to read-write for the owner, and read-only for others
    mode_t filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    // Open a new file with write permission, create if it doesn't exist, and truncate it if it does
    int fd = open(fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, filePerms);

    if (fd == -1) {
        // Failed to create the file, possibly due to permissions or filesystem issues
        return false;
    }

    // Close the file descriptor, effectively creating an empty file
    close(fd);
    return true;
}

#endif