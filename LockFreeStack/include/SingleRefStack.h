/* ************************************************************************
> File Name:     SingleRefStack.h
> Author:        程序员XiaoJiu
> mail           zy18790460359@gmail.com
> Created Time:  四  5/ 2 14:14:00 2024
> Description:   
 ************************************************************************/
#pragma once
#include <atomic>
#include <memory>

template<typename T>
class single_ref_stack{
public:
    single_ref_stack(){

    }
    ~single_ref_stack(){
        while(pop());
    }
    void push(const T & data){
        auto new_node = ref_node(data);
        new_node._node_ptr->next = head.load();
        while(!head.compare_exchange_weak(new_node._node_ptr->next,new_node));
    }

    std::shared_ptr<T> pop(){
        ref_node old_head = head.load();
        for(;;){
            //只要执行pop就对引用计数+1并更新到head中
            ref_node new_head;
            do{
                new_head = old_head;
                new_head._ref_count += 1;
            }while(!head.compare_exchange_weak(old_head,new_head));//这个是保证更新的head保证与head相等，而不是其他的对head进行操作之后变化的head
            old_head = new_head;
            auto* node_ptr = old_head._node_ptr;
            if(node_ptr == nullptr){
                return std::shared_ptr<T>();
            }

            //比较head与old_head相等则交换否则说明head已经被其他线程更新
            if(head.compare_exchange_strong(old_head,node_ptr->next)){
                //要返回的数据
                std::shared_ptr<T> res;
                //交换智能指针
                res.swap(node_ptr->_data);
                //增加的数量
                int increase_count = old_head._ref_count - 2;
                if(node_ptr->_dec_count.fetch_add(increase_count) == -increase_count){
                    delete node_ptr;
                }
                return res;
            }else{
                if(node_ptr->_dec_count.fetch_sub(1) == 1){
                    delete node_ptr;
                }
            }
        }
    }
private:
    struct ref_node;
    struct node{
        //数据域指针
        std::shared_ptr<T> _data;
        //下一个节点
        ref_node next;
        node(T const& data_) : _data(std::make_shared<T>(data_)){

        }
        //内部引用计数，减少的数量
        std::atomic<int> _dec_count;
    };
    struct ref_node{
        //引用计数
        int _ref_count;
        node* _node_ptr;
        ref_node(T const& data_) : _node_ptr(new node(data_)),_ref_count(1){

        }
        ref_node():_node_ptr(nullptr),_ref_count(0){

        }
    };

    //头部节点，这里是一个拷贝
    std::atomic<ref_node> head;
};
