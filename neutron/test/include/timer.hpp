#pragma once
#include <chrono>
#include <cstddef>
#include <neutron/print.hpp>
#include <neutron/tstring.hpp>

template <typename Duration = std::chrono::milliseconds>
class scope_timer {
public:
    scope_timer() noexcept
        : start_(std::chrono::high_resolution_clock::now()) {}

    scope_timer(const scope_timer&)            = delete;
    scope_timer& operator=(const scope_timer&) = delete;
    scope_timer(scope_timer&&)                 = delete;
    scope_timer& operator=(scope_timer&&)      = delete;

    ~scope_timer() noexcept {
        using namespace std::chrono;
        const auto now     = high_resolution_clock::now();
        const auto elapsed = duration_cast<Duration>(now - start_);
        neutron::println("elapsed: {}", elapsed);
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

template <typename Duration = std::chrono::milliseconds>
auto set_timer() {
    return scope_timer<Duration>();
}

template <typename Duration = std::chrono::milliseconds>
class scope_msg_timer {
public:
    template <size_t Size>
    scope_msg_timer(const char (&msg)[Size]) noexcept // NOLINT
        : msg_(msg), start_(std::chrono::high_resolution_clock::now()) {}

    scope_msg_timer(const scope_msg_timer&)            = delete;
    scope_msg_timer& operator=(const scope_msg_timer&) = delete;
    scope_msg_timer(scope_msg_timer&&)                 = delete;
    scope_msg_timer& operator=(scope_msg_timer&&)      = delete;

    ~scope_msg_timer() noexcept {
        using namespace std::chrono;
        const auto now     = high_resolution_clock::now();
        const auto elapsed = duration_cast<Duration>(now - start_);
        neutron::println("{} used: {}", msg_, elapsed);
    }

private:
    std::string msg_;
    std::chrono::high_resolution_clock::time_point start_;
};

template <typename Duration = std::chrono::milliseconds, size_t Size>
auto set_timer(const char (&msg)[Size]) { // NOLINT
    return scope_msg_timer<Duration>(msg);
}
