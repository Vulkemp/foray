#pragma once

namespace hsk {

    /// @brief Any resource
    template <class T, size_t Count>
    class FrameRotator
    {
      public:
        FrameRotator() : mObjects{} {}

        template <typename... TArgs>
        FrameRotator(TArgs... args)
        {
            for(size_t i = 0; i < Count; i++)
            {
                new (&(mObjects[i])) T(std::forward<TArgs>(args)...);
            }
        }

        template <typename... TArgs>
        void Init(TArgs... args)
        {
            for(size_t i = 0; i < Count; i++)
            {
                mObjects[i].Init(std::forward<TArgs>(args)...);
            }
        }

        void Destroy()
        {
            for(size_t i = 0; i < Count; i++)
            {
                mObjects[i].Destroy();
            }
        }

        T&       Get(size_t index) { return mObjects[index % Count]; }
        const T& Get(size_t index) const { return mObjects[index % Count]; }

        T&       operator[](size_t index) { return mObjects[index % Count]; }
        const T& operator[](size_t index) const { return mObjects[index % Count]; }

        ~FrameRotator() { Destroy(); }

      protected:
        T mObjects[Count];
    };

}  // namespace hsk