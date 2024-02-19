#ifndef SimpleTimer_h
#define SimpleTimer_h

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>

class SimpleTimer {
public:
    // Default constructor
    SimpleTimer() : m_interval(std::chrono::milliseconds(1000)), m_slotFunction(nullptr) 
    {
        m_active.store(false);
    }

    // Destructor
    ~SimpleTimer() {
        stop();
    }

    // Starts the timer
    void start() {
        if (m_slotFunction == nullptr || m_interval.count() <= 0) {
            std::cerr << "Error: Slot function or interval not set." << std::endl;
            return;
        }

        m_active = true;
        m_timerThread = std::thread([this]() {
            while (m_active.load()) {
                std::this_thread::sleep_for(m_interval);
                if (m_active.load()) {  // Check again before calling the slot function
                    m_slotFunction();
                }
            }
        });
    }

    // Stops the timer
void stop() 
{
    m_active.store(false);
    if (m_timerThread.joinable()) 
    {
        m_timerThread.join();
    }
}


    // Sets the slot function
    void setSlotFunction(std::function<void()> slotFunction) 
    {
        m_slotFunction = slotFunction;
    }

    // Gets the slot function
    std::function<void()> getSlotFunction() const {
        return m_slotFunction;
    }

    // Sets the interval
    void setInterval(std::chrono::milliseconds interval) {
        m_interval = interval;
    }

    // Gets the interval
    std::chrono::milliseconds getInterval() const {
        return m_interval;
    }

        // Gets the active status of the timer in a thread-safe manner
    bool isActive() const {
        return m_active.load();
    }

private:
    std::thread m_timerThread;               // Thread to run the timer
    std::atomic<bool> m_active;              // Thread-safe flag to indicate whether the timer is active
    std::chrono::milliseconds m_interval;    // Interval for the timer
    std::function<void()> m_slotFunction;    // Slot function to call when the timer fires
};

#endif // SimpleTimer_h
