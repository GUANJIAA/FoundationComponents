#include <pthread.h>
#include <unistd.h>

#include "thread_pool.hpp"

int nums = 0;
int done = 0;

pthread_mutex_t lock;

void do_task(void *arg)
{
    usleep(10000);
    pthread_mutex_lock(&lock);
    done++;
    printf("doing %d task\n", done);
    pthread_mutex_unlock(&lock);
}

int main()
{
    int threads = 4;

    threadpool *pool = new threadpool(threads);
    if (pool == nullptr)
    {
        printf("thread pool create error!\n");
        return 0;
    }

    while (pool->threadpool_post(task_t(&do_task, nullptr)) == 0)
    {
        pthread_mutex_lock(&lock);
        nums++;
        pthread_mutex_unlock(&lock);
    }

    printf("add %d tasks\n", nums);

    printf("-------------------------------------------------");

    pool->threadpool_wait_all_done();

    printf("did %d tasks\n", done);
    pool->threadpool_destroy();
    return 0;
}
