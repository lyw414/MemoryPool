#include "MemoryPool.h"
#include "Register.h"
namespace LYW_CODE::SimpleMemoryPool
{

    LYW_CODE::SimpleMemoryPool::MemoryPool::MemoryPool()
    {
        ::pthread_mutex_init(&m_lock, NULL);

        m_registerSize = 16;

        int dec = 1;

        m_register = (Register **)::malloc(sizeof(Register *) * m_registerSize);
        memset(m_register, 0x00, sizeof(Register *) * m_registerSize);
        for (int iLoop = 0; iLoop < SIM_MEM_MAX_EX; iLoop++)
        {
            unsigned long long size = 1;

            size = size << iLoop;

            size *= 8;

            m_cfg[iLoop].dataSize = size;

            m_cfg[iLoop].nodeSize = size + sizeof(Node_t);

            m_cfg[iLoop].interval = 2;

            m_cfg[iLoop].totalFreeNodeCount = 0;

            m_cfg[iLoop].totalNodeCount = 0;

            m_cfg[iLoop].keepPercent = 30;

            if (iLoop <= 10)
            {
                m_cfg[iLoop].revertCount = 4096/ dec;
                if (m_cfg[iLoop].revertCount < 128)
                {
                    m_cfg[iLoop].revertCount = 128;
                }

                m_cfg[iLoop].nodeCount = m_cfg[iLoop].revertCount;

            }
            else
            {
                m_cfg[iLoop].revertCount = 2;
                m_cfg[iLoop].nodeCount = 2;
            }

            m_cfg[iLoop].allocateSize = sizeof(Block_t) + m_cfg[iLoop].nodeCount * m_cfg[iLoop].nodeSize;

            dec *= 2;
        }
    }

    LYW_CODE::SimpleMemoryPool::MemoryPool::~MemoryPool()
    {

    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::FreeNode(Node_t * node)
    {
        ::pthread_mutex_lock(&m_lock);
        node->block->occupyCount--;
        if (node->block->occupyCount <= 0)
        {
            m_cfg[node->block->ex].totalFreeNodeCount -= m_cfg[node->block->ex].allocateSize;
            //printf("%d Read Do Free\n", node->block->ex);
            ::free(node->block);
        }
        ::pthread_mutex_unlock(&m_lock);
        return;
    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::Revert (NodePackage_t & package)
    {
        int percent = 0;

        m_cfg[package.ex].totalFreeNodeCount += package.count;

        percent = (m_cfg[package.ex].totalFreeNodeCount * 100) / m_cfg[package.ex].totalNodeCount;


        ::pthread_mutex_lock(&m_lock);
        if (percent > m_cfg[package.ex].keepPercent && m_cfg[package.ex].totalFreeNodeCount > m_cfg[package.ex].nodeCount)
        {
            //printf("%d Do Free %d %d %d %d\n",package.ex, percent, m_cfg[package.ex].totalFreeNodeCount, m_cfg[package.ex].totalFreeNodeCount, m_cfg[package.ex].nodeCount);
            package.begin->block->st = BLOCK_FREE;
            m_cfg[package.ex].totalNodeCount -= m_cfg[package.ex].allocateSize;
        }
        m_revert[package.ex].Push(package.begin, package.count);
        ::pthread_mutex_unlock(&m_lock);


        return;
    }

    inline int LYW_CODE::SimpleMemoryPool::MemoryPool::Exponent(unsigned long long number)
    {
        int ex = 0;

        if (number == 0)
        {
            return ex;
        }

        number = (number - 1) / SIM_MEM_MIN_SIZE;

        while(number > 0)
        {
            number = number >> 1;
            ex++;
        } 

        return ex;
    }

    Block_t * LYW_CODE::SimpleMemoryPool::MemoryPool::CreateBlock(int ex)
    {
        //static int count;
        //count++;
        //printf("%d::%d\n", ex, count);

        Block_t * pBlock = (Block_t *)::malloc(m_cfg[ex].allocateSize);

        Node_t * pNode = NULL;

        memset(pBlock, 0x00, m_cfg[ex].allocateSize);

        pBlock->ex = ex;

        pBlock->st = BLOCK_USE;

        pBlock->occupyCount = m_cfg[ex].nodeCount;
        
        for (int iLoop = 0; iLoop < m_cfg[ex].nodeCount; iLoop++)
        {
            pNode = (Node_t *)((unsigned char *)pBlock + sizeof(Block_t) + iLoop * m_cfg[ex].nodeSize);
            pNode->block = pBlock;
            pNode->next = (Node_t *)((unsigned char *)pBlock + sizeof(Block_t) + (iLoop + 1) * m_cfg[ex].nodeSize);

        }

        pNode->next = NULL;

        return pBlock;
    }
    
    Node_t * LYW_CODE::SimpleMemoryPool::MemoryPool::Apply(int ex)
    {
        int count = 0;
        Node_t * pNode = NULL;
        Block_t * pBlock = NULL;
        ::pthread_mutex_lock(&m_lock);
        pNode = (Node_t *)m_revert[ex].Pop(count);
        m_cfg[ex].totalFreeNodeCount -= count;

        if (pNode == NULL)
        {

            ::pthread_mutex_unlock(&m_lock);
            pBlock = CreateBlock(ex);
            ::pthread_mutex_lock(&m_lock);
            m_cfg[ex].totalNodeCount += m_cfg[ex].nodeCount;

            pNode = (Node_t *)pBlock->data;
        }

        ::pthread_mutex_unlock(&m_lock);

        return pNode;
    }

    int LYW_CODE::SimpleMemoryPool::MemoryPool::Regist()
    {
        int handle = 0;
        ::pthread_mutex_lock(&m_lock);
        for (int iLoop = 0; iLoop < m_registerSize; iLoop++)
        {
            if (m_register[iLoop] == NULL)
            {
                m_register[iLoop] = new Register(this);
                ::pthread_mutex_unlock(&m_lock);
                return iLoop;
            }
        }

        //扩展注册数组
        Register ** newArray = (Register **)::malloc(sizeof(Register *) * (m_registerSize + 16));
        
        ::memset(newArray, 0x00, sizeof(Register *) * (m_registerSize + 16));

        ::memcpy(newArray, m_register, sizeof(Register *) * (m_registerSize));
        
        ::free(m_register);

        newArray[m_registerSize] = new Register(this);

        handle = m_registerSize;

        m_registerSize += 16;

        m_register = newArray;

        ::pthread_mutex_unlock(&m_lock);

        return handle;
    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::UnRegist(int handle)
    {
        int tag = 1;
        ::pthread_mutex_lock(&m_lock);
        if (handle < m_registerSize && handle >= 0)
        {
            if (m_register[handle] != NULL)
            {
                delete m_register[handle];
                m_register[handle] = NULL;
            }
        }
        
        if (m_registerSize > 16)
        {
            for (int iLoop = m_registerSize - 16; iLoop < m_registerSize; iLoop++)
            {
                if (m_register[iLoop] != NULL)
                {
                    tag = 0;
                    break;
                }
            }

            if (tag == 1)
            {
                m_registerSize -= 16;

                Register ** newArray = (Register **)::malloc(sizeof(Register *) * m_registerSize);

                ::memcpy(newArray, m_register, sizeof(Register *) * m_registerSize);

                ::free(m_register);

                m_register = newArray;
            }
        }

        ::pthread_mutex_unlock(&m_lock);
    }

    void * LYW_CODE::SimpleMemoryPool::MemoryPool::Malloc (int handle, unsigned long long size)
    {
        int ex = 0;

        if (handle > m_registerSize || handle < 0 || m_register[handle] == NULL )
        {
            return NULL;
        }

        ex = Exponent(size);

        return m_register[handle]->Allocate(ex);
    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::Free (int handle, void * ptr)
    {
        if (handle > m_registerSize || handle < 0 || m_register[handle] == NULL )
        {
            return;
        }

        return m_register[handle]->Free(ptr);

    }
}
