/* ************************************************************************
> File Name:     ActorSingle.h
> Author:        程序员XiaoJiu
> mail           zy18790460359@gmail.com
> Created Time:  五  5/ 3 14:11:46 2024
> Description:   
 ************************************************************************/
#pragma once
#include <thread>
#include "ThreadSafeQue.h"
#include <atomic>
#include <iostream>
template<typename ClassType, typename QueType>
class ActorSingle {
public:
	static ClassType& Inst() {
		static ClassType as;
		return as;
	}

	~ ActorSingle(){
		
	}

	void PostMsg(const QueType& data) {
		_que.push(data);
	}

protected:
	
	ActorSingle():_bstop(false){
		
	}

	ActorSingle(const ActorSingle&) = delete;
	ActorSingle(ActorSingle&&) = delete;
	ActorSingle& operator = (const ActorSingle&) = delete;

	std::atomic<bool> _bstop;
	ThreadSafeQue<QueType>  _que;
	std::thread _thread;
};


