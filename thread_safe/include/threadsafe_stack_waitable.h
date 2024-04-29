#include <mutex>
#include <stack>
#include <condition_variable>

template<typename T>
class thread_stack_waitable{
private:    
    std::stack<T> data;
    mutable std::mutex m;
    std::condition_variable cv;
public:
    thread_stack_waitable(){}
    thread_stack_waitable(const thread_stack_waitable& other){
        std::lock_guard<std::mutex> lk(other.m);
        data = other.data;
    }
    thread_stack_waitable& operator=(const thread_stack_waitable&) = delete;
    
    void push(T new_value){
        std::lock_guard<std::mutex> lk(m);
        data.push(std::move(new_value));
        cv.notify_one();
    }

    std::shared_ptr<T> wait_and_pop(){
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk,[this](){
            return !data.empty();
        });
        std::shared_ptr<T> const res(std::move(data.top()));
        data.pop();
        return res;
    }

    void wait_and_pop(T& value){
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk,[this](){
            return ! data.empty();
        });
        value = std::move(data.top());
        data.pop();
    }

    bool empty(){
        std::lock_guard<std::mutex> lk(m);
        return data.empty();
    }

    bool try_pop(T& value){
        std::lock_guard<std::mutex> lk(m);
        if(data.empty()){
            return false;
        }
        value = std::move(data.top());
        data.pop();
        return true;
    }
    
    std::shared_ptr<T> try_pop(){
        std::lock_guard<std::mutex> lk(m);
        if(data.empty()){
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top())));
        data.pop();
        return res;
    }
};