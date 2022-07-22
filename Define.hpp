#ifndef __LYW_CODE_SIMPLE_MEMORY_POOL_DEFINE_HPP__
#define __LYW_CODE_SIMPLE_MEMORY_POOL_DEFINE_HPP__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SIM_MEM_MAX_EX 32
#define SIM_MEM_MIN_SIZE 8
namespace LYW_CODE::SimpleMemoryPool
{

#pragma pack(1)
    typedef enum _BlockST {
        BLOCK_USE = 0,
        BLOCK_FREE
    } BlockST_e;

    //申请大块
    typedef struct _Block {
        struct _Block * next;
        struct _Block * pre;
        BlockST_e st;
        int ex;
        int occupyCount;
        unsigned char data[0];
    } Block_t;
    
    //用于分配的节点
    typedef struct _Node {
        Block_t * block;
        struct _Node * next;
        unsigned char data[0];
    } Node_t;
    
    //节点包 减少归还次数
    typedef struct _NodePackage {
        Node_t * begin;
        Node_t * end;
        int count;
        int ex;
    } NodePackage_t;

    //配置 -- 控制每个幂下的分配策略
    typedef struct _CFG {
        //归还块上限 - 只读
        int revertCount;
        //申请块数量 - 只读
        int nodeCount;
        //数据块大小 - 只读
        unsigned long long dataSize;

        unsigned long long nodeSize;
        //分配大小 - 只读
        unsigned long long allocateSize; 
        
        int interval;

        int totalFreeNodeCount;

        int totalNodeCount;

        int keepPercent;

    } CFG_t;
#pragma pack()

    class SingleList 
    {
    private:
        typedef struct _Node {
            struct _Node * next;
            int count;
            void * data;
        } Node_t;

    private:
        Node_t * m_begin;
        Node_t * m_end;
        Node_t * m_free;
        
        int m_totalCount;
        int m_occupyCount;

    public:
        SingleList()
        {
            m_totalCount = 0;
            m_occupyCount = 0;

            m_free = NULL;
            m_begin = NULL;
            m_end = NULL;

        }

        void Push(void * ptr, int count)
        {
            Node_t * pNode = NULL;
            m_occupyCount++;
            if (m_free == NULL)
            {
                //申请一个节点
                pNode = (Node_t *)::malloc(sizeof(Node_t));
                m_totalCount++;
            }
            else
            {
                //空闲节点弹出
                pNode = m_free;
                m_free = m_free->next;
            }
            
            //赋值
            pNode->data = ptr;
            pNode->count = count;
            pNode->next = NULL;
            

            //添加至占有节点
            if (m_end == NULL)
            {
                m_end = m_begin = pNode;
            }
            else
            {
                m_end->next = pNode;
                m_end = pNode;
            }
        }

        void * Pop(int & count)
        {
            void * res = NULL;
            Node_t * pNode = NULL;
            count = 0;

            if (m_begin != NULL)
            {
                m_occupyCount--;
                count = m_begin->count;
                res = m_begin->data;
                pNode = m_begin;
                m_begin = m_begin->next;
                if (m_begin == NULL)
                {
                    m_end = NULL;
                }

                if (m_totalCount > m_occupyCount + 32)
                {
                    ::free(pNode);
                    m_totalCount--;
                }
                else
                {
                    pNode->next = m_free;
                    m_free = pNode;
                }
            }

            return res;
        }

        void * Front(int & count)
        {
            if (m_begin == NULL)
            {
                count = 0;
                return NULL;
            }
            else
            {
                count = m_begin->count;
                return m_begin->data;
            }

        }
    };

}
#endif
 
