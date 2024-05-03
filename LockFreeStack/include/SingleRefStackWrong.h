/* ************************************************************************
> File Name:     SingleRefStackWrong.h
> Author:        程序员XiaoJiu
> mail           zy18790460359@gmail.com
> Created Time:  四  5/ 2 13:44:09 2024
> Description:   
 ************************************************************************/

#pragma once
#include <atomic>
#include <memory>

template <typename T>
class single_ref_stack{
public:
    single_ref_stack():head(nullptr){

    }
    ~single_ref_stack(){
        //循环出栈
        while(pop());
    }

    void push(T const& data){
        auto new_node = new ref_node(data);
        new_node->next = head.load();
        while(!head.compare_exchange_weak(new_node->next,new_node));
    }

    std::shared_ptr<T> pop(){
        ref_node* old_head = head.load();
        for(;;){
            if(!old_head){
                return std::shared_ptr();
            }
            //只要执行pop()就对引用计数加1
            ++(old_head->_ref_count);//若此时有两个线程执行到这里，一个线程先执行到下面的逻辑，并满足条件删除了old_head节点，那么另一个线程就会因为访问nullptr而崩溃
            if(head.compare_exchange_strong(old_head,old_head->next)){//若当前线程先执行则更改head，其他线程访问的head为新的节点，此时执行else
                                                                      //若新的节点的引用计数为1，则此时会导致新节点被删除
                auto current = old_head->_ref_count;
                auto new_count;
                //循环保证引用计数的安全更新，保证current 保存最新的当前节点的引用计数
                do{
                    new_count = current - 2;
                }while(!old_head._ref_count.compare_exchange_weak(current,new_count));

                //返回头部数据
                std::shared_ptr<T> res;
                //交换数据
                res.swap(old_head->_data);
                if(old_head->_ref_count == 0){//若当前节点的引用计数为0，则删除该节点
                    delete old_head;
                }
                return res;
            }
            else{
                if(old_head->_ref_count.fetch_sub(1) == 1){
                    delete old_head;
                }
            }
        }
    }
private:
    struct ref_node{
        //数据域智能指针
        std::shared_ptr<T> _data;
        //引用计数
        std::atomic<int> _ref_count;
        //下一个节点
        ref_node* next;
        ref_node(T const& data_) : _data(std::make_shared<T>(data_)),_ref_count(1),next(nullptr){

        }
    };

    //头部节点
    std::atomic<ref_node*> head;
};
