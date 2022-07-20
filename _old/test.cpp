#include "MemoryPool.h"
#include <sys/time.h>
#include <stdio.h>

#include <pthread.h>

pthread_mutex_t lock;

time_t poolTime;
time_t sysTime;
int count1;
int count2;
int allSize;

LYW_CODE::MemoryPool * mPool;

int tag;

void * test(void * xxx)
{

    struct timeval begin, end;
    int iLoop = 10000;
    unsigned long t = 0;




    void ** array = (void **)malloc(sizeof(void *) * count2);

    if (tag == 0)
    {
        iLoop = count1;
        gettimeofday(&begin,NULL);
        while(iLoop--)
        {
            for(int index = 0; index < count2; index++ )
            {
                array[index] = ::malloc(allSize);
            }

            gettimeofday(&begin,NULL);
 

            for(int index = 0; index < count2; index++ )
            {
                ::free(array[index]);
            }

            gettimeofday(&end,NULL);
            t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
 
        }
        printf("sys %ld\n", t);
        pthread_mutex_lock(&lock);
        sysTime += t;
        pthread_mutex_unlock(&lock);

    }
    else
    {
        iLoop = count1;
        gettimeofday(&begin,NULL);
        while(iLoop--)
        {
            for(int index = 0; index < count2; index++ )
            {
                pthread_mutex_lock(&lock);
                array[index] =  mPool->malloc(allSize);
                pthread_mutex_unlock(&lock);
            }

            gettimeofday(&begin,NULL);

            for(int index = 0; index < count2; index++ )
            {
                pthread_mutex_lock(&lock);
                mPool->free(array[index]);
                pthread_mutex_unlock(&lock);
            }

            gettimeofday(&end,NULL);
            t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
            
        }

        printf("pool %ld\n", t);
        pthread_mutex_lock(&lock);
        poolTime += t;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}


int main(int argc, char * argv[])
{
    pthread_mutex_init(&lock,NULL);

    mPool = new LYW_CODE::MemoryPool();

    poolTime = 0;
    sysTime = 0;

    count1 = 1000;
    count2 = 1000;
    allSize = 32;

    tag = 0;   
    int threadNum = 1;

    if (argc >= 2)
    {
        tag = atoi(argv[1]);
    }

    if (argc >= 3)
    {
        threadNum = atoi(argv[2]);
    }

    if (argc >= 4)
    {
        count1 = atoi(argv[3]);
    }

    if (argc >= 5)
    {
        count2 = atoi(argv[4]);
    }

    if (argc >= 6)
    {
        allSize = atoi(argv[5]);
    }





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

