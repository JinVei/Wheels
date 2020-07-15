#include <iostream>
#include "../common/backtrace.h"
int main(){
    jv::Backtrace bt;
    bt.print(std::cout);
    return 0;
}