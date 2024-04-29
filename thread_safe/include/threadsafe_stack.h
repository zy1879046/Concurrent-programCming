#include <exception>
#include <mutex>
#include <stack>
#include <condition_variable>

struct  empty_stack : std::exception
{
    const char* what() const throw();
};

template <typename T>
class threadsafe_stack
{
private:
    /* data */
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack(/* args */){};
    threadsafe_stack(const threadsafe_stack& other){
        std::lock_guard<std::mutex> lk(other.m);
        data = other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;
    ~threadsafe_stack(){};

    void push(T new_value){
        std::lock_guard<std::mutex> lk(m);
        data.push(std::move(new_value));
    }
    std::shared_ptr<T> pop(){
        std::lock_guard<std::mutex> lk(m);
        if(data.empty()) throw empty_stack();
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top())));
        data.pop();
        return res;
    }
    void pop(T& value){
        std::lock_guard<std::mutex> lk(m);
        if(data.empty()) throw empty_stack();
        value = std::move(data.top());
        data.pop();
    }
    bool empty(){
        std::lock_guard<std::mutex> lk(m);
        return data.empty();
    }
};


