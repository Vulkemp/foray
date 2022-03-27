#pragma once
#include <memory>
#include "hsk_basics.hpp"

namespace hsk
{
    /// @brief Encapsulates a pointer to signify it is a non-owning reference
    /// @tparam T
    template <typename T>
    class loan_ptr
    {
    public:
        T *Data;

        loan_ptr() : Data() {}
        loan_ptr(T &data) : Data(&data) {}
        loan_ptr(T *data) : Data(data) {}
        loan_ptr(const std::unique_ptr<T> &ptr) : Data(ptr.get()) {}
        loan_ptr(const nullptr_t data) : Data(nullptr) {}
        template <typename TCast>
        loan_ptr(const loan_ptr<TCast> &other) : Data(dynamic_cast<T *>(other.Data))
        {
        }
        template <typename TCast>
        loan_ptr(const std::unique_ptr<TCast> &other) : Data(dynamic_cast<T *>(other.get()))
        {
        }

        loan_ptr(const loan_ptr<T> &other) = default;
        loan_ptr(const loan_ptr<T> &&other) noexcept : Data(other.Data) {}

        loan_ptr<T> &operator=(const loan_ptr<T> &other) = default;

        loan_ptr<T> &operator=(T &data);

        inline loan_ptr<T> &operator=(T *data);
        inline loan_ptr<T> &operator=(const std::unique_ptr<T> &ptr);
        inline loan_ptr<T> &operator=(const nullptr_t data);

        inline T &operator*() { return *Data; }
        inline const T &operator*() const { return *Data; }
        inline T *operator->() { return Data; }
        inline const T *operator->() const { return Data; }

        inline operator bool() const { return Data != nullptr; }

        template <typename TCast>
        inline operator loan_ptr<TCast>()
        {
            return loan_ptr<TCast>(dynamic_cast<TCast *>(Data));
        }

        inline bool operator==(const nullptr_t &other) const { return Data == nullptr; }
        inline bool operator==(const T *other) const { return Data == other; }
        inline bool operator==(const loan_ptr<T> &other) const { return Data == other.Data; }
        inline bool operator==(const std::unique_ptr<T> &other) const { return Data == other.get(); }
    };

#pragma region loan_ptr functions
    template <typename T>
    inline loan_ptr<T> &loan_ptr<T>::operator=(T &data)
    {
        Data = &data;
        return *this;
    }

    template <typename T>
    inline loan_ptr<T> &loan_ptr<T>::operator=(T *data)
    {
        Data = data;
        return *this;
    }

    template <typename T>
    inline loan_ptr<T> &loan_ptr<T>::operator=(const std::unique_ptr<T> &ptr)
    {
        Data = ptr.get();
        return *this;
    }

    template <typename T>
    inline loan_ptr<T> &loan_ptr<T>::operator=(const nullptr_t data)
    {
        Data = nullptr;
        return *this;
    }

    template<typename Tin, typename Tout>
    inline loan_ptr<Tout> loan_ptr_cast(loan_ptr<Tin> ptr){
        return loan_ptr<Tout>(dynamic_cast<Tout*>(*ptr));
    }

#pragma endregion

}