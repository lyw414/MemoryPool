#include "MemoryPool.h"
namespace LYW_CODE
{
    MemoryPool::MemoryPool(unsigned long maxSize, unsigned short freeProportion, unsigned long minBlockSize)
    {
        //初始化
        m_maxSize = maxSize;
        
        if (freeProportion > 100)
        {
            m_freeProportion = 30;
        }
        else
        {
            m_freeProportion = freeProportion;
        }
        
        m_minBlockSize = minBlockSize;

        m_maxExponent = 32;

        m_totalSize = 0;

        m_freeSize = 0;

        m_freeBlock = NULL;


        m_allocBlockArray = (Block_t **)::malloc(sizeof(Block_t *) * m_maxExponent);

        ::memset(m_allocBlockArray, 0x00, sizeof(Block_t *) * m_maxExponent);

        m_clearInterval = 1000; //1s


        ::gettimeofday(&m_lastClearTime, NULL);

    }

    MemoryPool::~MemoryPool()
    {
        Block_t * pBlock = NULL;
        Block_t * tmp = NULL;

        //可分配块释放 
        if (m_allocBlockArray != NULL)
        {
            for (int iLoop = 0; iLoop < m_maxExponent; iLoop++)
            {
                pBlock = m_allocBlockArray[iLoop];
                while (pBlock != NULL)
                {
                    tmp = pBlock->next;
                    ::free(pBlock);
                    pBlock = tmp;
                }
            }
            ::free(m_allocBlockArray);
            m_allocBlockArray = NULL;
        }
    }

    unsigned long MemoryPool::size(DataBegin_t * data)
    {
        unsigned long len =  0;
        if (data != NULL)
        {
            len = (char *)data->end - (char *)data - sizeof(DataBegin_t) - sizeof(DataStatus_t);
        }
        return len;
    }

    Block_t * MemoryPool::newBlock(unsigned long size)
    {
        unsigned long realSize = 0;

        short ex = 0;

        ex = Exponent(size);
            
        Block_t * block = NULL;
        DataBegin_t * begin = NULL;
        DataStatus_t * status = NULL;
        DataEnd_t * end = NULL;


        realSize += sizeof(Block_t);
        realSize += sizeof(DataBegin_t);
        realSize += sizeof(DataStatus_t);
        realSize += sizeof(DataEnd_t);
        
        if (ex < 0  || ex >= m_maxExponent)
        {
            return NULL;
        }

        if (size > m_minBlockSize)
        {
            realSize += size;
            m_freeSize += size;
        }
        else
        {
            realSize += m_minBlockSize;
            m_freeSize += m_minBlockSize;
        }

        m_totalSize += realSize;

        block = (Block_t *)::malloc(realSize);

        begin =(DataBegin_t *)((char *)block + sizeof(Block_t));

        status = (DataStatus_t *)((char *)begin + sizeof(Block_t));

        end = (DataEnd_t *)((char *)block + realSize - sizeof(DataEnd_t));
        
        block->size = realSize;

        block->occupyNum = 0;

        block->freeData = begin;

        block->next = NULL;

        block->before = NULL;

        block->fnext = NULL;

        block->fbefore = NULL;

        begin->block = block;

        begin->end = end;

        status->st = 0xA0;

        end->begin = begin;

        return  block;
    }

    /**
     * @brief   长度指数计算
     *
     * @return      成功 >= 0
     *              失败 <  0
     */
    inline short MemoryPool::Exponent(unsigned long number)
    {
        short ex = 0;

        //if (number == 0)
        //{
        //    return -1;
        //}

        //number = number >> 1;
        
        while (number != 0)
        {
            number = number >> 1;
            ex++;
        }
        return ex;
    }

    /**
     * @brief       分配一块内存 使用过程严禁越界
     *
     * @return      成功 可用内存地址
     *              失败 NULL 
     */
    void * MemoryPool::malloc(unsigned long size)
    {
        short ex = Exponent(size);

        Block_t * freeBlock = NULL;

        void * res = NULL;

        unsigned long index = 0;
        
        if (ex <= 0 || ex >= m_maxExponent)
        {
            return res;
        }
        
        //空闲块中查找
        if (size > (1 << (ex - 1)))
        {
            //处理最小值情况 如 size 为 4 ex = 2 则在 array[2]中查询即可(array[2] >= 4) 如size为5 ex = 2 则应在 array[3] 中查找
            ex++;
        }
        
        //查找可用块
        for (int iLoop = ex; iLoop < m_maxExponent; iLoop++)
        {
            if (m_allocBlockArray[iLoop] != NULL)
            {
                //可用块中移除
                freeBlock = m_allocBlockArray[iLoop];
                m_allocBlockArray[iLoop] = freeBlock->next;
                if (freeBlock->next != NULL)
                {
                    freeBlock->next->before = NULL;
                }

                freeBlock->next = NULL;

                //空闲块中移除
                if (m_freeBlock != NULL)
                {
                    if (freeBlock->fnext != NULL)
                    {
                        freeBlock->fnext->fbefore = freeBlock->fbefore;
                    }

                    if (freeBlock->fbefore != NULL)
                    {
                        freeBlock->fbefore->fnext = freeBlock->fnext;
                    }
                    else
                    {
                        m_freeBlock = freeBlock->fnext;
                    }

                    freeBlock->fnext = NULL;
                    freeBlock->fbefore = NULL;
                }

                break;
            }
        }
        
        if (freeBlock == NULL)
        {
            //最大可分配空间检查
            if (m_maxSize > 0 && m_totalSize >= m_maxSize)
            {
                return NULL;
            }

            //分配新块
            if ((freeBlock = newBlock(size)) == NULL)
            {
                return NULL;
            }
        }

        //块错误 添加至满块中 不再进入分配
        if (freeBlock->freeData == NULL)
        {
            if (m_allocBlockArray[0] != NULL)
            {
                m_allocBlockArray[0]->before = freeBlock;
            }
            freeBlock->next =  m_allocBlockArray[0];
            freeBlock->before = NULL;
            m_allocBlockArray[0] = freeBlock;
            return NULL;
        }
        

        //分割块
        DataBegin_t * dataBegin = freeBlock->freeData;

        DataStatus_t * dataStatus = (DataStatus_t *)((char *)dataBegin + sizeof(DataBegin_t));
        res = (char *)dataStatus + sizeof(DataStatus_t);
        DataEnd_t * dataEnd = dataBegin->end;

        dataBegin->block = freeBlock;


        dataStatus->st = 0XA1;
        freeBlock->occupyNum++;

        m_freeSize -= size;

        //剩余长度计算
        unsigned long leftLen = this->size(freeBlock->freeData) - size;

        
        if (leftLen <= sizeof(DataBegin_t)  + sizeof(DataEnd_t) + sizeof(DataStatus_t))
        {
            freeBlock->freeData = NULL;

            m_freeSize -= leftLen;

            //不足以生产一个新的数据块 添加至满块
            freeBlock->next = m_allocBlockArray[0];
            if (m_allocBlockArray[0] != NULL)
            {
                m_allocBlockArray[0]->before = freeBlock;
            }
            freeBlock->before = NULL;
            m_allocBlockArray[0] = freeBlock;
        }
        else
        {
            //足以生产一个新的可用块
            //可分配长度
            leftLen = leftLen - sizeof(DataBegin_t) - sizeof(DataEnd_t) - sizeof(DataStatus_t);

            ex = Exponent(leftLen);

            m_freeSize -= sizeof(DataBegin_t);
            m_freeSize -= sizeof(DataEnd_t);
            m_freeSize -= sizeof(DataStatus_t);
            
            //新块
            dataBegin = (DataBegin_t *)((char *)res + size + sizeof(DataEnd_t));
            dataStatus = (DataStatus_t *)((char *)dataBegin + sizeof(DataBegin_t));
            dataBegin->end = dataEnd;
            dataEnd->begin = dataBegin;
            dataStatus->st = 0xA0;

            //旧块 
            dataEnd = (DataEnd_t *)((char *)res + size);
            freeBlock->freeData->end = dataEnd;
            dataEnd->begin = freeBlock->freeData;

            //设置新块
            freeBlock->freeData = dataBegin;

            //添加至可分配块链表数组
            freeBlock->next = m_allocBlockArray[ex];
            if (m_allocBlockArray[ex] != NULL)
            {
                m_allocBlockArray[ex]->before = freeBlock;
            }
            freeBlock->before = NULL;
            m_allocBlockArray[ex] = freeBlock;

        }

        return res;
    }

    /**
     * @brief       归还内存
     *
     */
    void MemoryPool::free(void * ptr)
    {
        DataBegin_t * curDataBegin = NULL;
        DataStatus_t * curDataStatus = NULL;
        DataEnd_t * curDataEnd = NULL;

        DataBegin_t * preDataBegin = NULL;
        DataStatus_t * preDataStatus = NULL;
        DataEnd_t * preDataEnd = NULL;


        DataBegin_t * nextDataBegin = NULL;
        DataStatus_t * nextDataStatus = NULL;
        DataEnd_t * nextDataEnd = NULL;

        DataBegin_t * blockBegin = NULL;
        DataEnd_t * blockEnd = NULL;

        unsigned long freeSize = 0;
        unsigned long oldFreeSize = 0;

        short ex = 0;
        short oldEx = 0;


        Block_t * block = NULL;
        
        if (ptr == NULL)
        {
            return;
        }

        curDataStatus =(DataStatus_t *)((char *)ptr - sizeof(DataStatus_t));

        curDataBegin = (DataBegin_t *)((char *)curDataStatus - sizeof(DataBegin_t));

        curDataEnd = curDataBegin->end;

        
        //检查状态标志位
        if (curDataStatus->st != 0xA1)
        {
            //状态位异常 不处理
            return;
        }

        curDataStatus->st = 0xA0;

        if ((block = curDataBegin->block) == NULL)
        {
            return;
        }


        //计算当前块所在ex
        oldFreeSize = size(block->freeData);

        oldEx = Exponent(oldFreeSize);

        //归还块可分配大小
        freeSize = size(curDataBegin);

        blockBegin = (DataBegin_t *)((char *)block + sizeof(Block_t));
        blockEnd = (DataEnd_t *)((char *)block + block->size - sizeof(DataEnd_t));

        block->occupyNum--;

        //前合并 起始块不前合并
        if(blockBegin != curDataBegin)
        {
            preDataEnd = (DataEnd_t *)((char *)curDataBegin - sizeof(DataEnd_t));

            preDataBegin = preDataEnd->begin;

            preDataStatus = (DataStatus_t *)((char *)preDataBegin + sizeof(DataBegin_t));

            if(preDataStatus->st == 0xA0) 
            {
                //合并空闲块
                preDataBegin->end = curDataEnd;
                curDataEnd->begin = preDataBegin;
                curDataBegin = preDataBegin;
                curDataStatus = preDataStatus;

                freeSize += sizeof(DataBegin_t);
                freeSize += sizeof(DataEnd_t);
                freeSize += sizeof(DataStatus_t);
            }
        }

        //后合并 末尾块不进行后合并
        if (blockEnd != curDataEnd)
        {
            nextDataBegin =  (DataBegin_t *)((char *)curDataEnd + sizeof(DataEnd_t));

            nextDataStatus = (DataStatus_t *)((char *)nextDataBegin  + sizeof(DataBegin_t));

            nextDataEnd = nextDataBegin->end;

            if (nextDataStatus->st == 0xA0)
            {
                //合并空闲块
                curDataBegin->end = nextDataEnd;
                nextDataEnd->begin = curDataBegin;
                curDataEnd = nextDataEnd;

                freeSize += sizeof(DataBegin_t);
                freeSize += sizeof(DataEnd_t);
                freeSize += sizeof(DataStatus_t);
            }
        }

        m_freeSize += freeSize;


        //检查是否为空闲块
        if (block->occupyNum == 0)
        {
            //空闲块 则添加至空闲块中
            block->fnext = m_freeBlock;
            if (m_freeBlock != NULL)
            {
                m_freeBlock->fbefore = block;
            }
            block->fbefore = NULL;
            m_freeBlock = block;
        }

        //检查可分配空间是否变大
        freeSize = size(curDataBegin);

        ex = Exponent(freeSize);

        if (oldFreeSize < freeSize)
        {
            block->freeData = curDataBegin;
        }

        if (ex > oldEx)
        {
            //移除
            if (block->next != NULL)
            {
                block->next->before = block->before;
            }

            if (block->before != NULL)
            {
                block->before->next = block->next;
            }
            else
            {
                m_allocBlockArray[oldEx] = block->next;
            }

            block->before = NULL;
            block->next = NULL;

            //新增
            block->next = m_allocBlockArray[ex];

            if (m_allocBlockArray[ex] != NULL)
            {
                m_allocBlockArray[ex]->before = block;
            }

            m_allocBlockArray[ex] = block;

        }

        //检查空闲率
        ::gettimeofday(&m_nowTime,NULL);
        if ((m_nowTime.tv_sec < m_lastClearTime.tv_sec) || (m_nowTime.tv_sec == m_lastClearTime.tv_sec && m_nowTime.tv_usec < m_lastClearTime.tv_usec))
        {
            printf("Time Error!\n");
            ::memcpy(&m_lastClearTime,&m_nowTime, sizeof(timeval));
            return;
        }

        if ((m_nowTime.tv_sec - m_lastClearTime.tv_sec) * 1000 + (m_nowTime.tv_usec - m_lastClearTime.tv_usec) / 1000 < m_clearInterval)
        {
            return;
        }

        return;

        ::memcpy(&m_lastClearTime,&m_nowTime, sizeof(timeval));
        block = m_freeBlock;
        if (block != NULL)
        {
            while (block != NULL && m_totalSize > 0 && m_freeProportion < (m_freeSize * 100 / m_totalSize))
            {

                m_totalSize -= block->size;

                //oldFreeSize = (block->size - sizeof(Block_t) - sizeof(DataBegin_t) - sizeof(DataStatus_t) - sizeof(DataEnd_t));

                oldFreeSize = size(block->freeData);

                m_freeSize -= oldFreeSize;

                ex = Exponent(oldFreeSize);

                //可分配块中移除
                if (block->next != NULL)
                {
                    block->next->before = block->before;
                }

                if (block->before != NULL)
                {
                    block->before->next = block->next;
                }
                else
                {
                    m_allocBlockArray[ex] = block->next;
                }

                m_freeBlock = block;
                block = block->fnext;
                ::free(m_freeBlock);
            }

            m_freeBlock = block;

            if (m_freeBlock != NULL)
            {
                m_freeBlock->fbefore = NULL;
            }

        }
        
    }
    
    /**
     * @brief       查询内存大小
     *
     * @return      成功 大小
     *              失败 0
     */
    unsigned long MemoryPool::size(void * ptr)
    {
        DataBegin_t * curDataBegin = NULL;

        DataStatus_t * curDataStatus = NULL;

        if (ptr == NULL)
        {
            return 0;
        }

        curDataStatus =(DataStatus_t *)((char *)ptr - sizeof(DataStatus_t));

        curDataBegin = (DataBegin_t *)((char *)curDataStatus - sizeof(DataBegin_t));

        //检查状态标志位
        if (curDataStatus->st != 0xA1)
        {
            //状态位异常 不处理
            return 0;
        }

        //计算当前块所在ex
        return size(curDataBegin);
    }
}
