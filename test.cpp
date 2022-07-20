#include "MemoryPool.h"

int main()
{
    LYW_CODE::SimpleMemoryPool::MemoryPool pool;
    pool.Malloc(0, 222);
    return 0;
}

