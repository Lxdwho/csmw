/**
 * @brief 获取当前线程id
 * @date 2026.06.13
 */
#ifndef _SMW_THREAD_CURRENTTHREAD_H_
#define _SMW_THREAD_CURRENTTHREAD_H_

namespace smw           {
namespace thread        {
namespace CurrentThread {

extern __thread int t_cachedTid;
void cacheTid();

inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0)) {
        cacheTid();
    }
    return t_cachedTid;
}

} // namespace CurrentThread
} // namespace thread
} // namespace smw

#endif // _SMW_THREAD_CURRENTTHREAD_H_
