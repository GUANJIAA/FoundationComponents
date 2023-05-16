#include "thread_pool.hpp"

threadpool::threadpool(int thread_count)
{
    if (thread_count <= 0)
    {
        throw std::exception();
    }
    threads_ = new pthread_t[thread_count];
    if (threads_ == nullptr)
    {
        throw std::exception();
    }
    for (int i = 0; i < thread_count; i++)
    {
        if (pthread_create(&(threads_[i]), NULL, worker, this) != 0)
        {
            throw std::exception();
        }
        thread_count_++;
        started_++;
    }
}

threadpool::~threadpool()
{
    threadpool_destroy();
}

int threadpool::threadpool_post(task_t task)
{
    if (pthread_mutex_lock(&mutex_) != 0)
    {
        return 1;
    }
    if (closed_ == 1)
    {
        pthread_mutex_unlock(&mutex_);
        return 2;
    }
    task_queue_.push(task);
    if (pthread_cond_signal(&cond_) != 0)
    {
        pthread_mutex_unlock(&mutex_);
        return 3;
    }
    pthread_mutex_unlock(&mutex_);
    return 0;
}

void *threadpool::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    task_t task(nullptr, nullptr);
    while (1)
    {
        pthread_mutex_lock(&(pool->mutex_));

        while (pool->task_queue_.empty() && pool->closed_ == 0)
        {
            pthread_cond_wait(&(pool->cond_), &(pool->mutex_));
        }

        if (pool->closed_ == 1)
            break;
        task.func = pool->task_queue_.front().func;
        task.arg = pool->task_queue_.front().arg;
        pool->task_queue_.pop();
        pthread_mutex_unlock(&(pool->mutex_));
        task.start();
    }
    pool->started_--;
    pthread_mutex_unlock(&pool->mutex_);
    pthread_exit(NULL);
    return nullptr;
}

void threadpool::threadpool_destroy()
{
    if (closed_)
    {
        threadpool_free();
        return;
    }

    closed_ = 1;

    if (pthread_cond_broadcast(&cond_) != 0 ||
        pthread_mutex_unlock(&mutex_) != 0)
    {
        threadpool_free();
        return;
    }

    threadpool_wait_all_done();

    threadpool_free();
}

void threadpool::threadpool_free()
{
    if (started_ > 0)
    {
        return;
    }

    if (threads_)
    {
        delete[] threads_;
        threads_ = nullptr;

        pthread_mutex_lock(&mutex_);
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&cond_);
    }

    while (!task_queue_.empty())
    {
        task_queue_.pop();
    }
}

int threadpool::threadpool_wait_all_done()
{
    int result = 0;
    for (int i = 0; i < thread_count_; i++)
    {
        if (pthread_join(threads_[i], nullptr))
        {
            result = 1;
        }
    }
    return result;
}