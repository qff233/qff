#ifndef __QFF_THREAD_H__
#define __QFF_THREAD_H__

#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <functional>
#include "macro.h"

namespace qff {

class Semaphore final {
public:
    NONECOPYABLE(Semaphore);

    Semaphore(size_t count = 0);
    ~Semaphore();

    void wait();
    void notify();
private:
    sem_t m_semaphore;
};

template<class T>
class ScopeLockImpl final {
public:
    NONECOPYABLE(ScopeLockImpl);

    ScopeLockImpl(T& mutex) noexcept
        :m_mutex(mutex) {
        m_is_lock = true;
        m_mutex.lock();
    }

    ~ScopeLockImpl() noexcept {
        if(m_is_lock)
            m_mutex.unlock();
    }

    void lock() noexcept {
        if(m_is_lock)
            return;
        m_is_lock = true;
        m_mutex.lock();
    }

    void unlock() noexcept {
        if(!m_is_lock)
            return;
        m_is_lock = false;
        m_mutex.unlock();
    }
private:
    T& m_mutex;
    bool m_is_lock;
};

template<class T>
class ScopeReadLockImpl final {
public:
    NONECOPYABLE(ScopeReadLockImpl);

    ScopeReadLockImpl(T& mutex) noexcept
        :m_mutex(mutex) {
        m_is_lock = true;
        m_mutex.rdlock();
    }

    ~ScopeReadLockImpl() noexcept {
        if(m_is_lock) 
            m_mutex.unlock();
    }

    void lock() noexcept {
        if(m_is_lock)
            return;
        m_is_lock = true;
        m_mutex.rdlock();
    }

    void unlock() noexcept {
        if(!m_is_lock)
            return;
        m_is_lock = false;
        m_mutex.unlock();
    }
private:
    T& m_mutex;
    bool m_is_lock;
};

template<class T>
class ScopeWriteLockImpl final {
public:
    NONECOPYABLE(ScopeWriteLockImpl);

    ScopeWriteLockImpl(T& mutex) noexcept
        :m_mutex(mutex) {
        m_is_lock = true;
        m_mutex.wrlock();
    }

    ~ScopeWriteLockImpl() noexcept {
        if(m_is_lock)
            m_mutex.unlock();
    }

    void lock() noexcept {
        if(m_is_lock)
            return;
        m_is_lock = true;
        m_mutex.wrlock();
    }

    void unlock() noexcept {
        if(!m_is_lock)
            return;
        m_is_lock = false;
        m_mutex.unlock();
    }
private:
    T& m_mutex;
    bool m_is_lock;
};

class RWMutex final {
public:
    using ReadLock  = ScopeReadLockImpl<RWMutex>;
    using WriteLock = ScopeWriteLockImpl<RWMutex>;

    RWMutex() noexcept;
    ~RWMutex() noexcept;

    void rdlock() noexcept;
    void wrlock() noexcept;
    void unlock() noexcept;
private:
    pthread_rwlock_t m_lock;
};

class Mutex final {
public:
    NONECOPYABLE(Mutex);

    using Lock = ScopeLockImpl<Mutex>;

    Mutex() noexcept;
    ~Mutex() noexcept;

    void lock() noexcept;
    void unlock() noexcept;
private:
    pthread_mutex_t m_mutex;
};

class SpinLock final {
public:
    NONECOPYABLE(SpinLock);

    using Lock = ScopeLockImpl<SpinLock>;

    SpinLock() noexcept;
    ~SpinLock() noexcept;

    void lock() noexcept;
    void unlock() noexcept;
private:
    pthread_spinlock_t m_mutex;
};

class Thread final {
public:
    NONECOPYABLE(Thread);
    using ptr = std::shared_ptr<Thread>;
    using CallBackType = std::function<void()>;

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);

    Thread(CallBackType callback, const std::string& name = "");

    void join();

    pid_t get_id() const { return m_id; }
    const std::string& get_name() const { return m_name; }
private:
    static void* Run(void* arg);
private:
    pid_t m_id = -1;
    std::string m_name;
    CallBackType m_cb;
    pthread_t m_thread = 0;
    Semaphore m_sem;
};


} //namespace qff

#endif