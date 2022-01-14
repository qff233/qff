#ifndef __QFF_SINGLETON_H__
#define __QFF_SINGLETON_H__


namespace qff {

template<typename T>
class Singleton final {
public:
    static T* Get() { return *GetPPtr(); }

    template<typename... Args>
    static void New(Args&&... args) {
        if(Get() != nullptr) std::abort();
        *GetPPtr() = new T(std::forward<Args>(args)...);
    }

    static void Delete() {
        if (Get() == nullptr) return;
        delete Get();
        *GetPPtr() == nullptr;
    }
private:
    static T** GetPPtr() {
        static T* ptr = nullptr;
        return &ptr;
    }
};

}

#endif