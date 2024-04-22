
#include<iostream>
#include<condition_variable>
#include<mutex>
#include<thread>
#include<queue>

std::mutex mtx_num;
int num = 1;
//不好的实现
void PoorImpleman(){
    std::thread t1([](){
            for(;;){
            {
                std::lock_guard<std::mutex> lock(mtx_num);
                if(num == 1){
                    std::cout<< "thread A print 1..."<<std::endl;
                    ++num;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            });
    std::thread t2([](){
            for(;;){
            {
            std::lock_guard<std::mutex> lock(mtx_num);
            if(num == 2)
            {
                std::cout<<"thread B printf 2..."<<std::endl;
                --num;
                continue;
            }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            });
    t1.join();
    t2.join();
}
std::condition_variable cvA;
std::condition_variable cvB;
//使用条件变量实现
void ResonableImplemention()
{
    std::thread t1([](){
        for(;;)
        {
            std::unique_lock<std::mutex> lock(mtx_num);
            cvA.wait(lock,[](){return num == 1;});
            ++num;
            std::cout<<"thread A print 1.."<<std::endl;
            cvB.notify_one();
        }
    });
    std::thread t2([](){
        for(;;){
            std::unique_lock<std::mutex> lock(mtx_num);
            cvB.wait(lock,[](){return num == 2;});
            --num;
            std::cout<<"thread B print 2..."<<std::endl;
            cvA.notify_one();
        }
    });
    t1.join();
    t2.join();
}

// 线程安全队列
template <typename T>
class threadsafe_queue{
private:
    mutable std::mutex mtx;
    std::condition_variable data_cond;
    std::queue<T> data_queue;
public:
    threadsafe_queue() = default;
    threadsafe_queue(threadsafe_queue const& other){
        std::lock_guard<std::mutex> lk(mtx);
        data_queue = other.data_queue;
    }
    //threadsafe_queue& operator=(threadsafe_queue const&) = delete;
    
    void push(T new_value){
        std::lock_guard<std::mutex> lk(mtx);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();
    }
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk,[this](){return !data_queue.empty();});
        value = data_queue.front();
        data_queue.pop();
    }
    std::shared_ptr<T> wait_and_pop(){
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk,[this](){return !data_queue.empty();});
        std::shared_ptr<T> res = std::make_shared<T>(data_queue.front());
        return res;
    }
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mtx);
        if(data_queue.empty()) return false;
        value = data_queue.front();
        return true;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mtx);
        if(data_queue.empty())return nullptr;
        std::shared_ptr<T> res = std::make_shared<T>(data_queue.front());
        return res;
    }

};

void test_safe_que() {
    threadsafe_queue<int>  safe_que;
    std::mutex  mtx_print;
     std::thread producer(
        [&]() {
            for (int i = 0; ;i++) {
                safe_que.push(i);
                {
                    std::lock_guard<std::mutex> printlk(mtx_print);
                    std::cout << "producer push data is " << i << std::endl;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    );

    std::thread consumer1(
        [&]() {
            for (;;) {
                auto data = safe_que.wait_and_pop();
                {
                    std::lock_guard<std::mutex> printlk(mtx_print);
                    std::cout << "consumer1 wait and pop data is " << *data << std::endl;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }    
    );

    std::thread consumer2(
        [&]() {
            for (;;) {
                auto data = safe_que.try_pop();
                if (data != nullptr) {
                    {
                        std::lock_guard<std::mutex> printlk(mtx_print);
                        std::cout << "consumer2 try_pop data is " << *data << std::endl;
                    }

                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    );

    producer.join();
    consumer1.join();
    consumer2.join();
}

int main(int argc,char* argv[])
{
    // PoorImpleman();
    //ResonableImplemention();
    test_safe_que();
    return 0;
}
