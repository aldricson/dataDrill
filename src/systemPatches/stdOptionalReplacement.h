#include <stdexcept>

template <typename T>
class stdOptionalReplacement {
private:
    T value_;
    bool has_value_;

public:
    // Default constructor.
    stdOptionalReplacement() : has_value_(false) {}

    // Constructor from value.
    stdOptionalReplacement(const T& value) : value_(value), has_value_(true) {}

    // Check if value exists.
    bool has_value() const {
        return has_value_;
    }

    // Retrieve the value.
    T value() const {
        if (!has_value_) {
            throw std::runtime_error("No value");
        }
        return value_;
    }

    // Equality and inequality with sentinel value for uninitialized state.
    bool operator==(const stdOptionalReplacement<T>& other) const {
        if (has_value_ != other.has_value_) {
            return false;
        }
        if (!has_value_) {
            return true;  // Both are uninitialized.
        }
        return value_ == other.value_;
    }

    bool operator!=(const stdOptionalReplacement<T>& other) const {
        return !(*this == other);
    }

    // Converting to bool for easier conditional checks.
    explicit operator bool() const {
        return has_value_;
    }
};
