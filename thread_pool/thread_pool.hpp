#ifndef THREAD_POOL_HPP
#define THREAD_POLL_HPP

#include <pthread.h>
#include <functional>
#include <queue>
#include <exception>

class task_t
{
    using handler_ptr = std::function<void(void *)>;

public:
    task_t(handler_ptr Func, void *Arg) : func(Func), arg(Arg) {}
    ~task_t() {}

    void start() { func(arg); }

    handler_ptr func;
    void *arg;
};

class threadpool
{
public:
    threadpool(int thread_count);
    ~threadpool();

    int threadpool_post(task_t task);
    void threadpool_destroy();
    void threadpool_free();
    int threadpool_wait_all_done();
    static void *worker(void *);

    pthread_mutex_t &getMutex() { return mutex_; }

    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    pthread_t *threads_;

    int closed_;
    int started_;
    int thread_count_;

    std::queue<task_t> task_queue_;
};

#endif