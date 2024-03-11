#include "ThreadSafeCircularBuffer.h"

// Implementation of the locked_iterator class
template <typename T>
class ThreadSafeCircularBuffer<T>::locked_iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;

    locked_iterator(pointer ptr, std::unique_ptr<std::lock_guard<std::mutex>> lock)
        : m_ptr(ptr), m_lock(std::move(lock)) {}

    locked_iterator(const locked_iterator&) = delete;

    locked_iterator(locked_iterator&& other) noexcept
        : m_ptr(other.m_ptr), m_lock(std::move(other.m_lock)) {}

    reference operator*() const { return *m_ptr; }
    pointer operator->() { return m_ptr; }
    locked_iterator& operator++() { m_ptr++; return *this; }
    locked_iterator operator++(int) { locked_iterator tmp = std::move(*this); ++(*this); return tmp; }
    friend bool operator== (const locked_iterator& a, const locked_iterator& b) { return a.m_ptr == b.m_ptr; }
    friend bool operator!= (const locked_iterator& a, const locked_iterator& b) { return a.m_ptr != b.m_ptr; }

private:
    pointer m_ptr;
    std::unique_ptr<std::lock_guard<std::mutex>> m_lock;
};

// Constructor
template <typename T>
ThreadSafeCircularBuffer<T>::ThreadSafeCircularBuffer()
    : buffer_() {}

// Push an item to the back of the buffer
template <typename T>
bool ThreadSafeCircularBuffer<T>::push_back(const T& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer_.full()) buffer_.pop_front();
    buffer_.push_back(item);
    m_currentPosition++;
    return true;
}

// Pop an item from the front of the buffer
template <typename T>
bool ThreadSafeCircularBuffer<T>::pop_front(T& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer_.empty()) {
        return false;
    }
    item = buffer_.front();
    buffer_.pop_front();
    m_currentPosition--;
    return true;
}



// Get the beginning iterator
template <typename T>
typename ThreadSafeCircularBuffer<T>::locked_iterator ThreadSafeCircularBuffer<T>::begin() {
    auto lock = std::make_unique<std::lock_guard<std::mutex>>(mutex_);
    return locked_iterator(&buffer_.front(), std::move(lock));
}

// Get the ending iterator
template <typename T>
typename ThreadSafeCircularBuffer<T>::locked_iterator ThreadSafeCircularBuffer<T>::end() {
    auto lock = std::make_unique<std::lock_guard<std::mutex>>(mutex_);
    return locked_iterator(&buffer_.back() + 1, std::move(lock));
}

// Copy the contents of the buffer to a vector
template <typename T>
std::vector<T> ThreadSafeCircularBuffer<T>::copy() const {
    // Lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    // Initialize an empty vector to store the result
    std::vector<T> result;
    // Loop through the circular buffer and copy each element to the vector
    for (size_t i = 0; i < buffer_.size(); ++i) {
        result.push_back(buffer_[i]);
    }
    // Return the copied vector
    return result;
}



// Restore the buffer from a vector
template <typename T>
void ThreadSafeCircularBuffer<T>::restore(const std::vector<T>& source) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
    for (const T& item : source) {
        buffer_.push_back(item);
    }
}

// Get the current position
template <typename T>
int ThreadSafeCircularBuffer<T>::getCurrentPosition() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return m_currentPosition;
}

// Clear the buffer
template <typename T>
void ThreadSafeCircularBuffer<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
}

// Overload the [] operator
template <typename T>
T ThreadSafeCircularBuffer<T>::operator[](size_t i) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_[i];
}

// Explicit instantiation for the types you'll use
template class ThreadSafeCircularBuffer<std::vector<unsigned short>>;
