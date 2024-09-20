#ifndef SINGLETON_H
#define SINGLETON_H

#include <global.h>
#include <memory.h>
#include <iostream>
#include <mutex>

// 单例模板类
template <typename T>
class Singleton{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton<T>& operator=(const Singleton<T>&) = delete;

    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> getInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);
        });

        return _instance;
    }

    void printAddr(){
        std::cout << _instance.get() << std::endl;
    }
    ~Singleton(){
        std::cout << "this is singleton destruct" << std::endl;
    }

};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H
