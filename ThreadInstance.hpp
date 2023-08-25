#ifndef __LYW_CODE_THREAD_INSTANCE_HPP_FILE__
#define __LYW_CODE_THREAD_INSTANCE_HPP_FILE__

#include <pthread.h>
namespace LYW_CODE
{
    template <typename T>
    class ThreadInstance
    {
    public:
        static __thread T * m_self;
        static T * GetInstance()
        {
            if (m_self == NULL)    
            {
                m_self = new T();
            }
            return m_self;
        }
    
    };
    
    template <typename T>
    __thread T * ThreadInstance<T>::m_self = NULL;
}
#endif
