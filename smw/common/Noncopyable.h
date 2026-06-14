/**
 * @brief 不可复制类
 * @date 2026.06.14
 */

#ifndef SMW_COMMON_NONCOPYABLE_H_
#define SMW_COMMON_NONCOPYABLE_H_

namespace smw     {
namespace common  {

class Noncopyable {
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

} // namespace common 
} // namespace smw

#endif
