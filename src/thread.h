#ifndef __QFF_THREAD_H__
#define __QFF_THREAD_H__

#include <memory>
#include <pthread.h>
#include <functional>
#include "macro.h"

namespace qff {

class Thread {
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
};

} //namespace qff

#endif