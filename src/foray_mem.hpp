#pragma once
#include "foray_basics.hpp"
#include "foray_exception.hpp"

namespace foray {
    /// @brief Movable std::unique_ptr alternative
    /// @tparam T Stored Type
    template <typename T>
    class Heap
    {
      public:
        Heap() : mData(nullptr) {}

        /// @brief Allocates heap space, and initializes a new T instance
        /// @tparam ...TArgs
        /// @param ...args
        template <typename... TArgs>
        Heap(TArgs&&... args) : mData(new T(args...))
        {
        }

        /// @brief Initializes a heap reference by adopting the value of other
        /// @param other
        Heap(Heap<T>& other) : mData(other.mData) { other.mData = nullptr; }

        /// @brief Initializes to null
        /// @param other
        Heap(std::nullptr_t other) : mData(nullptr) {}

        Heap(const Heap<T>& other) = delete;
        Heap(Heap<T>&& other)
        {
            mData       = other.mData;
            other.mData = nullptr;  // other goes out of scope
        }

        /// @brief Adopt another heap references value
        Heap<T>& operator=(Heap<T>& other)
        {
            Delete();
            mData       = other.mData;
            other.mData = nullptr;
            return *this;
        }

        /// @brief Adopt another heap references value
        Heap<T>& operator=(Heap<T>&& other)
        {
            Delete();
            mData       = other.mData;
            other.mData = nullptr;
            return *this;
        }

        Heap<T>& operator=(const Heap<T>& other) = delete;

        /// @brief Assign null (deletes stored value)
        Heap<T>& operator=(std::nullptr_t other)
        {
            Delete();
            return *this;
        }

        virtual ~Heap() { Delete(); }

        /// @brief Dereference stored pointer
        T* operator->()
        {
            Assert(!!mData, "Attempted to deref a nullptr!");
            return mData;
        }

        /// @brief Dereference stored pointer
        const T* operator->() const
        {
            Assert(!!mData, "Attempted to deref a nullptr!");
            return mData;
        }

        T* Get()
        {
            Assert(!!mData);
            return mData;
        }
        const T* Get() const
        {
            Assert(!!mData);
            return mData;
        }

        T* GetNullable()
        {
            return mData;
        }
        const T* GetNullable() const
        {
            return mData;
        }

        operator bool() const { return !!mData; }

        bool Exists() const { return !!mData; }

        /// @brief Replace stored value with a newly allocated and constructed value
        template <typename... TArgs>
        void New(TArgs&&... args)
        {
            Delete();
            mData = new T(args...);
        }

        /// @brief Replace stored value with a newly constructed value, reusing an existing allocation if it exists
        template <typename... TArgs>
        void InPlaceNew(TArgs&&... args)
        {
            if(!!mData)
            {
                mData->~T();
                new(mData) T(args...);
            }
            else
            {
                mData = new T(args...);
            }
        }

        /// @brief Delete the stored value
        virtual void Delete()
        {
            if(mData)
            {
                delete mData;
                mData = nullptr;
            }
        }

        /// @brief Swap stored values with another heap reference instance
        void Swap(Heap<T>& other)
        {
            T* temp     = mData;
            mData       = other.mData;
            other.mData = temp;
        }

      private:
        T* mData = nullptr;
    };

    template <typename T>
    class Local
    {
      public:
        Local() : mData({}), mExists(false) {}

        /// @brief Allocates heap space, and initializes a new T instance
        /// @tparam ...TArgs
        /// @param ...args
        template <typename... TArgs>
        Local(TArgs&&... args) : mExists(true)
        {
            new(reinterpret_cast<T*>(&mData)) T(args...);
        }

        /// @brief Initializes to null
        /// @param other
        Local(std::nullptr_t other) : mData({}), mExists(false) {}

        Local(const Local<T>& other) = delete;
        Local(Local<T>&& other)
        {
            mExists = true;
            memcpy(mData, other.mData, sizeof(T));
            other.mExists = false;
        }
        Local<T>& operator=(const Local<T>& other) = delete;

        /// @brief Assign null (deletes stored value)
        Local<T>& operator=(std::nullptr_t other)
        {
            Delete();
            return *this;
        }

        virtual ~Local() { Delete(); }

        operator bool() { return mExists; }
        operator bool() const { return mExists; }

        bool Exists() const { return mExists; }

        /// @brief Dereference stored pointer
        T* operator->()
        {
            Assert(mExists, "Attempted to deref a nullptr!");
            return reinterpret_cast<T*>(&mData);
        }

        /// @brief Dereference stored pointer
        const T* operator->() const
        {
            Assert(mExists, "Attempted to deref a nullptr!");
            return reinterpret_cast<const T*>(&mData);
        }

        T* Get()
        {
            Assert(mExists);
            return reinterpret_cast<T*>(&mData);
        }
        const T* Get() const
        {
            Assert(mExists);
            return reinterpret_cast<const T*>(&mData);
        }

        T* GetNullable()
        {
            return mExists ? reinterpret_cast<T*>(&mData) : nullptr;
        }
        const T* GetNullable() const
        {
            return mExists ? reinterpret_cast<const T*>(&mData) : nullptr;
        }

        /// @brief Replace stored value with a newly allocated and constructed value
        template <typename... TArgs>
        void New(TArgs&&... args)
        {
            Delete();
            new(reinterpret_cast<T*>(&mData)) T(args...);
            mExists = true;
        }

        /// @brief Delete the stored value
        virtual void Delete()
        {
            if(mExists)
            {
                reinterpret_cast<T&>(mData).~T();
                mExists = false;
            }
        }

      private:
        struct alignas(alignof(T)) AlignedArea
        {
            uint8_t Data[sizeof(T)];
        } mData;
        bool mExists;
    };
}  // namespace foray