#include "GarbageCollector.h"
using namespace jinvei;

long long base_gcobject_ref::_null_ref = 0;

void gcobject::put_sub_ref(base_gcobject_ref* gcobject_ref) {
    _sub_ref_list.push_back(gcobject_ref);
}

void gcobject::update_sub_ref(base_gcobject_ref* old_ref, base_gcobject_ref* new_ref) {
    for (auto it = _sub_ref_list.begin(); it != _sub_ref_list.end(); ++it) {
        if (old_ref == *it) {
            _sub_ref_list.erase(it);
            break;
        }
    }
    _sub_ref_list.push_back(new_ref);
}

base_gcobject_ref::base_gcobject_ref() {
    _object_mem = (char*)&_null_ref;
}

base_gcobject_ref::base_gcobject_ref(gcobject* owner) : base_gcobject_ref() {
    owner->put_sub_ref(this);
}

base_gcobject_ref::base_gcobject_ref(nullptr_t null) : base_gcobject_ref()
{ }

base_gcobject_ref::base_gcobject_ref(char* mem) {
    if (mem == 0)
        _object_mem = (char*)&_null_ref;
    else
        _object_mem = mem;
}

base_gcobject_ref& base_gcobject_ref::operator= (nullptr_t null) {
    _object_mem = (char*)&_null_ref;
    return *this;
}

base_gcobject_ref& base_gcobject_ref::operator= (base_gcobject_ref& other) {
    _object_mem = other._object_mem;
    return *this;
}

bool base_gcobject_ref::expired() {
    return _object_mem == (char*)&_null_ref ? true : false;
}

void GarbageCollector::clean() {
    char*  old_object_addr;

    char* old_begin_addr = m_memory[m_current_mem_index];
    char* old_end_addr = m_avaliable_mem_addr;

    m_current_mem_index = (m_current_mem_index + 1) % 2;
    m_avaliable_mem_addr = m_memory[m_current_mem_index];

    //2019/6/22 如何标志和清理，如何将标志的对象移动到新地址
    //先调用mark标志对象
    //而后清理没有标志的对象，并且把标志的对象移动到新地址（在旧地址处记录新地址）
    //更新gc对象的引用
    for (auto& ref : m_root_refs){
        mark_gcobject(ref->_object_mem);
    }

    while (old_begin_addr < old_end_addr) {
        object_header& old_object_header = *((object_header*)old_begin_addr);
        old_begin_addr += sizeof(object_header);

        old_object_addr = old_begin_addr;
        old_begin_addr += old_object_header.object_len;

        if (old_object_header.new_object_addr == 0) {
            ((gcobject_destructor_t)old_object_header.destructor) ((void*)old_object_addr);
        }
        else {
            object_header& new_object_header = *((object_header*)(old_object_header.new_object_addr - sizeof(object_header)));
            new_object_header = old_object_header;
            new_object_header.new_object_addr = 0;

            if (old_object_header.gc_object_addr != 0) {
                new_object_header.gc_object_addr = (size_t)(old_object_header.new_object_addr + (old_object_header.gc_object_addr - (size_t)old_object_addr));
            }
            else {
                new_object_header.gc_object_addr = 0;
            }

            memcpy((void*)old_object_header.new_object_addr, (void*)old_object_addr, old_object_header.object_len);

            if (old_object_header.gc_object_addr != 0) {
                gcobject* new_gc_object = (gcobject*)(new_object_header.gc_object_addr);
                gcobject* old_gc_object = (gcobject*)(old_object_header.gc_object_addr);
                new ((char*)&(new_gc_object->_sub_ref_list)) std::list<base_gcobject_ref*>(old_gc_object->_sub_ref_list);
                for (auto& ref : old_gc_object->_sub_ref_list) {
                    new_gc_object->update_sub_ref(ref, (base_gcobject_ref*)((size_t)old_object_header.new_object_addr + ((size_t)ref - (size_t)old_object_addr )));
                }
            }

        }
    }

    update_reference(m_root_refs);
}

void GarbageCollector::mark_gcobject(char* object_addr) {
    object_header& header = *((object_header*)((size_t)object_addr - sizeof(header)));
    gcobject* gcobject_ptr = (gcobject*)header.gc_object_addr;

    if (header.new_object_addr == 0) {
        size_t new_allocated_addr = (size_t)allocate_memory(header.object_len, 0);
        header.new_object_addr = new_allocated_addr;
    }

    if (gcobject_ptr != 0) {
        for (auto& ref : gcobject_ptr->_sub_ref_list) {
            if(!ref->expired())
                mark_gcobject(ref->_object_mem);
        }
    }
}

void GarbageCollector::update_reference(std::list<base_gcobject_ref*>& root) {
    for (auto& ref : root) {
        object_header& old_header = *((object_header*)(ref->_object_mem - sizeof(object_header)));
        ref->_object_mem = (char*)(old_header.new_object_addr);

        object_header& new_header = *((object_header*)(ref->_object_mem - sizeof(object_header)));
        if (new_header.gc_object_addr != 0) {
            gcobject* _gcobject = (gcobject*)new_header.gc_object_addr;
            update_reference(_gcobject->_sub_ref_list);
        }
    }
}

char* GarbageCollector::allocate_memory(size_t object_size, size_t destructor_addr) {
    size_t allocated_size = object_size + sizeof(object_header);
    char* allocated_mem_addr;

    if ((m_avaliable_mem_addr + allocated_size) >= m_mem_end[m_current_mem_index]) {
        //m_root_refs.clear();
        if (m_set_ref_root_handler)
            m_set_ref_root_handler(m_root_refs);
        else
            ;//print err log

        clean();

        if ((m_avaliable_mem_addr + allocated_size) >= m_mem_end[m_current_mem_index]) {
            //print err
            return nullptr;
        }
    }

    object_header& header = *((object_header*)m_avaliable_mem_addr);
    header.gc_object_addr = 0;
    header.new_object_addr = 0;
    header.destructor = destructor_addr;
    header.object_len = object_size;
    m_avaliable_mem_addr += sizeof(object_header);

    allocated_mem_addr = m_avaliable_mem_addr;
    m_avaliable_mem_addr += object_size;

    return allocated_mem_addr;
}

void GarbageCollector::set_root_refs_list(std::list<base_gcobject_ref*>& refs_list) {
    m_root_refs = refs_list;
}
void GarbageCollector::set_root_refs_list(std::list<base_gcobject_ref*>&& refs_list) {
    m_root_refs = std::move(refs_list);
}

GarbageCollector::GarbageCollector(char* mem, size_t size) {
    if (size < 2) return;

    m_memory[0] = mem;
    m_mem_end[0] = mem + size / 2;

    m_memory[1] = mem + size / 2;
    m_mem_end[1] = mem + size - 1;

    m_current_mem_index = 0;

    m_avaliable_mem_addr = m_memory[0];
}
