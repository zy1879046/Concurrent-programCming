#include <iostream>
#include <thread>
#include <atomic>
#include <cassert>
#include <mutex>
#include <memory>
//  memory_order_seq_cst代表全局一致性顺序，可以用于 store, load 和 read-modify-write 操作, 
// 实现 sequencial consistent 的顺序模型. 在这个模型下, 所有线程看到的所有操作都有一个一致的顺序, 
//即使这些操作可能针对不同的变量, 运行在不同的线程.
// 简单来说就是当当前线程对原子操作采用了该模式，该线程对原子类型的修改会直接直接写入内存并对其他线程可见，并并通知写失效。然后其他线程读到的都是最新的值
std::atomic<bool> x,y;
std::atomic<int> z;

void write_x_then_y()
{
    x.store(true,std::memory_order_seq_cst);
    y.store(true,std::memory_order_seq_cst);
}

void read_y_then_x(){
    while(!y.load(std::memory_order_seq_cst)){
        std::cout<< "y load false" <<std::endl;
    }
    if(x.load(std::memory_order_seq_cst)){
        ++z;
    }
}

void TestOrderSeqCst(){
    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    t1.join();
    t2.join();
    assert(z.load() != 0);
}

/*memory_order_relaxed 可以用于 store, load 和 read-modify-write 操作
, 实现 relaxed 的顺序模型. 前文我们介绍过这种模型下, 只能保证操作的原子性和修改
顺序 (modification order) 一致性, 无法实现 synchronizes-with 的关系。*/
//简单来说，当使用该内存顺序时，并不保证修改后的原子类型立即写会内存，可能只在cache中保存，一个线程修改后，其他线程访问时，可能访问的是旧值

void TestOrderRelaxed() {
    std::atomic<bool> rx, ry;

    std::thread t1([&]() {
        rx.store(true, std::memory_order_relaxed); // 1
        ry.store(true, std::memory_order_relaxed); // 2
        });


    std::thread t2([&]() {
        while (!ry.load(std::memory_order_relaxed)); //3
        assert(rx.load(std::memory_order_relaxed)); //4
        });

    t1.join();
    t2.join();
}

/*在 acquire-release 模型中, 会使用 memory_order_acquire, memory_order_release 和 memory_order_acq_rel 这三种内存顺序. 它们的用法具体是这样的:

对原子变量的 load 可以使用 memory_order_acquire 内存顺序. 这称为 acquire 操作.

对原子变量的 store 可以使用 memory_order_release 内存顺序. 这称为 release 操作.

read-modify-write 操作即读 (load) 又写 (store), 它可以使用 memory_order_acquire, memory_order_release 和 memory_order_acq_rel:

如果使用 memory_order_acquire, 则作为 acquire 操作;
如果使用 memory_order_release, 则作为 release 操作;
如果使用 memory_order_acq_rel, 则同时为两者.
Acquire-release 可以实现 synchronizes-with 的关系. 如果一个 acquire 操作在同一个原子变量上读取到了一个 release 操作写入的值, 则这个 release 操作 “synchronizes-with” 这个 acquire 操作.

我们可以通过Acquire-release 修正 TestOrderRelaxed函数以达到同步的效果*/

// 当使用 release 时 在release之前的原子操作也会被写回内存，
//即一个线程有release的原子操作，那么该release之前的原子类型也被写会，保证另一个线程 acquire 的原子操作及之后的操作能读取已修改的值，达到同步效果

void TestReleaseAcquire(){
    std::atomic<bool> rx,ry;

    std::thread t1([&](){
        rx.store(true,std::memory_order_relaxed);
        ry.store(true,std::memory_order_relaxed);
    });
    std::thread t2([&](){
        while(!ry.load(std::memory_order_acquire));
        assert(rx.load(std::memory_order_acquire));
    });
    t1.join();
    t2.join();
}

void ReleasAcquireDanger2() {
    std::atomic<int> xd{0}, yd{ 0 };
    std::atomic<int> zd;

    std::thread t1([&]() {
        xd.store(1, std::memory_order_release);  // (1)
        yd.store(1, std::memory_order_release); //  (2)
        });

    std::thread t2([&]() {
        yd.store(2, std::memory_order_release);  // (3)
        });


    std::thread t3([&]() {
        while (!yd.load(std::memory_order_acquire)); //（4）
        assert(xd.load(std::memory_order_acquire) == 1); // (5)
        });

    t1.join();
    t2.join();
    t3.join();
}

void ReleaseSequence() {
    std::vector<int> data;
    std::atomic<int> flag{ 0 };

    std::thread t1([&]() {
        data.push_back(42);  //(1)
        flag.store(1, std::memory_order_release); //(2)
        });

    std::thread t2([&]() {
        int expected = 1;
        while (!flag.compare_exchange_strong(expected, 2, std::memory_order_relaxed)) // (3)
            expected = 1;
        });

    std::thread t3([&]() {
        while (flag.load(std::memory_order_acquire) < 2); // (4)
        assert(data.at(0) == 42); // (5)
        });

    t1.join();
    t2.join();
    t3.join();
}
/*memory_order_consume 其实是 acquire-release 模型的一部分, 但是它比较特殊, 
它涉及到数据间相互依赖的关系. 就是前文我们提及的 carries dependency和 dependency-ordered before.*/
void ConsumeDependency() {
    std::atomic<std::string*> ptr;
    int data;

    std::thread t1([&]() {
        std::string* p = new std::string("Hello World"); // (1)
        data = 42; // (2)
        ptr.store(p, std::memory_order_release); // (3)
        });

    std::thread t2([&]() {
        std::string* p2;
        while (!(p2 = ptr.load(std::memory_order_consume))); // (4)
        assert(*p2 == "Hello World"); // (5)
        assert(data == 42); // (6)
        });

    t1.join();
    t2.join();
}

//单例模式改良
class SingleAuto{
private:
    SingleAuto(){}
    SingleAuto(const SingleAuto&) = delete;
    SingleAuto& operator=(const SingleAuto&) = delete;
public:
    ~SingleAuto(){
        std::cout<< "single auto delete success" <<std::endl;
    }
    static std::shared_ptr<SingleAuto> GetInst(){
        if(single == nullptr)
        {
            return single;
        }
        s_mutex.lock();
        if(single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        single = std::shared_ptr<SingleAuto>(new SingleAuto);//因为 new 的问题，可能因为编译器的问题，先分配内存，在返回地址，最后构造，可能会有危险
        s_mutex.unlock();
        return single;
    }
private:
    static std::shared_ptr<SingleAuto> single;
    static std::mutex s_mutex;
};

std::shared_ptr<SingleAuto> SingleAuto::single = nullptr;
std::mutex SingleAuto::s_mutex;

void TestSingle() {
    std::thread t1([]() {
        std::cout << "thread t1 singleton address is 0X: " << SingleAuto::GetInst() << std::endl;
        });

    std::thread t2([]() {
        std::cout << "thread t2 singleton address is 0X: " << SingleAuto::GetInst() << std::endl;
        });

    t2.join();
    t1.join();
}

//使用内存模型来解决这个问题
class SingleMemoryModel{
private:
    SingleMemoryModel(){};
    SingleMemoryModel(const SingleMemoryModel&) = delete;
    SingleMemoryModel& operator=(const SingleMemoryModel&) = delete;
public:
    ~SingleMemoryModel(){
        std::cout << "single auto delete success" << std::endl;
    }
    static std::shared_ptr<SingleMemoryModel> GetInst(){
        if(_b_init.load(std::memory_order_acquire)){
            return single;
        }
        s_mutex.lock();
        if(_b_init.load(std::memory_order_relaxed))
        {
            s_mutex.unlock();
            return single;
        }
        single = std::shared_ptr<SingleMemoryModel>(new SingleMemoryModel());
        _b_init.store(true,std::memory_order_release);
        s_mutex.unlock();
        return single;
    }
private:
    static std::shared_ptr<SingleMemoryModel> single;
    static std::mutex s_mutex;
    static std::atomic<bool> _b_init;
};
std::shared_ptr<SingleMemoryModel> SingleMemoryModel::single = nullptr;
std::mutex SingleMemoryModel::s_mutex;
std::atomic<bool> SingleMemoryModel::_b_init(false);

void TestSingleMemory() {
    std::thread t1([]() {
        std::cout << "thread t1 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl;
        });

    std::thread t2([]() {
        std::cout << "thread t2 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl;
        });

    t2.join();
    t1.join();
}
int main()
{
    // TestOrderSeqCst();
    // TestSingleMemory();
    // std::make_shared()
    return 0;
}