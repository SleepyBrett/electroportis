#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atomic>
//#include <condition_variable>
#include <exception>
#include <memory>

namespace saver { namespace util {
struct failure_t : public std::exception {
    explicit failure_t(const LPCTSTR err)
        : msg(err)
    {}

    const LPCTSTR msg;
};

static int
err_msg(LPCTSTR msg)
{
    return MessageBox(nullptr, msg, TEXT("Error"), MB_ICONERROR | MB_OK);
}

#define EP_DIE(msg) \
{ \
    saver::util::err_msg(TEXT(msg)); \
    throw saver::util::failure_t(TEXT(msg)); \
}

class event_t {
    struct del {
        void operator()(void *ptr) { CloseHandle(ptr); }
    };
public:
    explicit event_t(BOOL manual_reset, BOOL initial_state)
        : ev(CreateEvent(nullptr, manual_reset, initial_state, nullptr))
    {
        if (nullptr == ev.get())
            EP_DIE("CreateEvent failed.");
    }

    BOOL set() const { return SetEvent(ev.get()); }
    BOOL reset() const { return ResetEvent(ev.get()); }
    BOOL wait(const DWORD ms) const {
        return WaitForSingleObject(ev.get(), ms);
    }

    std::unique_ptr<void, del> ev;
};

class thread_t {
    struct del {
        void operator()(void *ptr) { }
    };
public:
    explicit thread_t()
        : msgq_ready(TRUE, FALSE)
        , quitting(TRUE, FALSE)
        , thr(nullptr)
        , tid(0)
    {}

    void init(LPTHREAD_START_ROUTINE proc, LPVOID arg) {
        thr.reset(CreateThread(nullptr, 0, proc, arg, CREATE_SUSPENDED, &tid));
        if (nullptr == thr.get())
            EP_DIE("CreateThread failed.");
    }

    DWORD run() const { return ResumeThread(thr.get()); }

    event_t msgq_ready, quitting;
    std::unique_ptr<void, del> thr;
    DWORD tid;
};

class critsec_t {
    struct del {
        void operator()(LPCRITICAL_SECTION x) { DeleteCriticalSection(x); }
    };
public:
    explicit critsec_t()
        : cs(new CRITICAL_SECTION) {
        InitializeCriticalSection(cs.get());
    }

    explicit critsec_t(const DWORD spincount)
        : cs(new CRITICAL_SECTION) {
        InitializeCriticalSectionAndSpinCount(cs.get(), spincount);
    }

    void lock() const { EnterCriticalSection(cs.get()); }
    void unlock() const { LeaveCriticalSection(cs.get()); }
    BOOL try_lock() const { return TryEnterCriticalSection(cs.get()); }

    DWORD set_spincount(const DWORD spincount) const {
        SetCriticalSectionSpinCount(cs.get(), spincount);
    }

    std::unique_ptr<CRITICAL_SECTION, del> cs;
};

//struct barrier_t {
//    explicit barrier_t()
//        : waiting(0)
//    {}
//
//    bool wait(const unsigned long n) {
//        if (waiting++ <= n) {
//            cv.notify_all();
//            waiting.exchange(0);
//            return true;
//        } else {
//            std::unique_lock<critsec_t> lock(mutex);
//
//            cv.wait(lock);
//            return false;
//        }
//    }
//
//    std::atomic<unsigned long> waiting;
//    std::condition_variable cv;
//    critsec_t mutex;
//};

void open_console(void);
}
}