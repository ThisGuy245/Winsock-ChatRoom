#pragma once
#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <FL/Fl.H>

class Timer {
public:
    using Callback = std::function<void(void*)>;

    Timer(double duration = 1.0)
        : m_duration(duration), m_callback(nullptr), m_userdata(nullptr), m_active(false) {}

    // Sets the callback to be called when the timer triggers
    void setCallback(Callback cb) { m_callback = cb; }

    // Sets the user data to pass to the callback
    void setUserData(void* userdata) { m_userdata = userdata; }

    // Starts the timer
    void start() {
        if (!m_active) {
            m_active = true;
            Fl::add_timeout(m_duration, Tick, this);
        }
    }

    // Stops the timer
    void stop() {
        if (m_active) {
            m_active = false;
            Fl::remove_timeout(Tick, this);
        }
    }

    // Restarts the timer
    void restart() {
        stop();
        start();
    }

    // Destructor: Ensures timer cleanup
    ~Timer() {
        stop();
    }

protected:
    static void Tick(void* userdata) {
        auto* timer = static_cast<Timer*>(userdata);
        if (timer && timer->m_callback && timer->m_active) {
            timer->m_callback(timer->m_userdata); // Call the provided callback
            Fl::repeat_timeout(timer->m_duration, Tick, userdata); // Schedule next timeout
        }
    }

private:
    double m_duration;      // Timer interval duration
    void* m_userdata;       // User data to pass to the callback
    Callback m_callback;    // The function to execute when the timer fires
    bool m_active;          // Indicates if the timer is running
};

#endif // TIMER_H