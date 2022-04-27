#include "MemoryPool.h"
#include <sys/time.h>
#include <stdio.h>

#include <pthread.h>

pthread_mutex_t lock;

time_t poolTime;
time_t sysTime;

LYW_CODE::MemoryPool * mPool;

void * test(void * xxx)
{

    struct timeval begin, end;
    int iLoop = 10000;
    void * p ;
    void * p1 ;
    void * p2 ;
    void * p3 ;
    void * p4 ;
    unsigned long t = 0;


    void ** array = (void **)malloc(sizeof(void *) * 2000);


    iLoop = 5000;
    gettimeofday(&begin,NULL);
    while(iLoop--)
    {
        for(int index = 0; index < 1024; index++ )
        {
            pthread_mutex_lock(&lock);
            //array[index] =  mPool->malloc(32);
            array[index] =  ::malloc(iLoop / 16 + 1);
            pthread_mutex_unlock(&lock);
        }

        for(int index = 0; index < 1024; index++ )
        {
            pthread_mutex_lock(&lock);
            mPool->free(array[index]);
            pthread_mutex_unlock(&lock);
        }
        
    }
    gettimeofday(&end,NULL);
    
    t = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);

    printf("pool %ld\n", t);
    pthread_mutex_lock(&lock);
    poolTime += t;
    pthread_mutex_unlock(&lock);

    iLoop = 5000;
    gettimeofday(&begin,NULL);
    while(iLoop--)
    {
        for(int index = 0; index < 1024; index++ )
        {
           array[index] =  ::malloc(iLoop / 16 + 1);
        }

        for(int index = 0; index < 1024; index++ )
        {
            ::free(array[index]);
        }
 
    }
    gettimeofday(&end,NULL);

    t = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
    printf("sys %ld\n", t);
    pthread_mutex_lock(&lock);
    sysTime += t;
    pthread_mutex_unlock(&lock);



    return NULL;
}


int main()
{
    pthread_mutex_init(&lock,NULL);

    mPool = new LYW_CODE::MemoryPool();

    poolTime = 0;
    sysTime = 0;

    int threadNum = 1;

    pthread_t * h = (pthread_t *)::malloc(sizeof(pthread_t) *threadNum);

    for (int iLoop = 0; iLoop < threadNum; iLoop++)
    {
        pthread_create(h +iLoop, NULL, test, NULL);
    }


    for (int iLoop = 0; iLoop < threadNum; iLoop++)
    {
        pthread_join(h[iLoop], NULL);
    }
    
    printf("sys %ld  pool %ld\n", sysTime, poolTime);
    return 0;
}

