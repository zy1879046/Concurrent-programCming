#include <iostream>
#include <thread>
#include <memory>
#include <future>
class TestCopy {
public:
    TestCopy(){}
    TestCopy(const TestCopy& tp) {
        std::cout << "Test Copy Copy " << std::endl;
    }
    TestCopy(TestCopy&& cp) {
        std::cout << "Test Copy Move " << std::endl;
    }
    TestCopy& operator=(const TestCopy& tp){
        std::cout<< "Test Copy ="<<std::endl;
        return *this;
    }
    TestCopy& operator=(TestCopy&& tp){
        std::cout<< "Test Copy ="<<std::endl;
        return *this;
    }

};

TestCopy TestCp() {
    TestCopy tp = TestCopy();
    return tp;
}

std::unique_ptr<int> ReturnUniquePtr() {
    std::unique_ptr<int>  uq_ptr = std::make_unique<int>(100);
    return  uq_ptr;
}

std::thread ReturnThread() {
    std::thread t([]() {
        int i = 0;
        while (true) {
            std::cout << "i is " << i << std::endl;
            i++;
            if (i == 5) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        });

    return t;
}

void BlockAsync() {
    std::cout << "begin block async" << std::endl;
    {
        std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cout << "std::async called " << std::endl;
            });
            /*因为async返回一个右值类型的future，无论左值还是右值，future都要被析构，
            因为其处于一个局部作用域{}中。 当编译器执行到}时会触发future析构。但是
            future析构要保证其关联的任务完成，所以需要等待任务完成future才被析构， 
            所以也就成了串行的效果了*/
            //即该返回的future的析构会等待异步线程的完成
    }
    std::cout << "end block async" << std::endl;
}
//纯异步实现
template<typename Func, typename... Args  >
auto  ParallenExe(Func&& func, Args && ... args) -> std::future<decltype(func(args...))> {
    typedef    decltype(func(args...))  RetType;
    std::function<RetType()>  bind_func = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    std::packaged_task<RetType()> task(bind_func);
    auto rt_future = task.get_future();
    std::thread t(std::move(task));

    t.detach();

    return rt_future;
}
int main()
{
    // TestCopy tp1;
    // TestCopy tp2(std::move(tp1));
    // TestCopy tp3 = tp2;
    // TestCopy tp4 = std::move(tp3);
    // TestCopy tp = TestCp();
    // auto rt_ptr = ReturnUniquePtr();
    // std::cout << "rt_ptr value is " << *rt_ptr << std::endl;
    // std::thread rt_thread = ReturnThread();
    // rt_thread.join();
    // std::thread t;
    // std::cout<< t.get_id()<<std::endl;//未赋值的线程id是0 若给以有线程控制权的 t 仅行线程移动赋值，会通过检查当前线程id是否为0 若为0则触发terminate
    // return 0;
    BlockAsync();
    return 0;
}