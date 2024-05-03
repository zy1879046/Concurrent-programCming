/* ************************************************************************
> File Name:     actor.cpp
> Author:        程序员XiaoJiu
> mail           zy18790460359@gmail.com
> Created Time:  五  5/ 3 14:16:05 2024
> Description:   
 ************************************************************************/
// day20-Actor.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "./include/ClassA.h"
int main()
{
    MsgClassA msga;
    msga.name = "llfc";
    ClassA::Inst().PostMsg(msga);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "main process exited!\n";
}



