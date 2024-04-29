/* ************************************************************************
> File Name:     MyClass.h
> Author:        程序员XiaoJiu
> mail           zy18790460359@gmail.com
> Created Time:  一  4/29 14:21:47 2024
> Description:   
 ************************************************************************/
#pragma once
#include<iostream>
class MyClass{
public:
    MyClass(int i):_data(i){

    }
    friend std::ostream& operator<<(std::ostream& os ,MyClass const& mc){
        return os<<mc._data;
    }
private:
    int _data;
};
