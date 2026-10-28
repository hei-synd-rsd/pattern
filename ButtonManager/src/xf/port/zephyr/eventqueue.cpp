#include <config/xf-config.h>

#if (USE_XF_PORT_ZEPHYR_EVENT_QUEUE_CLASS != 0)

#include <cassert>
#include "xf/event.h"
#include "eventqueue.h"

XFEventQueue::XFEventQueue()
{
    k_msgq_init(&queue_, queue_buffer, sizeof(XFEvent*), QUEUE_MAX_ELEMENTS);

    k_sem_init(&waitForEventSem, 0, K_SEM_MAX_LIMIT);
    //
    // Note: The semaphore could also be initalised as binary semaphore, but
    //       here the semaphore must only be taken by the pend() method, when
    //       the queue is empty. Otherwise the pend() method would block, even
    //       though there are still messages in the queue.
}

XFEventQueue::~XFEventQueue()
{
	k_msgq_cleanup(&queue_);
}

bool XFEventQueue::empty() const
{
    const uint32_t count = k_msgq_num_used_get(const_cast<EventQueue*>(&queue_)); 	
    return (count == 0) ? true : false;
}

bool XFEventQueue::push(const XFEvent * pEvent)
{
    int status = ENOMSG;

    if (pEvent->getDelay() == 0)
    {
        // Push event onto the queue
        status = k_msgq_put(&queue_, &pEvent, K_NO_WAIT);

        // Signal the queue processing thread a message got pushed (ISR-safe)
        if (status == 0)
        {
            k_sem_give(&waitForEventSem);
        }
    }

    return (status == 0) ? true : false;
}

const XFEvent * XFEventQueue::front()
{
    XFEvent * event = nullptr;

    k_msgq_peek(&queue_, &event);
    return event;
}

void XFEventQueue::pop()
{
    XFEvent * event = nullptr;
    k_msgq_get(&queue_, &event, K_NO_WAIT);
}

bool XFEventQueue::pend()
{
    // Wait until an event is put into the queue
    k_sem_take(&waitForEventSem, K_FOREVER);
    return !empty();
}

#endif // USE_XF_PORT_ZEPHYR_EVENT_QUEUE_CLASS
