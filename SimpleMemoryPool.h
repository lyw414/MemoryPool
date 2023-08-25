#ifndef __LYW_CODE_SIMPLE_MEMORY_POOL_H_FILE__
#define __LYW_CODE_SIMPLE_MEMORY_POOL_H_FILE__
#include "ThreadInstance.hpp"

#include <list>
#include <stdlib.h>

namespace LYW_CODE
{
    class ProcessMemoryPool
    {
        friend class SimpleMemoryPool;
        private:
            typedef struct _Block
            {
                //身份校验 
                class SimpleMemoryPool *self;

                //链表
                struct _Block * next;
                struct _Block * pre;

                //线程块
                struct _Block * threadNext;
                struct _Block * threadPre;

                //引用计数 -- 引用计数
                int count;
                //块大小
                int size;
                //分配索引
                int index;
                //其他线程释放块 -- 原子操作
                int threadFreeCount;
                unsigned char ptr[0];
            } Block_t;

            typedef struct _Node
            {
                //所属块
                Block_t * block;
                //分配指针
                unsigned char ptr[0];
            } Node_t;


            Block_t m_allocateBlock;

        private:
            //资源锁
            pthread_mutex_t m_lock;

        private:
            ProcessMemoryPool();

            ~ProcessMemoryPool();

            static ProcessMemoryPool * GetInstance()
            {
                static ProcessMemoryPool * self = new ProcessMemoryPool();
                return self;
            }

            Block_t * Allocate(unsigned int size);

            void FreeBlock(Block_t * ptr);

            void FreeDirtyBlock(Block_t * ptr);


    };

    class SimpleMemoryPool : public ThreadInstance<LYW_CODE::SimpleMemoryPool>
    {
        friend class ThreadInstance<LYW_CODE::SimpleMemoryPool>;
        private:
            //空闲块
            ProcessMemoryPool::Block_t * m_freeBlockBegin;
            ProcessMemoryPool::Block_t * m_freeBlockEnd;
            //空闲块总大小
            unsigned int m_freeBlockSize;

            //占用块
            ProcessMemoryPool::Block_t * m_occupyBlockBegin;
            ProcessMemoryPool::Block_t * m_occupyBlockEnd;
            //占用块总大小
            unsigned int m_occupyBlockSize;

            //检测块 -- 每次分配无内存时，检测一次
            ProcessMemoryPool::Block_t * m_threadBlockBegin;
            ProcessMemoryPool::Block_t * m_threadBlockEnd;
            pthread_mutex_t m_lock;

        private:
            SimpleMemoryPool();

            void FreeBlockPushBack(ProcessMemoryPool::Block_t * block);
            ProcessMemoryPool::Block_t * FreeBlockPopFront();

            void OccupyBlockPushBack(ProcessMemoryPool::Block_t * block);
            void OccupyBlockPushFront(ProcessMemoryPool::Block_t * block);
            ProcessMemoryPool::Block_t * OccupyBlockPopFront();
            void OccupyBlockRemove(ProcessMemoryPool::Block_t * block);

            void ThreadBlockPushBack(ProcessMemoryPool::Block_t * block);
            void ThreadBlockRemove(ProcessMemoryPool::Block_t * block);
            ProcessMemoryPool::Block_t * CheckOccupyBlock();

            void ClearFreeBlock();

        public:
            ~SimpleMemoryPool();
            void * Malloc(unsigned int size);
            void Free(void * ptr);
    };
}
#endif
