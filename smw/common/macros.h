/**
 * @brief common宏
 * @date 2025.11.14
 */

#ifndef _SMW_COMMON_MACROS_H_
#define _SMW_COMMON_MACROS_H_

#include <type_traits>
#include <mutex>

#if __GNUC__ >= 3
#define csmw_likely(x) (__builtin_expect((x), 1))
#define csmw_unlikely(x) (__builtin_expect((x), 0))
#else
#define csmw_likely(x) (x)
#define csmw_unlikely(x) (x)
#endif

#define CACHELINE_SIZE 64

#define DEFINE_TYPE_TRAIT(name, func)                           \
    template <typename T>                                       \
    struct name {                                               \
        template <typename Class>                               \
        static constexpr bool Test(decltype(&Class::func)*) {   \
            return true;                                        \
        }                                                       \
        template <typename>                                     \
        static constexpr bool Test(...) {                       \
            return false;                                       \
        }                                                       \
        static constexpr bool value = Test<T>(nullptr);         \
    };                                                          \
    template <typename T>                                       \
    constexpr bool name<T>::value; 

DEFINE_TYPE_TRAIT(HasShutdown, Shutdown)

template <typename T>
typename std::enable_if<HasShutdown<T>::value>::type CallShutdown(T* instance) {
    instance->Shutdown();
}

template <typename T>
typename std::enable_if<!HasShutdown<T>::value>::type CallShutdown(T* instance) {
    (void)instance;
}

#undef UNUSED
#undef DISALLOW_COPY_AND_ASSIGN

#define UNUSED(param) (void)param

#define DISALLOW_COPY_AND_ASSIGN(classname)             \
    classname(const classname & ) = delete;             \
    classname &operator=(const classname & ) = delete;

#define DECLARE_SINGLETON(classname)                                                    \
public:                                                                                 \
    static classname *Instance(bool create_if_need = true) {                            \
        static classname *instance = nullptr;                                           \
        if(!instance && create_if_need) {                                               \
            static std::once_flag flag;                                                 \
            std::call_once(flag, [&]{ instance = new (std::nothrow) classname(); });    \
        }                                                                               \
        return instance;                                                                \
    }                                                                                   \
                                                                                        \
    static void CleanUp() {                                                             \
        auto instance = Instance(false);                                                \
        if(instance != nullptr) {                                                       \
            CallShutdown(instance);                                                     \
        }                                                                               \
    }                                                                                   \
private:                                                                                \
    classname();                                                                        \
    DISALLOW_COPY_AND_ASSIGN(classname)

#endif
