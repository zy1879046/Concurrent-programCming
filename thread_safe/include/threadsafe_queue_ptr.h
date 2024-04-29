#include <mutex>
#include <condition_variable>
#include <queue>

template< typename T>
class threadsafe_queue_ptr{
private:
    std::queue<std::shared_ptr<T>> data_queue;
    std::condition_variable data_cond;
    mutable std::mutex mtx;
public:
    threadsafe_queue_ptr(){}
    threadsafe_queue_ptr(const threadsafe_queue_ptr&) = delete;
    void wait_and_pop(T& value){
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk,[this](){
            return ! data_queue.empty();
        });
        value = std::move(*(data_queue.front()));
        data_queue.pop();
    }
    bool try_pop(T& value){
        std::lock_guard<std::mutex> lk(mtx);
        if(data_queue.empty()) return false;
        value = std::move(*(data_queue.front()));
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> wait_and_pop(){
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk,[this](){
            return !data_queue.empty();
        });
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }
    std::shared_ptr<T> try_pop(){
        std::lock_guard<std::mutex> lk(mtx);
        if(data_queue.empty()) return false;
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }
    void push(T new_value){
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
        std::lock_guard<std::mutex> lk(mtx);
        data_queue.push(data);
        data_cond.notify_one();
    }
    bool empty()const{
        std::lock_guard<std::mutex> lk(mtx);
        return data_queue.empty();
    }
};