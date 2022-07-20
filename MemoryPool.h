#ifndef __LYW_CODE_SIMPLE_MEMORY_POOL_HPP__
#define __LYW_CODE_SIMPLE_MEMORY_POOL_HPP__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#include "Define.hpp"
namespace LYW_CODE::SimpleMemoryPool
{
    class Register;
    class MemoryPool 
    {
    private:
        friend class Register;

        pthread_mutex_t m_lock;

        SingleList m_revert[SIM_MEM_MAX_EX];

        
    private:
        Block_t * CreateBlock(int ex);

        void FreeNode(Node_t * node);

        void Revert (NodePackage_t & package);
        
        Node_t * Apply(int ex);
        
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

    public:

        MemoryPool();

        ~MemoryPool();


        int Regist();

        void UnRegist();

        void * Malloc (int handle, unsigned long long size);

        void Free (int handle, void * ptr);
    };
}
#endif
