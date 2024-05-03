/* ************************************************************************
> File Name:     ClassA.h
> Author:        程序员XiaoJiu
> mail           zy18790460359@gmail.com
> Created Time:  五  5/ 3 14:12:43 2024
> Description:   
 ************************************************************************/
#pragma once
#include "ActorSingle.h"
#include "ClassB.h"

struct MsgClassA {
	std::string name;
	friend std::ostream& operator << (std::ostream& os, const MsgClassA& ca) {
		os << ca.name;
		return os;
	}
};


class ClassA : public ActorSingle<ClassA, MsgClassA> {
	friend class ActorSingle<ClassA, MsgClassA>;
public:
	~ClassA() {
		_bstop = true;
		_que.NotifyStop();
		_thread.join();
		std::cout << "ClassA destruct " << std::endl;
	}

	void DealMsg(std::shared_ptr<MsgClassA> data) {
		std::cout << "class A deal msg is " << *data << std::endl;

		MsgClassB msga;
		msga.name = "llfc";
		ClassB::Inst().PostMsg(msga);
	}
private:
	ClassA(){
		_thread = std::thread([this]() {
			for (; (_bstop.load() == false);) {
				std::shared_ptr<MsgClassA> data = _que.WaitAndPop();
				if (data == nullptr) {
					continue;
				}

				DealMsg(data);
			}

			std::cout << "ClassA thread exit " << std::endl;
			});
	}
};
