#include <iostream>
#include "../common/assert.h"
int main(){
    int i = 0;
    JV_ASSERT( i == 1 ? 1: 0);
    return 0;
}