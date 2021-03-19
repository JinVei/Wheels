#include <iostream>
#include "../cache/LRUCache.hpp"
#include "../common/assert.h"

int main(){
    LRUCache<int,int> cache(3);
    
    auto d =  cache.Get(11);
    JV_ASSERT( d == nullptr);
    cache.Set(11, 112);
    d =  cache.Get(11);
    JV_ASSERT( d != nullptr && *d == 112);

    auto d2 =  cache.Get(22);
    JV_ASSERT( d2 == nullptr);
    cache.Set(22, 222);
    d2 =  cache.Get(22);
    JV_ASSERT( d != nullptr && *d2 == 222);

    auto d3 =  cache.Get(33);
    JV_ASSERT( d3 == nullptr);
    cache.Set(33, 333);
    d3 =  cache.Get(33);
    JV_ASSERT( d3 != nullptr && *d3 == 333);

    // auto d1 =  cache.Get(11);
    // JV_ASSERT( d1 != nullptr && *d1 == 112);

    auto d4 =  cache.Get(44);
    JV_ASSERT( d4 == nullptr);
    cache.Set(44, 444);
    d4 =  cache.Get(44);
    JV_ASSERT( d4 != nullptr && *d4 == 444);

    auto d5 =  cache.Get(55);
    JV_ASSERT( d5 == nullptr);
    cache.Set(55, 555);
    d5 =  cache.Get(55);
    JV_ASSERT( d5 != nullptr && *d5 == 555);

    auto d11 =  cache.Get(11);
    JV_ASSERT( d11 == nullptr);


    auto d111 =  cache.Get(11);
    JV_ASSERT( d111 == nullptr);

    auto d222 =  cache.Get(22);
    JV_ASSERT( d222 == nullptr);

    auto d333 =  cache.Get(33);
    JV_ASSERT( d333 != nullptr &&  *d333 == 333);

    auto d444 =  cache.Get(44);
    JV_ASSERT( d444 != nullptr &&  *d444 == 444);

    auto d555 =  cache.Get(55);
    JV_ASSERT( d555 != nullptr && *d555 == 555);

    return 0;
}