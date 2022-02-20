#ifndef __QFF_SINGLETON_H__
#define __QFF_SINGLETON_H__

#include <memory>

namespace qff {

template<typename T>
class Singleton final {
public:
    static T* Get() { return GetPPtr()->get(); }

    template<typename... Args>
    static void New(Args&&... args) {
        if(Get() != nullptr) std::abort();
        *GetPPtr() = std::make_shared<T>(std::forward<Args>(args)...);
    }

    static void Delete() {
        if (Get() == nullptr) return;
        GetPPtr()->reset();
    }
private:
    static std::shared_ptr<T>* GetPPtr() {
        static std::shared_ptr<T> ptr = nullptr;
        return &ptr;
    }
};

}

#endif