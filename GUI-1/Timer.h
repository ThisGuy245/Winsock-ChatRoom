#pragma once
#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <FL/Fl.H>

/**
 * @brief Timer class to manage periodic callbacks.
 */
class Timer {
public:
    using Callback = std::function<void(void*)>;

    /**
     * @brief Constructs a Timer with a specified duration.
     * @param duration The interval duration in seconds (default is 1.0).
     */
    Timer(double duration = 1.0)
        : m_duration(duration), m_callback(nullptr), m_userdata(nullptr), m_active(false) {}


    /** Sets the callback to be called when the timer triggers */
    void setCallback(Callback cb) { m_callback = cb; }

    /** Sets the user data to pass to the callback */
    void setUserData(void* userdata) { m_userdata = userdata; }

    /** Starts the timer */
    void start() {
        if (!m_active) {
            m_active = true;
            Fl::add_timeout(m_duration, Tick, this);
        }
    }

    /** Stops the timer */
    void stop() {
        if (m_active) {
            m_active = false;
            Fl::remove_timeout(Tick, this);
        }
    }

    /**
     * @brief Restarts the timer.
     */
    void restart() {
        stop();
        start();
    }

    /**
     * @brief Destructor ensures timer cleanup by stopping it if active.
     */
    ~Timer() {
        stop();
    }

protected:
    /**
     * @brief Static function called by FLTK to trigger the timer.
     * @param userdata Pointer to the Timer instance.
     */
    static void Tick(void* userdata) {
        auto* timer = static_cast<Timer*>(userdata);
        if (timer && timer->m_callback && timer->m_active) {
            timer->m_callback(timer->m_userdata); // Execute the callback
            Fl::repeat_timeout(timer->m_duration, Tick, userdata); // Schedule the next timeout
        }
    }

private:
    double m_duration;      /**< The interval duration in seconds. */
    void* m_userdata;       /**< Pointer to user data passed to the callback. */
    Callback m_callback;    /**< The callback function executed when the timer fires. */
    bool m_active;          /**< Indicates if the timer is currently running. */
};

#endif // TIMER_H
