#include "Register.h"
#include "MemoryPool.h"
namespace LYW_CODE::SimpleMemoryPool
{
    Register::Register(MemoryPool * memPool)
    {
        m_memPool = memPool;
        m_free = (Node_t **)::malloc(sizeof(Node_t *) * SIM_MEM_MAX_EX);
        ::memset(m_free, 0x00, sizeof(Node_t *) * SIM_MEM_MAX_EX);

        m_revert = (NodePackage_t *)::malloc(sizeof(NodePackage_t) * SIM_MEM_MAX_EX);

        ::memset(m_revert, 0x00, sizeof(NodePackage_t) * SIM_MEM_MAX_EX);

        for(int iLoop = 0; iLoop < SIM_MEM_MAX_EX; iLoop++)
        {
            m_revert[iLoop].ex = iLoop;
        }
    }

    Register::~Register()
    {
        if (m_free != NULL)
        {
            ::free(m_free);
            m_free = NULL;
        }


        if (m_revert != NULL)
        {
            ::free(m_revert);
            m_revert = NULL;
        }
    }


    void * Register::Allocate(int ex)
    {
        void * res = NULL;

        if (ex >= SIM_MEM_MAX_EX)
        {
            return NULL;
        }

        if (m_free[ex] == NULL)
        {
            if (m_revert[ex].begin != NULL)
            {
                m_free[ex] = m_revert[ex].begin;
                m_revert[ex].begin = m_revert[ex].end = NULL;
                m_revert[ex].count = 0;
            }
            else
            {
                if ((m_free[ex] = m_memPool->Apply(ex)) == NULL)
                {
                    return NULL;
                }
            }
        }

        res = m_free[ex]->data;

        m_free[ex] = m_free[ex]->next;

        return res;
    }

    void Register::Free (void * ptr)       
    {
        Node_t * pNode = NULL;

        int ex = 0;

        if (ptr == NULL)
        {
            return;
        }

        pNode = (Node_t *)((unsigned char *)ptr - sizeof(Node_t));

        ex = pNode->block->ex;

        if (ex >= SIM_MEM_MAX_EX)
        {
            return;
        }

        if (pNode->block->st == BLOCK_FREE)            
        {
            m_memPool->FreeNode(pNode);
        }
        else
        {
            m_revert[ex].count++;
            if (m_revert[ex].begin == NULL)
            {
                pNode->next = NULL;
                m_revert[ex].end = m_revert[ex].begin = pNode;
            }
            else
            {
                pNode->next = m_revert[ex].begin;
                m_revert[ex].begin = pNode;
            }

            if (m_revert[ex].count >= 1024)
            {
                m_memPool->Revert(m_revert[ex]);

                m_revert[ex].begin = m_revert[ex].end = NULL;
                m_revert[ex].count = 0;
            }
        }
    }
}
