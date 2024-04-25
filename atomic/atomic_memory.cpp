#include <iostream>
#include <thread>
#include <atomic>
#include <cassert>
//自旋锁的实现
class SpinLock{
public:
    void lock()
    {
        while(flag.test_and_set(std::memory_order_acquire));//自旋等待，直到释放
    }
    void unlock(){
        flag.clear(std::memory_order_release);//释放锁
    }
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

//测试
void TestSpinLock()
{
    SpinLock spinlock;
    std::thread t1([&](){
        spinlock.lock();
        for(int i = 0; i < 3; ++i)
        {
            std::cout << "*";
        }
        std::cout<< std::endl;
        spinlock.unlock();
    });

    std::thread t2([&](){
        spinlock.lock();
        for(int i = 0; i < 3; ++i){
            std::cout<<"?";
        }
        std::cout << std::endl;
        spinlock.unlock();
    });
    t1.join();
    t2.join();
}

std::atomic<bool> x, y;
std::atomic<int> z;

void write_x_then_y() {
    x.store(true, std::memory_order_relaxed);  // 1
    y.store(true, std::memory_order_relaxed);  // 2
}

void read_y_then_x() {
    while (!y.load(std::memory_order_relaxed)) { // 3
        std::cout << "y load false" << std::endl;
    }

    if (x.load(std::memory_order_relaxed)) { //4
        ++z;
    }

}

void TestOrderRelaxed() {
    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    t1.join();
    t2.join();
    assert(z.load() != 0); // 5
}

void TestOderRelaxed2() {
    std::atomic<int> a{ 0 };
    std::vector<int> v3, v4;
        std::thread t1([&a]() {
            for (int i = 0; i < 10; i += 2) {
                a.store(i, std::memory_order_relaxed);
            }    
        });

        std::thread t2([&a]() {
            for (int i = 1; i < 10; i += 2)
                a.store(i, std::memory_order_relaxed);
            });


        std::thread t3([&v3, &a]() {
            for (int i = 0; i < 10; ++i)
                v3.push_back(a.load(std::memory_order_relaxed));
            });

        std::thread t4([&v4, &a]() {
            for (int i = 0; i < 10; ++i)
                v4.push_back(a.load(std::memory_order_relaxed));
            });

        t1.join();
        t2.join(); 
        t3.join(); 
        t4.join();

        for (int i : v3) {
            std::cout << i << " ";
        }

        std::cout << std::endl;
        for (int i : v4) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
}
int main()
{
    TestSpinLock();
    return 0;
}