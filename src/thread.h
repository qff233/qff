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

    ScopeLockImpl(T& mutex, bool is_lock = true) noexcept
        :m_mutex(mutex) {
        if(!is_lock) 
            return;
        m_mutex.lock();
    }

    ~ScopeLockImpl() noexcept {
        m_mutex.unlock();
    }

    void lock() noexcept {
        m_mutex.lock();
    }

    void unlock() noexcept {
        m_mutex.unlock();
    }
private:
    T& m_mutex;
};

template<class T>
class ScopeReadLockImpl final {
public:
    NONECOPYABLE(ScopeReadLockImpl);

    ScopeReadLockImpl(T& mutex, bool is_lock = true) noexcept
        :m_mutex(mutex) {
        if(!is_lock) 
            return;
        m_mutex.rdlock();
    }

    ~ScopeReadLockImpl() noexcept {
        m_mutex.unlock();
    }

    void lock() noexcept {
        m_mutex.rdlock();
    }

    void unlock() noexcept {
        m_mutex.unlock();
    }
private:
    T& m_mutex;
};

template<class T>
class ScopeWriteLockImpl final {
public:
    NONECOPYABLE(ScopeWriteLockImpl);

    ScopeWriteLockImpl(T& mutex, bool is_lock = true) noexcept
        :m_mutex(mutex) {
        if(!is_lock) 
            return;
        m_mutex.wrlock();
    }

    ~ScopeWriteLockImpl() noexcept {
        m_mutex.unlock();
    }

    void lock() noexcept {
        m_mutex.wrlock();
    }

    void unlock() noexcept {
        m_mutex.unlock();
    }
private:
    T& m_mutex;
};

class Conditon final {
public:
    NONECOPYABLE(Conditon);

    Conditon() noexcept;
    ~Conditon() noexcept;

    void lock() noexcept;
    void unlock() noexcept;

    void wait() noexcept;
    void notify() noexcept;
private:
    bool m_is_locked = false;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_conditon;
};

class RWMutex final {
public:
    typedef ScopeReadLockImpl<RWMutex> ReadLock;
    typedef ScopeWriteLockImpl<RWMutex> WriteLock;

    RWMutex() noexcept;
    ~RWMutex() noexcept;

    void rdlock() noexcept;
    void wrlock() noexcept;
    void unlock() noexcept;
private:
    bool m_is_locked = false;
    pthread_rwlock_t m_lock;
};

class Mutex final {
public:
    NONECOPYABLE(Mutex);

    typedef ScopeLockImpl<Mutex> Lock;

    Mutex() noexcept;
    ~Mutex() noexcept;

    void lock() noexcept;
    void unlock() noexcept;
private:
    bool m_is_locked = false;
    pthread_mutex_t m_mutex;
};

class SpinLock final {
public:
    NONECOPYABLE(SpinLock);

    typedef ScopeLockImpl<SpinLock> Lock;

    SpinLock() noexcept;
    ~SpinLock() noexcept;

    void lock() noexcept;
    void unlock() noexcept;
private:
    bool m_is_locked = false;
    pthread_spinlock_t m_mutex;
};

class Thread final {
public:
    NONECOPYABLE(Thread);
    typedef std::shared_ptr<Thread> ptr;
    typedef std::function<void()> CallBackType;

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