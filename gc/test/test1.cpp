#include "GarbageCollector.h"
#include <iostream>
#include <vector>

using namespace std;
using namespace jinvei;

struct B {
    ~B() {
        std::cout << "~B()" << endl;
    }
};

struct C {
    ~C() {
        std::cout << "~C()" << endl;
    }
};

class D {
public:
    ~D() {
        std::cout << "~D()" << endl;
    }
};

class A : virtual public gcobject{
public:
    A() : b_ref(this), c_ref(this) {
    }
    A(int r1, int r2, int r3) : A() {
        q = r1;
        w = r2;
        e = r3;
    }
    ~A() {
        std::cout << "~A()" << endl;
    }
public:
    gcobject_ref<B> b_ref;
    gcobject_ref<C> c_ref;
    int q;
    int w;
    int e;

};

class E {
public:
    E(int a, int b, int c) : _a(a),_b(b),_c(c){

    }
    ~E() {
        std::cout << "~E()" << endl;
    }
    int _a, _b,_c;
};
void main() {
    GarbageCollector gc(new char[1000], 1000);

    gcobject_ref<A> a_ref = gc.creator<gcobject,A>(99,22,33);
    (*a_ref).b_ref = gc.creator<B>();

    gcobject_ref<C> c_ref1 = gc.creator<C>();

    a_ref->c_ref = c_ref1;

    c_ref1 = nullptr;

    gcobject_ref<D> d_ref = gc.creator<D>();

    vector<E> vec({ E(1, 2, 3) });
    gcobject_ref<std::vector<E>> vec_ref = gc.creator<vector<E>>(vec);

    cout << (*vec_ref)[0]._a << endl;

    cout << a_ref->w << endl;

    gc.set_root_refs_list({ &a_ref, /*&d_ref*/ });
    gc.clean_gcobject();

    getchar();
}
