#include "GarbageCollector.h"

long long base_gcobject_ref::_null_ref = 0;

void GarbageCollector::clean_gcobject() {
    size_t new_gc_object_addr;
    size_t destructor;
    size_t object_len;
    char*  gcobject_addr;

    char* old_begin_addr = m_memory[m_current_mem_index];
    char* old_end_addr = m_avaliable_mem_addr;

    m_current_mem_index = (m_current_mem_index + 1) % 2;
    m_avaliable_mem_addr = m_memory[m_current_mem_index];

    //2019/6/22 如何标志和清理，如何将标志的对象移动到新地址
    //先调用mark标志对象
    //而后清理没有标志的对象，并且把标志的对象移动到新地址（在旧地址处记录新地址）
    //更新gc对象的引用
    mark_gcobject(m_root_refs);

    while (old_begin_addr < old_end_addr) {
        new_gc_object_addr = *((size_t*)old_begin_addr);
        old_begin_addr += sizeof(size_t);

        destructor = *((size_t*)old_begin_addr);
        old_begin_addr += sizeof(size_t);

        object_len = *((size_t*)old_begin_addr);
        old_begin_addr += sizeof(size_t);

        gcobject_addr = old_begin_addr;
        old_begin_addr += object_len;

        if (new_gc_object_addr == 0) {
            ((gcobject_destructor_t)destructor) ((void*)gcobject_addr);
        }
        else {
            *((size_t*)(new_gc_object_addr - 3 * sizeof(size_t))) = 0;
            *((size_t*)(new_gc_object_addr - 2 * sizeof(size_t))) = destructor;
            *((size_t*)(new_gc_object_addr - sizeof(size_t))) = object_len;
            memcpy((void*)new_gc_object_addr, (void*)gcobject_addr, object_len);

        }
    }

    update_reference(m_root_refs);
}

void GarbageCollector::mark_gcobject(std::list<base_gcobject_ref>& root) {
    for (auto& ref : root) {
        size_t* new_gcobjectaddr_solt = (size_t*)(ref._object_mem - 3 * sizeof(size_t));
        size_t gcobject_size = *((size_t*)(ref._object_mem - sizeof(size_t)));

        //base_gcobject_ref* gcobject = dynamic_cast<base_gcobject_ref*>(ref._object_mem);

        if (*new_gcobjectaddr_solt == 0) {
            size_t new_allocated_addr = (size_t)allocate_memory(gcobject_size, 0);
            *new_gcobjectaddr_solt = new_allocated_addr;
        }
        mark_gcobject(ref._sub_ref_list);
    }
}

void GarbageCollector::update_reference(std::list<base_gcobject_ref>& root) {
    for (auto& ref : root) {
        size_t* new_gcobjectaddr_solt = (size_t*)(ref._object_mem - 3 * sizeof(size_t));
        ref._object_mem = (char*)(*new_gcobjectaddr_solt);
    }
}

char* GarbageCollector::allocate_memory(size_t object_size, size_t destructor_addr) {
    size_t allocated_size = object_size + sizeof(size_t) + sizeof(size_t) + sizeof(size_t);
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

    *((size_t*)m_avaliable_mem_addr) = object_size;
    m_avaliable_mem_addr += sizeof(size_t);

    allocated_mem_addr = m_avaliable_mem_addr;
    m_avaliable_mem_addr += object_size;

    return allocated_mem_addr;
}
void GarbageCollector::set_root_refs_list(std::list<base_gcobject_ref>& refs_list) {
    m_root_refs = refs_list;
}
void GarbageCollector::set_root_refs_list(std::list<base_gcobject_ref>&& refs_list) {
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
