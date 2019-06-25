#ifndef __GARBAGECOLLECTOR_H_
#define __GARBAGECOLLECTOR_H_

#include <list>
#include <functional>

//no thread safa
class GarbageCollector;

using gcobject_destructor_t = void(*) (void*);

class base_gcobject_ref {
    friend GarbageCollector;

public:
    virtual ~base_gcobject_ref() { }

    base_gcobject_ref() {
        _object_mem = (char*)&_null_ref;
    }

    base_gcobject_ref(nullptr_t null) {
        _object_mem = (char*)&_null_ref;
    }

    base_gcobject_ref(char* mem) {
        if(mem == 0)
            _object_mem = (char*)&_null_ref;
        else
            _object_mem = mem;
    }

    base_gcobject_ref(char* mem, base_gcobject_ref& owner) : base_gcobject_ref(mem) {
        owner._sub_ref_list.push_back(*this);
    }

    base_gcobject_ref(base_gcobject_ref&& other) {
        this->_object_mem = other._object_mem;
        this->_sub_ref_list = std::move(other._sub_ref_list);
    }
    base_gcobject_ref(const base_gcobject_ref& other){
        this->_object_mem = other._object_mem;
        this->_sub_ref_list = other._sub_ref_list;
    }
    base_gcobject_ref(base_gcobject_ref* other) : base_gcobject_ref(*other) 
    {}

    base_gcobject_ref& operator= (nullptr_t null) {
        _object_mem = (char*) &_null_ref;
        _sub_ref_list.clear();
        return *this;
    }

    base_gcobject_ref& operator= (base_gcobject_ref& other) {
        _object_mem = other._object_mem;
        _sub_ref_list = other._sub_ref_list;
        return *this;
    }

    base_gcobject_ref& operator= (base_gcobject_ref&& other) {
        _object_mem = other._object_mem;
        _sub_ref_list = std::move(other._sub_ref_list);
        return *this;
    }

    void put_sub_ref(base_gcobject_ref&& gcobject_ref) {
        _sub_ref_list.push_back(std::move(gcobject_ref));
    }

    void put_sub_ref(base_gcobject_ref& gcobject_ref) {
        _sub_ref_list.push_back(gcobject_ref);
    }

protected:

    char*                                               _object_mem = (char*) &_null_ref;
    std::list<base_gcobject_ref>    _sub_ref_list;

    static long long _null_ref;

};

template<class T>
class gcobject_ref : public base_gcobject_ref{
public:
    ~gcobject_ref() {}

    gcobject_ref() : base_gcobject_ref() 
    {}

    gcobject_ref(nullptr_t null) : base_gcobject_ref(null)
    {}

    gcobject_ref(char* mem) : base_gcobject_ref((char*)mem)
    {}

    gcobject_ref(char* mem, base_gcobject_ref* owner) : base_gcobject_ref((char*)mem, owner)
    {}

    gcobject_ref(char* mem, base_gcobject_ref& owner) : base_gcobject_ref((char*)mem, owner)
    {}

    gcobject_ref(nullptr_t null, base_gcobject_ref& owner) : base_gcobject_ref(null, owner)
    {}

    gcobject_ref(T* mem) : base_gcobject_ref((char*)mem)
    {}

    gcobject_ref(T* mem, base_gcobject_ref* owner) : base_gcobject_ref((char*)mem, owner)
    {}

    gcobject_ref(T&& other)
    {
        base_gcobject_ref(std::forward<T>(other));
    }

    bool expired() {
        return _object_mem == (char*)&_null_ref ? true : false;
    }

    T& operator= (T&& other) {
        base_gcobject_ref::operator=(std::forward<T>(other));
        return *this;
    }

    gcobject_ref& operator= (std::nullptr_t other) {
        base_gcobject_ref::operator=(other);
        return *this;
    }

    T& operator* () {
        return *((T*)_object_mem);
    }
    T* operator-> () {
        return (T*)_object_mem;
    }

    static void gcobject_destructor(void* object_mem) {
        ((T*)object_mem)->~T();
    }
};

class GarbageCollector {
public :
    void clean_gcobject();
    void mark_gcobject(std::list<base_gcobject_ref>& root);
    void update_reference(std::list<base_gcobject_ref>& root);
    char* allocate_memory(size_t object_size, size_t destructor_addr);
    void set_root_refs_list(std::list<base_gcobject_ref>& refs_list);
    void set_root_refs_list(std::list<base_gcobject_ref>&& refs_list);

    GarbageCollector(char* mem, size_t size);

    template<typename ClassType, typename... Args>
    auto gcobject_creator(Args&&... args) -> gcobject_ref<ClassType> {

        char* object_addr = allocate_memory(sizeof(ClassType), (size_t)(gcobject_ref<ClassType>::gcobject_destructor));

        new(object_addr) ClassType(std::forward<Args>(args)...);

        gcobject_ref<ClassType> object_ref(object_addr);
        return object_ref;
    }

public:
    char*       m_memory[2];
    char*       m_mem_end[2];

    size_t      m_current_mem_index = 0;

    char*       m_avaliable_mem_addr = nullptr;

    std::function<void(std::list<base_gcobject_ref>&)>  m_set_ref_root_handler = nullptr;
    std::list<base_gcobject_ref>                        m_root_refs;
};

#endif //ifndef
