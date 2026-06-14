/**
 * @brief 获取当前线程id
 * @date 2026.06.13
 */
#include "CurrentThread.h"
#include <unistd.h>
#include <sys/syscall.h>

namespace smw           {
namespace thread        {
namespace CurrentThread {

__thread int t_cachedTid = 0;
void cacheTid() {
    t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
}

} // namespace CurrentThread
} // namespace thread
} // namespace smw
