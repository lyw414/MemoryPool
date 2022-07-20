#ifndef __LYW_CODE_SIMPLE_MEMORY_POOL_RE_HPP__
#define __LYW_CODE_SIMPLE_MEMORY_POOL_RE_HPP__


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Define.hpp"

namespace LYW_CODE::SimpleMemoryPool
{
    class MemoryPool;
    class Register
    {
    private:
        friend class MemoryPool;

    private:
        MemoryPool * m_memPool;

        Node_t ** m_free;

        NodePackage_t * m_revert;

    private:
        Register(MemoryPool * memPool);
        ~Register();

        void * Allocate(int ex);
        void Free (void * ptr);
    };
}
#endif
