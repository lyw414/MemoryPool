#include "MemoryPool.h"
#include "Register.h"
namespace LYW_CODE::SimpleMemoryPool
{

    LYW_CODE::SimpleMemoryPool::MemoryPool::MemoryPool()
    {
        ::pthread_mutex_init(&m_lock, NULL);
    }

    LYW_CODE::SimpleMemoryPool::MemoryPool::~MemoryPool()
    {

    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::FreeNode(Node_t * node)
    {
        return;
    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::Revert (NodePackage_t & package)
    {
        ::pthread_mutex_lock(&m_lock);
        m_revert[package.ex].Push(package.begin);
        ::pthread_mutex_unlock(&m_lock);
        return;
    }

    inline int Exponent(unsigned long long number)
    {
        int ex = 0;
        unsigned long long size = 1;
        if (number == 0)
        {
            return ex;
        }

        do
        {
            size = size << 1;
            ex++;
        } while (size < number);

        return ex;
    }



    Block_t * LYW_CODE::SimpleMemoryPool::MemoryPool::CreateBlock(int ex)
    {
        return NULL;
    }
    
    Node_t * LYW_CODE::SimpleMemoryPool::MemoryPool::Apply(int ex)
    {
        
        Node_t * pNode = NULL;
        Block_t * pBlock = NULL;
        ::pthread_mutex_lock(&m_lock);
        pNode = (Node_t *)m_revert[ex].Pop();
        ::pthread_mutex_unlock(&m_lock);

        if (pNode == NULL)
        {
            pBlock = CreateBlock(ex);
            pNode = (Node_t *)pBlock->data;
        }

        return pNode;
    }

    int LYW_CODE::SimpleMemoryPool::MemoryPool::Regist()
    {
        return 0;
    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::UnRegist()
    {

    }

    void * LYW_CODE::SimpleMemoryPool::MemoryPool::Malloc (int handle, unsigned long long size)
    {
        //LYW_CODE::SimpleMemoryPool::Register * x = new LYW_CODE::SimpleMemoryPool::Register(this);

        return NULL;
    }

    void LYW_CODE::SimpleMemoryPool::MemoryPool::Free (int handle, void * ptr)
    {

    }
}
