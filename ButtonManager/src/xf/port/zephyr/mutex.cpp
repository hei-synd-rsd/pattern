#include <config/xf-config.h>

#if (USE_XF_PORT_ZEPHYR_MUTEX_CLASS != 0)

#include <cassert>
#include "critical/critical.h"
#include "mutex.h"

/**
 * @brief Implementation of interface::XFMutex::create method.
 */
interface::XFMutex * interface::XFMutex::create()
{
    return new ::XFMutex;
}

XFMutex::XFMutex()
{
    int success = k_mutex_init(&mutex_);
    assert(success == 0);
}

void XFMutex::lock()
{
    if (!inISR())
    {
        k_mutex_lock(&mutex_, K_FOREVER);
    }
}

void XFMutex::unlock()
{
    if (!inISR())
    {
        k_mutex_unlock(&mutex_);
    }
}

bool XFMutex::tryLock(int32_t timeout /* = 0 */)
{
    const k_timeout_t WAIT_TIME = (timeout == 0) ? K_NO_WAIT : ((timeout < 0) ? K_FOREVER : K_MSEC(timeout));
    int success = -1;

    if (!inISR())
    {
        success = k_mutex_lock(&mutex_, WAIT_TIME);
    }
    return (success == 0);
}

#endif // USE_XF_PORT_ZEPHYR_MUTEX_CLASS
