#ifndef XF_PORT_ZEPHYR_EVENT_QUEUE_H
#define XF_PORT_ZEPHYR_EVENT_QUEUE_H

#include "config/xf-config.h"

#if (USE_XF_PORT_ZEPHYR_EVENT_QUEUE_CLASS != 0)

#include <cstdint>
#include <zephyr/kernel.h>
#include "xf/interface/eventqueue.h"

#ifndef QUEUE_MAX_ELEMENTS_DEFAULT          /* May be overridden at project level. */
    #define QUEUE_MAX_ELEMENTS_DEFAULT  32
#endif 

/** @ingroup port_idf_zephyr
 *  @{
 */

/**
 * @brief Default Zephyr implementation for the XFEventQueue interface.
 *
 * This event queue allows the event processing thread to wait/sleep until a new 
 * message gets pushed by calling the method pend(). 
 * Every time an event gets pushed onto the queue (push method) the waiting thread
 * gets freed and can process the queued message(s). 
 */
class XFEventQueue : public interface::XFEventQueue
{
public:
    XFEventQueue();
    ~XFEventQueue() override;

    // XFEventQueue interface implementation
public:
    bool empty() const override;                    ///< Returns true if no event is in the queue.
    bool push(const XFEvent * pEvent) override;     ///< Pushes the given event onto the queue. Returns false if the event could not be pushed.
    const XFEvent * front() override;               ///< Returns pointer to next event to pop.
    void pop() override;                            ///< Pops the next event from the queue.
    bool pend() override;                           ///< Wait for the next event to arrive. Returns true if an event is in the queue.

protected:
    typedef struct k_msgq EventQueue;               ///< Type of the event queue.

    static const constexpr uint32_t QUEUE_MAX_ELEMENTS = QUEUE_MAX_ELEMENTS_DEFAULT;
    EventQueue queue_;                              ///< Internal queue holding the events.
    char __aligned(4) queue_buffer[QUEUE_MAX_ELEMENTS * sizeof(XFEvent *)];     ///< Internal Buffer structure use by the event queue.
    struct k_sem waitForEventSem;                   ///< Semaphore allowing the event processing thread to wait/sleep until a message gets pushed. ISR-safe.
};

/** @} */ // end of port_idf_zephyr group
#endif // USE_XF_PORT_ZEPHYR_EVENT_QUEUE_CLASS
#endif // XF_PORT_ZEPHYR_EVENT_QUEUE_H
