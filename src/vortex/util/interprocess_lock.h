// universal_sync_output.hpp
#pragma once

#include <iostream>
#include <string>
#include <format>
#include <chrono>
#include <memory>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#endif

namespace vortex {
class interprocess_lock
{
public:
    explicit interprocess_lock(std::string_view name = "stdout_sync");
    ~interprocess_lock();

public:
    bool try_lock(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    bool lock();
    void unlock();

private:
    std::string sync_name;

#if defined(_WIN32)
    HANDLE mutex_handle;
#else // Linux
    sem_t* semaphore;
#endif
};
} // namespace vortex