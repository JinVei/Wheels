#ifndef __GARBAGECOLLECTOR_H_
#define __GARBAGECOLLECTOR_H_
#include <map>
#include <list>
#include <functional>
//no thread safa
class GarbageCollector;

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
        _object_mem = mem;
    }

    base_gcobject_ref(char* mem, base_gcobject_ref& owner) :base_gcobject_ref(mem) {
        if(owner._object_mem != (char*)&_null_ref)
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

protected:
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

public:
    //base_gcobject_ref*                                  _ref_owner = nullptr;//
    char*                                               _object_mem = (char*) &_null_ref;
    //std::map<base_gcobject_ref*, base_gcobject_ref*>    _sub_ref;
    std::list<base_gcobject_ref>    _sub_ref_list;

    static long long _null_ref;

    using gcobject_destructor_t = void (*) (void*);
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

    gcobject_ref(T* mem) : base_gcobject_ref((char*)mem)
    {}

    gcobject_ref(T* mem, base_gcobject_ref* owner) : base_gcobject_ref((char*)mem, owner)
    {}

    gcobject_ref(T&& other)
    {
        base_gcobject_ref(std::forward<T>(other));
    }

    bool expired() {
        return _object_mem==_null_ref ? true : false;
    }

    T& operator= (T&& other) {
        base_gcobject_ref::operator=(std::forward<T>(other));
        return *this;
    }

    gcobject_ref& operator= (std::nullptr_t other) {
        base_gcobject_ref::operator=(other);
        return *this;
    }

    T& operator* () {//when -men ref nullptr
        return *((T*)_object_mem);
    }

    static void gcobject_destructor(void* object_mem) {
        ((T*)object_mem)->~T();
    }
};

class GarbageCollector {
public :
    void clean_gcobject() {
        size_t new_gc_object_addr;
        size_t destructor;
        size_t object_len;
        char*  gcobject_addr;
        //base_gcobject_ref::gcobject_destructor_t t;


        char* old_begin_addr = m_memory[m_current_mem_index];
        char* old_end_addr = m_avaliable_mem_addr;

        m_current_mem_index = (m_current_mem_index + 1) % 2;
        m_avaliable_mem_addr = m_memory[m_current_mem_index];

        //2019/6/22 如何标志和清理，如何将标志的对象移动到新地址
        //先调用mark标志对象
        //而后清理没有标志的对象，并且把标志的对象移动到新地址（在旧地址处记录新地址）
        //更新gc对象的引用
        mark_gcobject(m_root_refs);

        while(old_begin_addr < old_end_addr){
            new_gc_object_addr = *((size_t*)old_begin_addr);
            old_begin_addr += sizeof(size_t);

            destructor = *((size_t*)old_begin_addr);
            old_begin_addr += sizeof(size_t);

            object_len = *((size_t*)old_begin_addr);
            old_begin_addr += sizeof(size_t);

            gcobject_addr = old_begin_addr;
            old_begin_addr += object_len;

            if (new_gc_object_addr == 0) {
                ((base_gcobject_ref::gcobject_destructor_t)destructor) ((void*)gcobject_addr);
            }
            else {
                *((size_t*)(new_gc_object_addr - 3 * sizeof(size_t))) = 0;
                memcpy((void*)(new_gc_object_addr - 2 * sizeof(size_t)), (void*)(gcobject_addr - 2 * sizeof(size_t)), sizeof(size_t));
                memcpy((void*)(new_gc_object_addr - sizeof(size_t)), (void*)(gcobject_addr - sizeof(size_t)), sizeof(size_t));
                memcpy((void*)new_gc_object_addr, (void*)gcobject_addr, object_len);
                
            }
        }

        update_reference(m_root_refs);
    }
    void mark_gcobject(std::list<base_gcobject_ref>& root) {
        for (auto& ref : root) {
            size_t* new_gcobjectaddr_solt = (size_t*)(ref._object_mem - 3 * sizeof(size_t));
            size_t gcobject_size = *((size_t*)ref._object_mem - sizeof(size_t));
            size_t new_allocated_addr = (size_t)allocate_memory(gcobject_size, 0);
            *new_gcobjectaddr_solt = new_allocated_addr;
        }
    }

    void update_reference(std::list<base_gcobject_ref>& root) {
        for (auto& ref : root) {
            size_t* new_gcobjectaddr_solt = (size_t*)(ref._object_mem - 3 * sizeof(size_t));
            ref._object_mem = (char*)(*new_gcobjectaddr_solt);
        }
    }

    char* allocate_memory(size_t size, size_t destructor_addr) {
        size_t allocated_size = size + sizeof(size_t)+ sizeof(size_t) + sizeof(size_t);
        char* allocated_mem_addr;

        if ((m_avaliable_mem_addr + allocated_size) >= m_mem_end[m_current_mem_index]) {
            //m_root_refs.clear();
            if (m_set_ref_root_handler)
                m_set_ref_root_handler(m_root_refs);
            else
                ;//print err log

            clean_gcobject();

            if ((m_avaliable_mem_addr + allocated_size) >= m_mem_end[m_current_mem_index]) {
                //print err
                return nullptr;
            }
        }

        *((size_t*)m_avaliable_mem_addr) = 0x0000;
        m_avaliable_mem_addr += sizeof(size_t);

        *((size_t*)m_avaliable_mem_addr) = destructor_addr;
        m_avaliable_mem_addr += sizeof(size_t);

        *((size_t*)m_avaliable_mem_addr) = size;
        m_avaliable_mem_addr += sizeof(size_t);

        allocated_mem_addr = m_avaliable_mem_addr;
        m_avaliable_mem_addr += size;

        return allocated_mem_addr;
    }

    template<typename ClassType, typename... Args>
    auto gcobject_creator(base_gcobject_ref& owner, Args... args) -> gcobject_ref<ClassType> {
        char* object_addr = allocate_memory(sizeof(ClassType), (size_t)(gcobject_ref<ClassType>::gcobject_destructor));
        new(object_addr) ClassType(args...);

        gcobject_ref<ClassType> object_ref(object_addr, owner);
        return object_ref;
    }

    void set_root_refs_list(std::list<base_gcobject_ref>& refs_list) {
        m_root_refs = refs_list;
    }

    void set_root_refs_list(std::list<base_gcobject_ref>&& refs_list) {
        m_root_refs = std::move(refs_list);
    }

    GarbageCollector(char* mem, size_t size) {
        if (size < 2) return;

        m_memory[0] = mem;
        m_mem_end[0] = mem + size / 2;

        m_memory[1] = mem + size / 2;
        m_mem_end[0] = mem + size - 1;

        m_current_mem_index = 0;

        m_avaliable_mem_addr = m_memory[0];
    }

    char*       m_memory[2];
    char*       m_mem_end[2];

    size_t      m_current_mem_index = 0;

    char*       m_avaliable_mem_addr = nullptr;

    std::function<void(std::list<base_gcobject_ref>&)>  m_set_ref_root_handler = nullptr;
    std::list<base_gcobject_ref>                        m_root_refs;
};

#endif //ifndef
