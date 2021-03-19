#ifndef __JINVEI_CACHE
#define __JINVEI_CACHE
#include <list>
#include <unordered_map>
#include <memory>
#include <iterator>

// No thread safe
// Caller must correctly implement copy constructor and assign operator for DataType
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
        _buf_list.splice(_buf_list.begin(), _buf_list, dat_it);

        return dat_it->data;
    }

    void Set(KeyType key, DataType& data) {
        if (_max_size <= _buf_list.size()) {
            auto evited_dat_it = std::prev(_buf_list.end(), 1);
            auto evited_dat_idx_it = _idx.find(evited_dat_it->key);
            
            _idx.erase(evited_dat_idx_it);

            auto free_buf_it = evited_dat_it;
            free_buf_it->key = key;
            *(free_buf_it->data) = data;

            _buf_list.splice(_buf_list.begin(), _buf_list, free_buf_it);
        } else {
            CacheData cache_dat;
            cache_dat.key = key;
            cache_dat.data = data_ref_t(new DataType(data));
            _buf_list.push_front(cache_dat);
        }

        _idx[key] = _buf_list.begin();
    }

    void Set(KeyType key, DataType&& data) {
        if (_max_size <= _buf_list.size()) {
            auto evited_dat_it = std::prev(_buf_list.end(), 1);
            auto evited_dat_idx_it = _idx.find(evited_dat_it->key);
            
            _idx.erase(evited_dat_idx_it);

            auto free_buf_it = evited_dat_it;
            free_buf_it->key = key;
            *(free_buf_it->data) = std::move(data);

            _buf_list.splice(_buf_list.begin(), _buf_list, free_buf_it);
        } else {
            CacheData cache_dat;
            cache_dat.key = key;
            cache_dat.data = data_ref_t(new DataType(std::move(data)));
            _buf_list.push_front(cache_dat);
        }

        _idx[key] = _buf_list.begin();
    }
};


#endif // __JINVEI_CACHE