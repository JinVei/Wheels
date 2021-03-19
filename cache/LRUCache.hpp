#ifndef __JINVEI_CACHE
#define __JINVEI_CACHE
#include <list>
#include <unordered_map>
#include <memory>
#include <iterator>

template<typename KeyType, typename DataType>
class LRUCache {
public:
    using data_ref_t = std::shared_ptr<DataType>;

        struct CacheData {
        KeyType key;
        data_ref_t data;
    };

    using list_t = std::list<CacheData>;
    using hash_t = std::unordered_map<KeyType, typename list_t::iterator>;

private:
    uint64_t _max_size;
    list_t _buf_list;
    hash_t _idx;
public:
    LRUCache(uint64_t size) {
        _max_size = size;
    }

    data_ref_t Get(const KeyType& key) {
        auto it = _idx.find(key);

        if (it == _idx.end()) {
            return data_ref_t(nullptr);
        }

        auto dat_it = it->second;
        auto cache_dat = *dat_it;
        _buf_list.erase(dat_it);
        _buf_list.push_front(cache_dat);
    
        _idx[key] = _buf_list.begin();

        return cache_dat.data;
    }

    void Set(KeyType key, DataType& data) {
        if (_max_size <= _buf_list.size()) {
            auto evited_dat_it = std::prev(_buf_list.end(), 1);
            auto evited_dat_idx_it = _idx.find(evited_dat_it->key);
            
            if (evited_dat_idx_it != _idx.end()) {
                _idx.erase(evited_dat_idx_it);
            }
            _buf_list.erase(evited_dat_it);
        }
        CacheData cache_dat;
        cache_dat.key = key;
        cache_dat.data = data_ref_t(new DataType(data));
        _buf_list.push_front(cache_dat);
        _idx[key] = _buf_list.begin();
    }

    void Set(KeyType key, DataType&& data) {
        if (_max_size <= _buf_list.size()) {
            auto evited_dat_it = std::prev(_buf_list.end(), 1);
            auto evited_dat_idx_it = _idx.find(evited_dat_it->key);
            
            if (evited_dat_idx_it != _idx.end()) {
                _idx.erase(evited_dat_idx_it);
            }
            _buf_list.erase(evited_dat_it);
        }

        CacheData cache_dat;
        cache_dat.key = key;
        cache_dat.data = data_ref_t(new DataType(std::move(data)));
         _buf_list.push_front(cache_dat);
        _idx[key] = _buf_list.begin();
    }
};


#endif // __JINVEI_CACHE