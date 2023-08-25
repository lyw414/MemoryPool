#include "SimpleMemoryPool.h"
#include <unistd.h>
#include <stdio.h>
namespace LYW_CODE
{
    ProcessMemoryPool::ProcessMemoryPool()
    {
        pthread_mutex_init(&m_lock, NULL);
    }

    ProcessMemoryPool::~ProcessMemoryPool()
    {
        pthread_mutex_destroy(&m_lock);
    }

    ProcessMemoryPool::Block_t * ProcessMemoryPool::Allocate(unsigned int size)
    {
        return (Block_t *)::malloc(size + sizeof(Block_t));
    }

    void ProcessMemoryPool::FreeBlock(Block_t * ptr)
    {
        return ::free(ptr);
    }

    void ProcessMemoryPool::FreeDirtyBlock(Block_t * ptr)
    {
        //暂时直接释放
        return ::free(ptr);
    }

    SimpleMemoryPool::SimpleMemoryPool()
    {
        m_freeBlockBegin = NULL;
        m_freeBlockEnd = NULL;

        m_occupyBlockBegin= NULL;
        m_occupyBlockEnd = NULL;

        m_freeBlockSize = 0;
        m_occupyBlockSize = 0;

        m_threadBlockEnd = NULL;
        m_threadBlockBegin = NULL;

        pthread_mutex_init(&m_lock, NULL);

    }

    SimpleMemoryPool::~SimpleMemoryPool()
    {
        //释放所有空闲块
        ProcessMemoryPool::Block_t * curNode = m_freeBlockBegin;
        ProcessMemoryPool::Block_t * nextNode = NULL;
        while (curNode != NULL)
        {
            nextNode = curNode->next;
            ProcessMemoryPool::GetInstance()->FreeBlock(curNode);
            curNode = nextNode;
        }
        

        //释放所有占用块
        curNode = m_occupyBlockBegin;
        nextNode = NULL;
        while (curNode != NULL)
        {
            nextNode = curNode->next;
            if (curNode->count == 0)
            {
                ProcessMemoryPool::GetInstance()->FreeBlock(curNode);
            }
            else
            {
                //脏块 -- 存在未释放资源 丢给ProcessMemoryPool 管理
                curNode->self = NULL;
                ProcessMemoryPool::GetInstance()->FreeDirtyBlock(curNode);
            }
            curNode = nextNode;
        }

        //延迟1s结束
        usleep(1000);
    }

    void *SimpleMemoryPool::Malloc(unsigned int size)
    {
        ProcessMemoryPool::Block_t * block;
        ProcessMemoryPool::Node_t* node;
        int needSize = size + sizeof(ProcessMemoryPool::Node_t);

        //大块
        if (needSize >= 8192)
        {
            if ((block = ProcessMemoryPool::GetInstance()->Allocate(needSize)) == NULL)
            {
                return NULL;
            }

            block->self = this;
            block->size = needSize;
            block->count = 1;
            block->threadFreeCount = 0;
            block->index = needSize;

            node = (ProcessMemoryPool::Node_t *)block->ptr;
            node->block = block;
            OccupyBlockPushBack(block);
            return node->ptr;
        }

        //正常块
        if ((m_occupyBlockBegin != NULL) && (m_occupyBlockBegin->size - m_occupyBlockBegin->index) >= needSize)
        {
            //当前占用块足够分配
            block = m_occupyBlockBegin;
        }
        else
        {

            //查看是否由空闲块 
            if (m_freeBlockBegin != NULL)
            {
                //使用空闲块
                block = m_freeBlockBegin;
                //空闲列表弹出
                FreeBlockPopFront();
                //添加至占用列表
                OccupyBlockPushFront(block);
                
                //清理空闲块
                ClearFreeBlock();

            }
            else
            {
                if ((block = CheckOccupyBlock()) == NULL)
                {
                    //探测块失败 创建新块
                    if ((block = ProcessMemoryPool::GetInstance()->Allocate(8192)) == NULL)
                    {

                        return NULL;
                    }


                    block->self = this;
                    block->size = 8192;
                    block->index = 0;
                    block->count = 0;
                    block->threadFreeCount = 0;
                    OccupyBlockPushFront(block);
                }
                else
                {
                    //使用空闲块
                    block = m_freeBlockBegin;
                    //空闲列表弹出
                    FreeBlockPopFront();
                    //添加至占用列表
                    OccupyBlockPushFront(block);

                }
            }

        }
        
        //分配
        node = (ProcessMemoryPool::Node_t *)(block->ptr + block->index);
        block->count++;
        block->index += needSize;
        node->block = block;


    
        return  node->ptr;

    }

    void SimpleMemoryPool::Free(void *ptr)
    {
        ProcessMemoryPool::Block_t * block;
        ProcessMemoryPool::Node_t * node;
        
        if (ptr != NULL)
        {
            node = (ProcessMemoryPool::Node_t *)((unsigned char *)ptr - sizeof(ProcessMemoryPool::Block_t *));
            block = node->block;

            if (block->self == this)
            {
                block->count--;
                //__sync_fetch_and_sub(&(block->count), 1);
                if (block->count == block->threadFreeCount)
                {
                    if (block->threadFreeCount != 0)
                    {
                        pthread_mutex_lock(&m_lock);
                        ThreadBlockRemove(block);
                        pthread_mutex_unlock(&m_lock);
                    }

                    block->count = 0;
                    block->index = 0;
                    block->threadFreeCount = 0;
                    OccupyBlockRemove(block);
                    FreeBlockPushBack(block);
                }
            }
            else
            {
                if (block->self != NULL)
                {
                    if (block->threadFreeCount == 0)
                    {
                        pthread_mutex_lock(&(block->self->m_lock));
                        if (block->threadFreeCount == 0)
                        {
                            block->self->ThreadBlockPushBack(block);
                            __sync_fetch_and_add(&(block->threadFreeCount), 1);
                            pthread_mutex_unlock(&(block->self->m_lock));
                        }
                        else
                        {
                            pthread_mutex_unlock(&(block->self->m_lock));
                            __sync_fetch_and_add(&(block->threadFreeCount), 1);
                        }
                    }
                    else
                    {
                        __sync_fetch_and_add(&(block->threadFreeCount), 1);
                    }
                }
                else
                {
                    __sync_fetch_and_add(&(block->threadFreeCount), 1);
                }

            }
        }


        return;
    }


    ProcessMemoryPool::Block_t * SimpleMemoryPool::CheckOccupyBlock()
    {
        ProcessMemoryPool::Block_t * block = m_threadBlockBegin;
        ProcessMemoryPool::Block_t * tag = NULL;
        if (block == NULL)
        {
            return NULL;
        }

        do 
        {
            if (block->count == block->threadFreeCount)
            {
                pthread_mutex_lock(&m_lock);
                ThreadBlockRemove(block);
                pthread_mutex_unlock(&m_lock);

                OccupyBlockRemove(block);
                FreeBlockPushBack(block);
                

                block->count = 0;
                block->threadFreeCount = 0;
                block->index = 0;

                tag = block;

                //return block;
            }

            pthread_mutex_lock(&m_lock);
            block = block->next;
            pthread_mutex_unlock(&m_lock);

        } while (block != NULL);


        return tag;
    }

    void SimpleMemoryPool::FreeBlockPushBack(ProcessMemoryPool::Block_t * block)
    {
        if (m_freeBlockBegin == NULL)
        {
            m_freeBlockEnd = m_freeBlockBegin = block;
            block->pre = block->next = NULL;
        }
        else
        {
            m_freeBlockEnd->next = block;
            block->pre = m_freeBlockEnd;
            m_freeBlockEnd = block;
            block->next = NULL;
        }

        m_freeBlockSize += block->size;

        return;
    }


    ProcessMemoryPool::Block_t * SimpleMemoryPool::FreeBlockPopFront()
    {
        ProcessMemoryPool::Block_t * block = m_freeBlockBegin;

        if (block != NULL)
        {
            if (block->next != NULL)
            {
                m_freeBlockBegin = block->next;
                m_freeBlockBegin->pre = NULL;
            }
            else
            {
                m_freeBlockBegin = m_freeBlockEnd = NULL;
            }
        }

        m_freeBlockSize -= block->size;

        return block;
    }

    void SimpleMemoryPool::OccupyBlockPushBack(ProcessMemoryPool::Block_t * block)
    {
        if (m_occupyBlockBegin == NULL)
        {
            m_occupyBlockBegin = m_occupyBlockEnd = block;
            block->pre = block->next = NULL;
        }
        else
        {
            m_occupyBlockEnd->next = block;
            block->pre = m_occupyBlockEnd;
            m_occupyBlockEnd = block;
            block->next = NULL;
        }

        m_occupyBlockSize += block->size;
        return;
    }

    void SimpleMemoryPool::OccupyBlockPushFront(ProcessMemoryPool::Block_t * block)
    {
        if (m_occupyBlockBegin == NULL) 
        {
            m_occupyBlockEnd = m_occupyBlockBegin = block;
            block->pre = block->next = NULL;
        }
        else
        {
            block->next = m_occupyBlockBegin;
            m_occupyBlockBegin->pre = block;
            block->pre = NULL;
            m_occupyBlockBegin = block;
        }

        m_occupyBlockSize += block->size;

        return;
    }

    ProcessMemoryPool::Block_t * SimpleMemoryPool::OccupyBlockPopFront()
    {
        ProcessMemoryPool::Block_t * block = m_occupyBlockBegin;

        if (block != NULL)
        {
            if (block->next != NULL)
            {
                m_occupyBlockBegin = m_occupyBlockBegin->next;
                m_occupyBlockBegin->pre = NULL;
            }
            else
            {
                m_occupyBlockBegin = m_occupyBlockEnd = NULL;
            }
        }

        m_occupyBlockSize -= block->size;

        return block;
    }

    void SimpleMemoryPool::OccupyBlockRemove(ProcessMemoryPool::Block_t * block)
    {
        m_occupyBlockSize -= block->size;
        //开始节点 或者只剩一个节点
        if (block->pre == NULL)
        {
            OccupyBlockPopFront();
            return;
        }
        
        //尾巴节点，且有多个
        if (block->next == NULL)
        {
            m_occupyBlockEnd = block->pre;
            block->pre->next = NULL;
            return;
        }

        //中间节点
        block->pre->next = block->next;
        block->next->pre = block->pre;

        
        return;
    }

    void SimpleMemoryPool::ThreadBlockPushBack(ProcessMemoryPool::Block_t * block)
    {
        if (m_threadBlockBegin == NULL)
        {
            m_threadBlockBegin = m_threadBlockEnd = block;
            block->threadPre = block->threadNext = NULL;
        }
        else
        {
            m_threadBlockEnd->threadNext = block;
            block->threadPre = m_threadBlockEnd;
            m_threadBlockEnd = block;
            block->threadNext = NULL;
        }

        return;
 
    }
    void SimpleMemoryPool::ThreadBlockRemove(ProcessMemoryPool::Block_t * block)
    {
        if (block == NULL)
        {
            return;
        }

        //唯一节点
        if (block->threadPre == NULL && block->threadNext == NULL)
        {
            m_threadBlockBegin = m_threadBlockEnd = NULL;
            return;
        }
        
        //首节点 有尾节点
        if (block->threadPre == NULL)
        {
            m_threadBlockBegin = block->threadNext;
            block->threadNext->threadPre = NULL;
            return;
        }
        
        //尾节点 有首节点
        if (block->threadNext == NULL)
        {
            m_threadBlockEnd = block->threadPre;
            block->threadPre->threadNext = NULL;
            return;
        }

        //中间节点
        block->threadPre->threadNext = block->threadNext;
        block->threadNext->threadPre = block->threadPre;
        return;
    }


    void SimpleMemoryPool:: ClearFreeBlock()
    {
        while (m_freeBlockSize * 2 > m_occupyBlockSize)
        {
            ProcessMemoryPool::Block_t * block = FreeBlockPopFront();
            if (block != NULL)
            {
                ProcessMemoryPool::GetInstance()->FreeBlock(block);
            }
            else
            {
                break;
            }
        }
    }
}

