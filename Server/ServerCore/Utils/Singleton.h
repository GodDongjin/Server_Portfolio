#pragma once

template<typename T>
class Singleton
{
protected:
    Singleton() = default;
    virtual ~Singleton() = default;

public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    static T* get_instance()
    {
        static T instance;
        return &instance;
    }
};