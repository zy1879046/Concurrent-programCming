#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <atomic>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool{
public:
    using Task = std::packaged_task<void()>;
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    static ThreadPool& instance(){
        static ThreadPool ins;
        return ins;
    }
    ~ThreadPool(){
        stop();
    }
    //给外面的用户使用的，通过该函数将线程任务封装成参数为void的形式，然后加入到任务队列，最后形式与packaged_task使用一样，返回一个future的对象
    template<class F,class... Args>
    auto commit(F&& f,Args&&... args) -> std::future<decltype(f(args...))>{
        using RetType = decltype(f(args...));
        if(stop_.load())
            return std::future<RetType>();
        //将串过来的函数绑定为一个 函数列表为void的函数，然后在用来初始化 make_shared 中的packaged_task<RetType()
        auto task = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f),std::forward<Args>(args)...));
        std::future<RetType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> cv_mt(cv_mt_);
            tasks_.emplace([task]{(*task)();});
        }
        cv_lock_.notify_one();
        return ret;
    }
    int idleThreadCount(){
        return thread_num_;
    }
private:
    ThreadPool(unsigned int num = 5):stop_(false)
    {
        {
            if(num < 1)
                thread_num_ = 1;
            else
                thread_num_ = num;
        }
        start();
    }

    void start()
    {
        for(int i = 0; i < thread_num_; ++i)//将线程全部启动 
        {
            pool_.emplace_back([this](){
                while(!this->stop_.load()){
                    Task task;
                    {
                        std::unique_lock<std::mutex> cv_mt(cv_mt_);
                        this->cv_lock_.wait(cv_mt,[this]{return this->stop_.load() || !this->tasks_.empty();});
                        if(this->tasks_.empty())return;
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    this->thread_num_--;//主要用来对函数idleThreadCount起作用
                    task();
                    this->thread_num_++;
                }
            });
        }
    }
    void stop(){
        stop_.store(true);
        cv_lock_.notify_all();
        for(auto& td : pool_){
            if(td.joinable()){
                std::cout<<"join thread"<<td.get_id()<<std::endl;
                td.join();
            }
        }
    }
private:
    std::mutex cv_mt_;
    std::condition_variable cv_lock_;
    std::atomic_bool stop_;
    std::atomic_int thread_num_;
    std::queue<Task> tasks_;
    std::vector<std::thread> pool_;
};

#endif 