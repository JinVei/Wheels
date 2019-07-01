#ifndef __GARBAGECOLLECTOR_H_
#define __GARBAGECOLLECTOR_H_

#include <list>
#include <functional>
namespace jinvei {
    class GarbageCollector;
    class base_gcobject_ref;

    using gcobject_destructor_t = void(*) (void*);

    class gcobject {
    public:
        friend GarbageCollector;
        void put_sub_ref(base_gcobject_ref* gcobject_ref);
    protected:
        std::list<base_gcobject_ref* >    _sub_ref_list;
    };

    class base_gcobject_ref {
    public:
        friend GarbageCollector;

        virtual ~base_gcobject_ref() { }
        base_gcobject_ref();
        base_gcobject_ref(gcobject* owner);
        base_gcobject_ref(nullptr_t null);
        base_gcobject_ref(char* mem);

        base_gcobject_ref& operator= (nullptr_t null);
        base_gcobject_ref& operator= (base_gcobject_ref& other);

        bool expired();

    protected:
        char*                                               _object_mem = (char*)&_null_ref;
        static long long _null_ref;

    };

    template<class T>
    class gcobject_ref : public base_gcobject_ref {
    public:
        ~gcobject_ref() {}
        gcobject_ref() : base_gcobject_ref(){}
        gcobject_ref(gcobject* owner) : base_gcobject_ref(owner) {}
        gcobject_ref(nullptr_t null) : base_gcobject_ref(null) {}
        gcobject_ref(char* mem) : base_gcobject_ref((char*)mem) {}
        gcobject_ref(T* mem) : base_gcobject_ref((char*)mem) {}

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
    public:
        void clean_gcobject();
        void mark_gcobject(char* object_addr);
        void update_reference(std::list<base_gcobject_ref*>& root);
        char* allocate_memory(size_t object_size, size_t destructor_addr);
        void set_root_refs_list(std::list<base_gcobject_ref*>& refs_list);
        void set_root_refs_list(std::list<base_gcobject_ref*>&& refs_list);

        GarbageCollector(char* mem, size_t size);

        template<typename DerivedType>
        gcobject* get_gcobject_addr(char* object_addr, void*) {
            return (gcobject*)0;
        }

        template<typename DerivedType>
        gcobject* get_gcobject_addr(char* object_addr, gcobject*) {
            return static_cast<gcobject*>((DerivedType*)object_addr);
        }

        template<typename ClassType, typename... Args>
        auto creator(Args&&... args) -> gcobject_ref<ClassType> {
            gcobject* gcobject_addr = 0;
            char* object_addr = allocate_memory(sizeof(ClassType), (size_t)(gcobject_ref<ClassType>::gcobject_destructor));
            if (object_addr != nullptr) {
                new(object_addr) ClassType(std::forward<Args>(args)...);

                gcobject_addr = get_gcobject_addr<ClassType>(object_addr, (ClassType*)0);

                *((size_t*)(object_addr - 4 * sizeof(size_t))) = (size_t)gcobject_addr;

                return gcobject_ref<ClassType>(object_addr);;
            }
            else
                return gcobject_ref<ClassType>(nullptr);
        }

    protected:
        char*       m_memory[2];
        char*       m_mem_end[2];

        size_t      m_current_mem_index = 0;

        char*       m_avaliable_mem_addr = nullptr;

        std::function<void(std::list<base_gcobject_ref*>&)>  m_set_ref_root_handler = nullptr;
        std::list<base_gcobject_ref*>                        m_root_refs;
    };
}
#endif //ifndef
