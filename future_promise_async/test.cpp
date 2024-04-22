#include "include/ThrealPool.h"
int main()
{
    for(int i = 0; i < 100; ++i)
    {
        int m = 0;
        std::future<int> fut = ThreadPool::instance().commit([](int& m){
            m = 1024;
            std::cout<<"inner set is: "<<m <<std::endl;
            std::cout<<"m address is: "<<&m<<std::endl;
            return 34;
        },std::ref(m));
        std::cout<<fut.get()<<std::endl;
    }
    return 0;
}