#pragma once
#include "basics.hpp"
#include "exception.hpp"

namespace foray {
    /// @brief A smart, nullable single-object storage in heap bound to the local scope (stack or object field)
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

        T*       GetNullable() { return mData; }
        const T* GetNullable() const { return mData; }

        T& GetRef()
        {
            Assert(!!mData);
            return *mData;
        }
        const T& GetRef() const
        {
            return Assert(!!mData);
            *mData;
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
    class LocalBase
    {
      public:
        LocalBase() : mData({}), mExists(false) {}

        template <typename... TArgs>
        LocalBase(TArgs&&... args) : mExists(true)
        {
            new(reinterpret_cast<T*>(&mData)) T(args...);
        }

        /// @brief Initializes to null
        /// @param other
        LocalBase(std::nullptr_t other) : mData({}), mExists(false) {}

        LocalBase(const LocalBase<T>& other) = delete;
        LocalBase(LocalBase<T>&& other)
        {
            mExists       = true;
            mData         = other.mData;
            other.mData   = {};
            other.mExists = false;
        }
        LocalBase<T>& operator=(const LocalBase<T>& other) = delete;

        /// @brief Assign null (deletes stored value)
        LocalBase<T>& operator=(std::nullptr_t other)
        {
            Delete();
            return *this;
        }

        virtual ~LocalBase() { Delete(); }

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

        T*       GetNullable() { return mExists ? reinterpret_cast<T*>(&mData) : nullptr; }
        const T* GetNullable() const { return mExists ? reinterpret_cast<const T*>(&mData) : nullptr; }

        T& GetRef()
        {
            Assert(mExists);
            return *reinterpret_cast<T*>(&mData);
        }
        const T& GetRef() const
        {
            Assert(mExists);
            return *reinterpret_cast<const T*>(&mData);
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

      protected:
        struct alignas(alignof(T)) AlignedArea
        {
            uint8_t Data[sizeof(T)];
        } mData;
        bool mExists;
    };

    /// @brief A smart, nullable single-object storage in the local scope (stack or object field)
    template <typename T>
    class Local : public LocalBase<T>
    {
      public:
        Local() : LocalBase<T>() {}

        template <typename... TArgs>
        Local(TArgs&&... args) : LocalBase<T>(args...)
        {
        }

        Local(std::nullptr_t other) : LocalBase<T>(other) {}

        Local(LocalBase<T>&& other) : LocalBase<T>(other) {}

        Local<T>& operator=(std::nullptr_t other)
        {
            LocalBase<T>::operator=(other);
            return *this;
        }
    };

    /// @brief A nullable, non-owning ptr to a single object
    template <typename T>
    class View
    {
      public:
        View() : mData(nullptr){};
        View(std::nullptr_t) : mData(nullptr) {}
        View(T* data) : mData(data) {}

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

        T*       GetNullable() { return mData; }
        const T* GetNullable() const { return mData; }

        T& GetRef()
        {
            Assert(!!mData);
            return *mData;
        }
        const T& GetRef() const
        {
            Assert(!!mData);
            return *mData;
        }

        operator bool() const { return !!mData; }

        bool Exists() const { return !!mData; }

      protected:
        T* mData;
    };

    // A guaranteed-valid, non-owning pointer to a single object
    template <typename T>
    class Ref
    {
      public:
        Ref() = delete;
        Ref(T* data) : mData(data) {Assert(!!mData, "Ref initialized with invalid data!");}
        Ref(T& data) : mData(data) {}
        Ref(Local<T>& data) : mData(data.GetRef()) {}
        Ref(Heap<T>& data) : mData(data.GetRef()) {}
        Ref(View<T>& data) : mData(data.GetRef()) {}

        /// @brief Dereference stored pointer
        T* operator->()
        {
            return mData;
        }

        /// @brief Dereference stored pointer
        const T* operator->() const
        {
            return mData;
        }

        T* Get()
        {
            return mData;
        }
        const T* Get() const
        {
            return mData;
        }

        T*       GetNullable() { return mData; }
        const T* GetNullable() const { return mData; }

        T& GetRef()
        {
            return *mData;
        }
        const T& GetRef() const
        {
            return *mData;
        }

        operator bool() const { return true; }

        bool Exists() const { return true; }

      protected:
        T* mData;
    };

#define FORAY_LOCAL_SPECIALIZATION_DEFAULTS(Type)                                                                                                                                  \
    Local() : LocalBase<Type>() {}                                                                                                                                                 \
    Local(std::nullptr_t other) : LocalBase<Type>(other) {}                                                                                                                        \
    Local(Local<Type>&& other)                                                                                                                                                     \
    {                                                                                                                                                                              \
        mExists       = true;                                                                                                                                                      \
        mData         = other.mData;                                                                                                                                               \
        other.mData   = {};                                                                                                                                                        \
        other.mExists = false;                                                                                                                                                     \
    }                                                                                                                                                                              \
    Local<Type>& operator=(std::nullptr_t other)                                                                                                                                   \
    {                                                                                                                                                                              \
        LocalBase<Type>::operator=(other);                                                                                                                                         \
        return *this;                                                                                                                                                              \
    }

/// @brief Return mutable stored pointer of Heap/Local wrapped member
#define FORAY_HAS_MEM(member)                                                                                                                                                      \
    inline bool Has##member() const                                                                                                                                                \
    {                                                                                                                                                                              \
        return m##member.Exists();                                                                                                                                                 \
    }

/// @brief Return mutable stored pointer of Heap/Local wrapped member
#define FORAY_GETTER_MMEM(member)                                                                                                                                                  \
    inline auto* Get##member()                                                                                                                                                     \
    {                                                                                                                                                                              \
        return m##member.Get();                                                                                                                                                    \
    }

/// @brief Return const stored pointer of Heap/Local wrapped member
#define FORAY_GETTER_CMEM(member)                                                                                                                                                  \
    inline const auto* Get##member() const                                                                                                                                         \
    {                                                                                                                                                                              \
        return m##member.Get();                                                                                                                                                    \
    }

#define FORAY_GETTER_MEM(member)                                                                                                                                                   \
    FORAY_HAS_MEM(member)                                                                                                                                                          \
    FORAY_GETTER_MMEM(member)                                                                                                                                                      \
    FORAY_GETTER_CMEM(member)

#define FORAY_STACKALLOC(type, count) reinterpret_cast<type*>(__builtin_alloca_with_align(sizeof(type) * count, alignof(type)))

}  // namespace foray