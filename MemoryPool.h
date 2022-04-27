#ifndef __LYW_CODE_MEMORY_POOL_H__
#define __LYW_CODE_MEMORY_POOL_H__
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <stdio.h>

/**
 * 内存池 
 *
 */
namespace LYW_CODE
{
    struct _DataBegin;
    struct _DataEnd;

    /**
     * @brief       malloc块
     */
    typedef struct _Block
    {
        unsigned long size;
        struct _DataBegin * freeData;
        unsigned long occupyNum;
        struct _Block * next;
        struct _Block * before;

        struct _Block * fnext;
        struct _Block * fbefore;

    } Block_t;
    
    /**
     * @brief       数据块头
     */
    typedef struct _DataBegin
    {
        Block_t * block;
        struct _DataEnd * end;
    } DataBegin_t;

    /**
     * @brief       数据块状态 0xA0 free 0xA1 occupy 其余非法
     */
    typedef struct _DataStatus
    {
        unsigned char st;
    } DataStatus_t;
    
    /**
     * @brief       数据块尾
     */
    typedef struct _DataEnd
    {
        DataBegin_t * begin;
    } DataEnd_t;



    class MemoryPool
    {
    private:
        //最小分配块 默认为4096
        unsigned long m_minBlockSize;
        
        //最大指数 默认为 32
        short m_maxExponent;
        
        //最大空间 0 不限制
        unsigned long m_maxSize;

        //总空间
        unsigned long m_totalSize;
        
        //空闲空间
        unsigned long m_freeSize;
        
        //空闲占比 0 ~ 100 默认为 20%
        unsigned short m_freeProportion;

        //可用于分配的块链
        Block_t ** m_allocBlockArray;

        //空闲块
        Block_t * m_freeBlock;

        //清理间隔 毫秒
        time_t m_clearInterval;

        //清理时间
        struct timeval m_lastClearTime;

        struct timeval m_nowTime;

    private:
        unsigned long size(DataBegin_t * data);

        /**
         * @brief   长度指数计算
         *
         * @return      成功 >= 0
         *              失败 <  0
         */
        inline short Exponent(unsigned long number);

        
        /**
         * @brief   申请一个新块(不添加至任何链表中)
         *          若申请大小不足 m_minBlockSize 则申请m_minBlockSize 否则申请 size (用户可使用空间)
         *
         * return       成功  Block_t *
         *              失败  NULL
         */
        Block_t * newBlock(unsigned long size);


    public:
        MemoryPool(unsigned long maxSize = 0, unsigned short freeProportion = 20, unsigned long minBlockSize = 4096);

        ~MemoryPool();


        /**
         * @brief       分配一块内存 使用过程严禁越界
         *
         * @return      成功 可用内存地址
         *              失败 NULL 
         */
        void * malloc(unsigned long size);
        
        /**
         * @brief       归还内存
         *
         */
        void free(void * ptr);
        
        /**
         * @brief       查询内存大小
         *
         * @return      成功 大小
         *              失败 0
         */
        unsigned long size(void * ptr);
    };
}
#endif
