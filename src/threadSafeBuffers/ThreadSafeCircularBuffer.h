#ifndef THREAD_SAFE_CIRCULAR_BUFFER_H
#define THREAD_SAFE_CIRCULAR_BUFFER_H

#include <cstddef>
#include <vector>
#include <mutex>
#include "circular_buffer.h"  // Include your circular_buffer header

template <typename T>
class ThreadSafeCircularBuffer {
public:
    // Forward declaration of the nested iterator class
    class locked_iterator;

    // Constructor
    ThreadSafeCircularBuffer();

    // Push an item to the back of the buffer
    bool push_back(const T& item);

    // Pop an item from the front of the buffer
    bool pop_front(T& item);

    // Get the beginning iterator
    locked_iterator begin();

    // Get the ending iterator
    locked_iterator end();

    // Copy the contents of the buffer to a vector
    std::vector<T> copy() const;

    // Restore the buffer from a vector
    void restore(const std::vector<T>& source);

    // Get the current position
    int getCurrentPosition() const;

    // Clear the buffer
    void clear();

    // Overload the [] operator
    T operator[](size_t i) const;

private:
    mutable std::mutex mutex_;  // Mutex for thread safety
    circular_buffer<T, 11> buffer_;  // Your circular_buffer class
    int m_currentPosition = 0;  // Current position in the buffer
};

#endif  // THREAD_SAFE_CIRCULAR_BUFFER_H
