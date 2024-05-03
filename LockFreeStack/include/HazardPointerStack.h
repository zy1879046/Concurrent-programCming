#pragma once
#include <mutex>
#include <queue>
#include <atomic>
#include <memory>
#include <thread>

//最大风险指针数量
unsigned const max_hazard_pointers = 100;

//风险指针
struct hazard_pointer{
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;
};

//风险指针数组
extern hazard_pointer hazard_pointers[max_hazard_pointers];
//风险指针持有类
class hp_owner{
public:
    hp_owner(const hp_owner&) = delete;
    hp_owner& operator=(const hp_owner&) = delete;
    hp_owner():hp(nullptr){
        bind_hazard_pointer();//绑定风险指针
    }
    std::atomic<void*>& get_pointer(){
        return hp->pointer;
    }

    ~hp_owner(){
        hp->pointer.store(nullptr);
        hp->id.store(std::thread::id());
    }
private:
    void bind_hazard_pointer(){
        for(unsigned i = 0; i < max_hazard_pointers; ++i){
            std::thread::id old_id;
            if(hazard_pointers[i].id.compare_exchange_strong(old_id,std::this_thread::get_id())){
                hp = &hazard_pointers[i];
                break;
            }
        }
        if(!hp){
            throw std::runtime_error("No hazard_pointers available");
        }
    }
    hazard_pointer* hp;
};

std::atomic<void*>& get_hazard_pointer_for_current_thread(){
   //每个线程具有自己的风险指针 ，且只有一个，在第一次启用时会进行初始化，其他时候不会进行初始化
    thread_local static hp_owner hazzard;
    return hazzard.get_pointer();
}

template <typename T>
class hazard_pointer_stack{
private:
    //栈节点
    struct node{
        std::shared_ptr<T> data;
        node* next;
        node(T const& data_) : data(std::make_shared<T>(data_)){

        }
    };
    
    //待删节点
    struct data_to_reclaim{
        node* data;
        data_to_reclaim* next;
        data_to_reclaim(node* p) : data(p),next(nullptr){

        }
        ~data_to_reclaim(){
            delete data;
        }
    };

    hazard_pointer_stack(const hazard_pointer_stack&) = delete;
    hazard_pointer_stack& operator=(const hazard_pointer_stack&) = delete;
    std::atomic<node*> head;
    std::atomic<data_to_reclaim*> nodes_to_reclaim;

public:
    hazard_pointer_stack(){

    }

    void push(T const& data){
        node* const new_node = new node(data);
        new_node->next = head.load();
        while(!head.compare_exchange_weak(new_node->next,new_node));
    }

    bool outstanding_hazard_pointers_for(void *p){//判断当前节点是否被风险指针所指向
        for(unsigned i = 0; i < max_hazard_pointers;++i){
            if(hazard_pointers[i].pointer.load() == p){
                return true;
            }
        }
        return false;
    }

    void add_to_reclaim_list(data_to_reclaim* reclaim_node){//将当前节点加入到待删链表中
        reclaim_node->next = nodes_to_reclaim.load();
        while(!nodes_to_reclaim.compare_exchange_weak(reclaim_node->next,reclaim_node));
    }

    void reclaim_later(node* old_head){
        add_to_reclaim_list(new data_to_reclaim(old_head));
    }

    void delete_nodes_with_no_hazards()//删除没有被风险指针指向的待删节点
    {
        data_to_reclaim* current = nodes_to_reclaim.exchange(nullptr);//将待删链表取出来
        while(current){
            data_to_reclaim* const next = current->next;
            if(!outstanding_hazard_pointers_for(current->data)){//若当前节点没有被风险指针指向，删除该节点
                delete current;
            }else{//否则加入到待删链表中
                add_to_reclaim_list(current);
            }
            current = next;
        }
    }

    std::shared_ptr<T> pop(){
        std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();//从风险指针中取出一个节点给当前线程
        node* old_head = head.load();
        do{
            node* temp;
            do{
                temp = old_head;
                hp.store(old_head);
                old_head = head.load();
            }while(old_head != temp);//保证当前线程的风险指针指向当前线程应当pop()的值，如果head被其他线程先pop()了，需要重试
        }while(old_head && !head.compare_exchange_strong(old_head,old_head->next));//保证是当前线程pop()，如果被其他线程先pop，重试
        //一旦更新head，则该线程已经与 old_head 没有关系了，所以将当前的风险指针置nullptr
        hp.store(nullptr);
        std::shared_ptr<T> res;
        if(old_head){
            res.swap(old_head->data);
            if(outstanding_hazard_pointers_for(old_head)){//若当前的节点还有被其他的风险指针指向，那么加入待删链表，延迟删除
                reclaim_later(old_head);
            }else{
                delete old_head;//不然可以直接删
            }
            delete_nodes_with_no_hazards();//删除没有风险的节点
        }
        return res;
    }
};
