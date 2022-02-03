#include "thread.h"

#include "utils.h"
#include "log.h"

namespace qff {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

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
    cb();
    return 0;
}

} //namespace qff