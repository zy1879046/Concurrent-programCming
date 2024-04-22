#include <iostream>
#include <future>
#include <chrono>

//定义一个异步任务
std::string fetchDataFromDB(std::string query)
{
    //模拟一个异步任务
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return "Data" + query;
}
void testAsync()
{
    // 使用 std::async 异步调用 std::launch::async表示异步执行 而std::launch::defered 表示延迟执行，当遇到 future::get() 或者future::wait() 才开始执行
    std::future<std::string> resultFromDB = std::async(std::launch::async ,fetchDataFromDB,"DATA");
    std::cout<< "Doing Something else..."<<std::endl;
    std::string dbData = resultFromDB.get();//会获得执行后的线程的结果，如果没有执行完则阻塞
    std::cout<<dbData<<std::endl;
}

//std::packaged_task 和 std::future是C++11中引入的两个类，它们用于处理异步任务的结果。
/*以下是使用std::packaged_task和std::future的基本步骤：
        创建一个std::packaged_task对象，该对象包装了要执行的任务。
        调用std::packaged_task对象的get_future()方法，该方法返回一个与任务关联的std::future对象。
        在另一个线程上调用std::packaged_task对象的operator()，以执行任务。
        在需要任务结果的地方，调用与任务关联的std::future对象的get()方法，以获取任务的返回值或异常。*/
int mytask()
{
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout<< "my task run 5 s"<<std::endl;
    return 42;
}

void use_package(){
    //创建一个包含任务的 std::packaged_task 对象
    std::packaged_task<int()> task(mytask);
    // 获取与任务关联的 std::future 对象
    std::future<int> result = task.get_future();
    //在另一个线程执行任务
    std::thread t(std::move(task));
    t.detach();
    //等待任务完成并返回结果
    int value = result.get();
    std::cout<< "The result is: "<<value<<std::endl;
}
//promise 用法
/*C++11引入了std::promise和std::future两个类，用于实现异步编程。
std::promise用于在某一线程中设置某个值或异常，而std::future则用于在另一线程中获取这个值或异常*/
void set_value(std::promise<int> prom){
    prom.set_value(10);
}
void use_promise(){
    //创建一个对象
    std::promise<int> prom;
    //获取该对象相关联的future
    std::future<int> fut = prom.get_future();
    //启动任务线程，设置该值
    std::thread t(set_value,std::move(prom));
    std::cout<<"Waiting for the thread to set the vlaue...\n";
    std::cout<<"Value set by the thread:"<<fut.get()<<std::endl;
    t.join();
}
/*除了set_value()方法外，std::promise还有一个set_exception()方法，用于设置异常。该方法接受一个
std::exception_ptr参数，该参数可以通过调用std::current_exception()方法获取。*/
void set_expection(std::promise<void> prom)
{
    try{
        throw std::runtime_error("An error occurred");
    }catch(...){
        prom.set_exception(std::current_exception());
    }
}
void obtain_exp(){
    std::promise<void> prom;
    std::future<void> fut = prom.get_future();
    std::thread t(set_expection,std::move(prom));
    try{
        std::cout<<"Waiting for the thread to set the exception"<<std::endl;
        fut.get();
    }catch(const std::exception& e){
        std::cout<<"Exception set by the thread: "<<e.what()<<std::endl;
    }
    t.join();
}
//共享类型的future
/*假设你有一个异步任务，需要多个线程等待其完成，然后这些线程需要访问任务的结果。
 在这种情况下，你可以使用std::shared_future来共享异步任务的结果。*/
void myFunction(std::promise<int> prom){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    prom.set_value(42);
}
void threadFunction(std::shared_future<int> future){
    try{
        int result = future.get();
        std::cout << "Result: " << result<<std::endl;
    }catch(const std::future_error& e){
        std::cout<< "Future error: "<<e.what()<<std::endl;
    }
}
void use_shared_future()
{
    std::promise<int> prom;
    std::shared_future<int> future = prom.get_future();
    std::thread myThread1(myFunction,std::move(prom));

    std::thread myThread2(threadFunction,future);
    std::thread myThread3(threadFunction,future);
    myThread1.join();
    myThread2.join();
    myThread3.join();
}
//异常处理
/*std::future 是C++的一个模板类，它用于表示一个可能还没有准备好的异步操作的结果。
你可以通过调用 std::future::get 方法来获取这个结果。如果在获取结果时发生了异常，
那么 std::future::get 会重新抛出这个异常*/
void may_throw(){
    throw std::runtime_error("Oops,something went wrong!");
}
void future_throw()
{
    std::future<void> result(std::async(std::launch::async,may_throw));
    try{
        result.get();
    }catch(const std::exception& e)
    {
        std::cerr <<"Caught exception: "<<e.what()<<std::endl;
    }
}
int main()
{
    //use_package();
    //use_promise();
    // obtain_exp();
    // use_shared_future();
    future_throw();
    return 0;
}