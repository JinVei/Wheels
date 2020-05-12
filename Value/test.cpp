#include "Value.hpp"
#include <iostream>

struct A {
    ~A(){
        std::cout<<"A\n";
    }
};

int main() {
    Value val;
    
    val.Set(1);
    int i = val.Get<int>();
    std::cout<<i<<std::endl;

    A a;
    val.Set(a);
    val.Set(3.3);
    double f = val.Get<double>();
    std::cout<<f<<std::endl;

    try {
        int i1 = val.Get<int>();
        std::cout<<i1<<std::endl;//throw error;
    } catch(std::runtime_error e) {
        std::cout<<e.what()<<std::endl;
    }

    Value val2(val);
    Value val3;
    Value val4(val3);
    Value val5;
    Value val6(std::move(val5));
    val5.Set(1);
    val5.Set("aaa");
    Value val7 = val5;
    val5.Set(A());
    Value val8 = std::move(val5);

    return 0;
}
