#include "thread.h"

#include "utils.h"
#include "log.h"

namespace qff {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

Semaphore::Semaphore(size_t count) {
    int rt = ::sem_init(&m_semaphore, 0, count);
    if(rt) 
        throw std::logic_error("sem_init error");
}

Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    int rt = ::sem_wait(&m_semaphore);
    if(rt) 
        throw std::logic_error("sem_wait error");
}

void Semaphore::notify() {
    int rt = ::sem_post(&m_semaphore);
    if(rt)
        throw std::logic_error("sem_post error");
}

RWMutex::RWMutex() noexcept {
    ::pthread_rwlock_init(&m_lock, nullptr);
}

RWMutex::~RWMutex() noexcept {
    ::pthread_rwlock_destroy(&m_lock);
}

void RWMutex::rdlock() noexcept {
    ::pthread_rwlock_rdlock(&m_lock);
}

void RWMutex::wrlock() noexcept {
    ::pthread_rwlock_wrlock(&m_lock);
}

void RWMutex::unlock() noexcept {
    ::pthread_rwlock_unlock(&m_lock);
}

Mutex::Mutex() noexcept {
    ::pthread_mutex_init(&m_mutex, nullptr);
}

Mutex::~Mutex() noexcept {
    ::pthread_mutex_destroy(&m_mutex);
}

void Mutex::lock() noexcept {
    ::pthread_mutex_lock(&m_mutex);
}

void Mutex::unlock() noexcept {
    ::pthread_mutex_unlock(&m_mutex);
}

SpinLock::SpinLock() noexcept {
    ::pthread_spin_init(&m_mutex, 0);
}

SpinLock::~SpinLock() noexcept {
    ::pthread_spin_destroy(&m_mutex);
}

void SpinLock::lock() noexcept {
    ::pthread_spin_lock(&m_mutex);
}

void SpinLock::unlock() noexcept {
    ::pthread_spin_unlock(&m_mutex);
}

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if (t_thread)
        t_thread->m_name = name;
    t_thread_name = name;
}

Thread::Thread(CallBackType callback, const std::string& name) 
    :m_cb(callback) {
    if (name.empty()) 
        m_name = "UNKNOW";
    else
        m_name = name;
    
    int rt = pthread_create(&m_thread, nullptr, &Thread::Run, this);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "pthread_create fail, rt=" << rt
            << "  name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_sem.wait();
}

void Thread::join() {
    if(!m_thread) return;
    int rt = pthread_join(m_thread, nullptr);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "pthread_join fail, rt=" << rt
            << "  name=" << m_name;
        throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
}

void* Thread::Run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;

    thread->m_id = GetThreadId();
    std::string name = thread->m_name.substr(0, 15);
    pthread_setname_np(thread->m_thread, name.c_str());
    SetName(name);

    CallBackType cb;
    cb.swap(thread->m_cb);

    thread->m_sem.notify();

    cb();
    return 0;
}

} //namespace qff