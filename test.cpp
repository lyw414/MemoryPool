#include "MemoryPool.h"
#include <sys/time.h>
#include <stdio.h>
#include <list>
#include <unistd.h>

time_t poolTime;
time_t sysTime;
int count1;
int count2;
int allSize;

LYW_CODE::SimpleMemoryPool::MemoryPool *  mPool;

pthread_mutex_t lock;

int tag;

std::list <void *> ptrList;

int endCount;

int syncTag;


void * test_free(void * xxx)
{
    void * ptr = NULL;
    struct timeval begin, end;
    unsigned long t = 0;

    std::list<void * > myList;

    std::list<void * >::iterator it;

    int handle = mPool->Regist();
    
    while(true)
    {
        if (!ptrList.empty())
        {
            ::pthread_mutex_lock(&lock);
            myList = ptrList;
            ptrList.clear();
            ::pthread_mutex_unlock(&lock);
        }
        else
        {

            if (endCount <= 0)
            {
                break;
            }
            usleep(10000);
            continue;
        }

        for (it = myList.begin(); it != myList.end(); it++)
        {
            ptr = *it;
            if (tag == 0)
            {
                gettimeofday(&begin,NULL);
                ::free(ptr);
                gettimeofday(&end,NULL);
                t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
 
            }
            else
            {
                gettimeofday(&begin,NULL);
                mPool->Free(handle, ptr);
                gettimeofday(&end,NULL);
                t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
            }
        }

        myList.clear();

    }
    if (tag == 0)
    {
        printf("sys Free %ld\n", t);
        pthread_mutex_lock(&lock);
        sysTime += t;
        pthread_mutex_unlock(&lock);

    }
    else
    {
        printf("pool Free %ld\n", t);
        pthread_mutex_lock(&lock);
        poolTime += t;
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

void * test(void * xxx)
{
    //LYW_CODE::SimpleMemoryPool * mPool;
    //mPool = new LYW_CODE::SimpleMemoryPool(allSize, 1024 * 1024);
    struct timeval begin, end;
    int iLoop = 10000;
    
    unsigned long t = 0;

    unsigned int as = 0;

    void ** array = (void **)malloc(sizeof(void *) * count2);
    void * ptr;
    std::list<void *> myList;
    srand(time(NULL));

    int handle = mPool->Regist();

    if (tag == 0)
    {
        iLoop = count1;
        gettimeofday(&begin,NULL);

        if (syncTag == 0)
        {
            while(iLoop--)
            {
                for(int index = 0; index < count2; index++ )
                {
                    as = rand();
                    as = as % allSize;
                    gettimeofday(&begin,NULL);
                    ptr = ::malloc(as + 1);
                    //ptr = ::malloc(allSize);
                    gettimeofday(&end,NULL);
                    array[index] = ptr;
                    t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);

                    myList.push_back(ptr);

                    if (ptr == NULL)
                    {
                        printf("Error\n");
                    }
                }

                pthread_mutex_lock(&lock);
                ptrList.merge(myList);
                pthread_mutex_unlock(&lock);
                myList.clear();
            }
        }
        else
        {
            while(iLoop--)
            {
                for(int index = 0; index < count2; index++ )
                {
                    as = rand();
                    as = as % allSize;
                    gettimeofday(&begin,NULL);
                    ptr = ::malloc(as + 1);
                    //ptr = ::malloc(allSize);
                    gettimeofday(&end,NULL);
                    array[index] = ptr;
                    t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);


                    if (ptr == NULL)
                    {
                        printf("Error\n");
                    }
                }

                for(int index = 0; index < count2; index++ )
                {
                    gettimeofday(&begin,NULL);
                    ::free(array[index]);
                    gettimeofday(&end,NULL);
                    t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
                }
            }

        }


        printf("sys malloc %ld\n", t);
        pthread_mutex_lock(&lock);
        sysTime += t;
        endCount--;
        pthread_mutex_unlock(&lock);

    }
    else
    {
        iLoop = count1;
        gettimeofday(&begin,NULL);

        if (syncTag == 0)
        {
            while(iLoop--)
            {
                for(int index = 0; index < count2; index++ )
                {
                    as = rand();
                    as = as % allSize;

                    gettimeofday(&begin,NULL);
                    //ptr = mPool->Malloc(as + 1);
                    //ptr = mPool->Malloc(handle, allSize);
                    ptr = mPool->Malloc(handle, as + 1);
                    //ptr = mPool->Malloc(allSize);

                    gettimeofday(&end,NULL);
                    array[index] = ptr;
                    t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);

                    myList.push_back(ptr);

                    if (ptr == NULL)
                    {
                        printf("Error\n");
                    }
                }

                pthread_mutex_lock(&lock);
                ptrList.merge(myList);
                pthread_mutex_unlock(&lock);
                myList.clear();



                //for(int index = 0; index < count2; index++ )
                //{
                //    gettimeofday(&begin,NULL);
                //    mPool->Free(handle, array[index]);
                //    gettimeofday(&end,NULL);
                //    t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
                //}

                //t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
                
            }



        }
        else
        {
            while(iLoop--)
            {
                for(int index = 0; index < count2; index++ )
                {
                    as = rand();
                    as = as % allSize;

                    gettimeofday(&begin,NULL);
                    //ptr = mPool->Malloc(as + 1);
                    //ptr = mPool->Malloc(handle, allSize);
                    ptr = mPool->Malloc(handle, as + 1);
                    //ptr = mPool->Malloc(allSize);

                    gettimeofday(&end,NULL);
                    array[index] = ptr;
                    t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);


                    if (ptr == NULL)
                    {
                        printf("Error\n");
                    }
                }

                for(int index = 0; index < count2; index++ )
                {
                    gettimeofday(&begin,NULL);
                    mPool->Free(handle, array[index]);
                    gettimeofday(&end,NULL);
                    t += (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
                }

            }


        }
        printf("pool malloc %ld\n", t);
        pthread_mutex_lock(&lock);
        poolTime += t;
        endCount--;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}


int main(int argc, char * argv[])
{
    pthread_mutex_init(&lock,NULL);


    poolTime = 0;
    sysTime = 0;

    count1 = 1000;
    count2 = 1000;
    allSize = 32;

    tag = 0;   
    int threadNum = 1;
    int threadNum1 = 1;

    if (argc >= 2)
    {
        tag = atoi(argv[1]);
    }

    if (argc >= 3)
    {
        endCount = threadNum = atoi(argv[2]);
    }

    if (argc >= 4)
    {
        threadNum1 = atoi(argv[3]);
    }



    if (argc >= 5)
    {
        count1 = atoi(argv[4]);
    }

    if (argc >= 6)
    {
        count2 = atoi(argv[5]);
    }

    if (argc >= 7)
    {
        allSize = atoi(argv[6]);
    }

    if (argc >= 8)
    {
        syncTag = atoi(argv[7]);
    }



    mPool = new LYW_CODE::SimpleMemoryPool::MemoryPool() ;
    //new LYW_CODE::SimpleMemoryPool(allSize, 1024 * 1024 * 10);

    pthread_t * h = (pthread_t *)::malloc(sizeof(pthread_t) *threadNum);
    pthread_t * h1 = (pthread_t *)::malloc(sizeof(pthread_t) * threadNum1);

    for (int iLoop = 0; iLoop < threadNum; iLoop++)
    {
        pthread_create(h +iLoop, NULL, test, NULL);
    }

    for (int iLoop = 0; iLoop < threadNum1; iLoop++)
    {
        pthread_create(h1 +iLoop, NULL, test_free, NULL);
    }


    for (int iLoop = 0; iLoop < threadNum; iLoop++)
    {
        pthread_join(h[iLoop], NULL);
    }
    
    for (int iLoop = 0; iLoop < threadNum1; iLoop++)
    {
        pthread_join(h1[iLoop], NULL);
    }
 
    printf("sys %ld  pool %ld\n", sysTime, poolTime);
    return 0;
}

