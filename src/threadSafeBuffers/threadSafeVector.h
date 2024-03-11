#ifndef THREAD_SAFE_VECTOR_H
#define THREAD_SAFE_VECTOR_H

#include <vector>
#include <mutex>

template <typename T>
class ThreadSafeVector {
public:
    ThreadSafeVector() = default;

    // Copy the contents of the vector to a new vector and return it
    std::vector<T> copy() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_;
    }

    // Replace the contents of the vector with the contents of another vector
    void restore(const std::vector<T>& source) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = source;
    }

    // Clear the contents of the vector
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
    }

    // Get the size of the vector
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<T> data_;
};

#endif // THREAD_SAFE_VECTOR_H
