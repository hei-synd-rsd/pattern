#include <config/xf-config.h>

#if (USE_XF_PORT_ZEPHYR_EVENT_CLASS != 0)

#include <cassert>
#include <zephyr/kernel.h>
#include "xf/interface/behavior.h"
#include "xf/event.h"

XFEvent::~XFEvent()
{
    stopTimer();
    purgeTimer();
}

uint32_t XFEvent::startTimer(int32_t timeoutInMilliseconds)
{
    // TODO: Create and start a Zephyr timer:
    //       - Use 'k_malloc' to dynamically allocate the k_timer structure: https://docs.zephyrproject.org/latest/kernel/memory_management/heap.html
    //       - Learn how to create and start a Zephyr timer: https://docs.zephyrproject.org/latest/kernel/services/timing/timers.html
    //       - Use 'timeoutHandle' attribute to keep a reference to the created timer
    //       - Initialize timer and use the callback methods 'XFEvent::onTimerTimeout' and 'XFEvent::onTimerCanceled'
    //       - Use 'k_timer_user_data_set' to keep a reference to the current event (used afterwards in callback methods)
    //       - Return 'timeoutHandle' attribute to provide the caller a handle to the timer

    struct k_timer * timer;

    // Check if timer was already created
    if (!timeoutHandle)
    {
        // Create timer
        timer = (struct k_timer*)k_malloc(sizeof(struct k_timer));
        timeoutHandle = (uint32_t)timer;

        k_timer_init(timer, (k_timer_expiry_t)&XFEvent::onTimerTimeout, (k_timer_stop_t)&XFEvent::onTimerCanceled);
        k_timer_user_data_set(timer, this);     // Store pointer for later use
    }
    else
    {
        timer = (struct k_timer*)timeoutHandle;
    }
    assert(timer);

    k_timer_start(timer, K_MSEC(timeoutInMilliseconds), K_MSEC(0));

    return timeoutHandle;
}

void XFEvent::stopTimer()
{
    if (timeoutHandle)
    {
        struct k_timer* timer = (struct k_timer*) timeoutHandle;

        k_timer_stop(timer);
    }
}

void XFEvent::purgeTimer()
{
    if (timeoutHandle)
    {
        struct k_timer * timer = (struct k_timer*)timeoutHandle;

        k_free(timer);
        timeoutHandle = 0;
    }
}

// static
void XFEvent::onTimerTimeout(void * timerContext)
{
    struct k_timer * t = (struct k_timer *)timerContext;    // Convert to k_timer
    XFEvent * event = (XFEvent*) k_timer_user_data_get(t);

    event->getBehavior()->pushEvent(event, 0);
}

// static
void XFEvent::onTimerCanceled(void * timerContext)
{
    struct k_timer * t = (struct k_timer *)timerContext;    // Convert to k_timer
    XFEvent * event = (XFEvent*) k_timer_user_data_get(t);

    // Delete software timer
    // Note: Not realy necessary here. For testing purposes only
    event->purgeTimer();
}

#endif // USE_XF_PORT_ZEPHYR_EVENT_CLASS
