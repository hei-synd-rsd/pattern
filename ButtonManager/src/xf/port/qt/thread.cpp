#include <config/xf-config.h>

#if (USE_XF_PORT_QT_THREAD_CLASS != 0)

#include <cassert>
#include <cstring>
#include <QCoreApplication>
#include <QSemaphore>
#include "thread.h"


#if !defined(XFTHREAD_DEFAULT_STACK_SIZE)
    #ifdef Q_OS_APPLE
        #define XFTHREAD_DEFAULT_STACK_SIZE	0   /* 0 means take OS specific default stack size */
    #else
        #define XFTHREAD_DEFAULT_STACK_SIZE	256
    #endif
#endif

/**
 * @brief Implementation of interface::XFThread::create method.
 */
interface::XFThread * interface::XFThread::create(interface::XFThreadEntryPointProvider * pProvider,
                                                  interface::XFThread::EntryMethodBody entryMethod,
                                                  const char * threadName,
                                                  const uint32_t stackSize /* = 0 */)
{
    return new ::XFThread(pProvider, entryMethod, threadName, stackSize);
}

XFThread::XFThread(interface::XFThreadEntryPointProvider * pProvider,
                                 interface::XFThread::EntryMethodBody entryMethod,
                                 const char * threadName,
                                 const uint32_t stackSize /* = 0 */)
{
    uint32_t threadStackSize = stackSize;

    pEntryMethodProvider_ = pProvider;
    entryMethod_ = entryMethod;

    const QString name(threadName);
    if (!name.isEmpty())
    {
        setObjectName(name);
    }

    if (threadStackSize == 0)
    {
        threadStackSize = XFTHREAD_DEFAULT_STACK_SIZE;
    }

    setStackSize(threadStackSize);
}

void XFThread::run()
{
    // Stop thread when application gets closed
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(quit()));

    // Call the entry point method with received provider
    (this->pEntryMethodProvider_->*this->entryMethod_)(this);
}

void XFThread::start()
{
    QThread::start();
}

void XFThread::stop()
{
    if (QThread::currentThread() == this)
    {
        // Self-stop must not return, mirroring the Zephyr port's
        // k_thread_suspend(self) semantics: the caller is being destroyed and
        // returning would resume execution inside freed objects.
        // Parking instead of unwinding: pthread_exit()/terminate() abort on
        // Linux/glibc (forced unwind across noexcept destructors). The parked
        // thread is a bounded leak, exactly as on the Zephyr port.
        QSemaphore parked;
        parked.acquire();           // never released: does not return
    }

    // External stop: the dispatcher has already cleared its executing flag and
    // woken the queue, so the thread exits cooperatively. terminate() is only a
    // safety net, as it would cut a run-to-completion step short.
    if (!QThread::wait(2000))
    {
        QThread::terminate();
        QThread::wait();
    }
}


void XFThread::setPriority(XFThreadPriority priority)
{
    auto prio = QThread::NormalPriority;
    switch (priority)
    {
    case XF_THREAD_PRIO_LOW:
        prio = QThread::LowPriority;
        break;
    case XF_THREAD_PRIO_HIGH:
        prio = QThread::HighPriority;
        break;
    default:
        break;
    }

    QThread::setPriority(prio);
}

XFThreadPriority XFThread::getPriority() const
{
    const auto prio = QThread::priority();
    XFThreadPriority priority = XF_THREAD_PRIO_UNKNOWN;

    switch (prio)
    {
    case QThread::LowPriority:
        priority = XF_THREAD_PRIO_LOW;
        break;
    case QThread::NormalPriority:
        priority = XF_THREAD_PRIO_NORMAL;
        break;
    case QThread::HighPriority:
        priority = XF_THREAD_PRIO_HIGH;
        break;
    default:
        break;
    }

    return priority;
}

void XFThread::delay(uint32_t milliseconds)
{
    QThread::msleep(milliseconds);
}

#endif // USE_XF_PORT_QT_THREAD_CLASS
