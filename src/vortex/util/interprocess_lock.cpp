#include "interprocess_lock.h"

vortex::interprocess_lock::interprocess_lock(std::string_view name)
    : sync_name(name)
{
#if defined(_WIN32)
    std::string mutex_name = std::format("Global\\vortex_{}", name);
    mutex_handle = CreateMutexA(nullptr, FALSE, mutex_name.c_str());
    if (mutex_handle == nullptr) {
        throw std::runtime_error("Failed to create Windows mutex: " + std::to_string(GetLastError()));
    }

#else // Linux
    std::string sem_name = std::format("/vortex_{}", name);
    semaphore = sem_open(sem_name.c_str(), O_CREAT, 0644, 1);
    if (semaphore == SEM_FAILED) {
        throw std::runtime_error("Failed to create Linux semaphore: " + std::string(strerror(errno)));
    }
#endif
}

vortex::interprocess_lock::~interprocess_lock() noexcept
{
#if defined(_WIN32)
    if (mutex_handle != nullptr) {
        CloseHandle(mutex_handle);
    }

#else // Linux
    if (semaphore != SEM_FAILED) {
        sem_close(semaphore);
        std::string sem_name = std::format("/vortex_{}", sync_name);
        sem_unlink(sem_name.c_str()); // Clean up
    }
#endif
}

bool vortex::interprocess_lock::try_lock(std::chrono::milliseconds timeout) noexcept
{
#if defined(_WIN32)
    DWORD result = WaitForSingleObject(mutex_handle, static_cast<DWORD>(timeout.count()));
    return result == WAIT_OBJECT_0;

#else // Linux
    struct timespec ts;
    auto now = std::chrono::system_clock::now();
    auto timeout_point = now + timeout;
    auto timeout_time_t = std::chrono::system_clock::to_time_t(timeout_point);
    auto timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              timeout_point.time_since_epoch())
                              .count() %
            1000000000;

    ts.tv_sec = timeout_time_t;
    ts.tv_nsec = timeout_ns;

    return sem_timedwait(semaphore, &ts) == 0;
#endif
}
bool vortex::interprocess_lock::lock() noexcept
{
#if defined(_WIN32)
    DWORD result = WaitForSingleObject(mutex_handle, INFINITE);
    return result == WAIT_OBJECT_0;
#else // Linux
    return sem_wait(semaphore) == 0;
#endif
}

void vortex::interprocess_lock::unlock() noexcept
{
#if defined(_WIN32)
    ReleaseMutex(mutex_handle);
#else // Linux
    sem_post(semaphore);
#endif
}