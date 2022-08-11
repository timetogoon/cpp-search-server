#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);


    struct Access {    
        std::lock_guard<std::mutex> guard_;
        Value& ref_to_value;
        Access(std::pair<std::map<Key, Value>, std::mutex>& map, const Key& key) :
            guard_(map.second), ref_to_value(map.first[key]) {}
    };

    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count), bucket_count_(bucket_count) {};

    Access operator[](const Key& key);

    void Erase(const Key& key) {
        uint64_t tmp_key = static_cast<uint64_t>(key) % bucket_count_;        
        buckets_[tmp_key].first.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap();

private:
    std::vector<std::pair<std::map<Key, Value>, std::mutex>> buckets_;
    const size_t bucket_count_;
};

template<typename Key, typename Value>
typename ConcurrentMap<Key, Value>::Access
ConcurrentMap<Key, Value>::operator[](const Key& key) {
    auto bucket = static_cast<uint64_t>(key) % bucket_count_;
    return Access(buckets_[bucket], key);
}

template<typename Key, typename Value>
std::map<Key, Value> ConcurrentMap<Key, Value>::BuildOrdinaryMap() {
    std::map<Key, Value> result;
    for (auto bucket = 0; bucket < bucket_count_; ++bucket) {
        buckets_[bucket].second.lock();
    }
    for (auto bucket = 0; bucket < bucket_count_; ++bucket) {
        result.insert(buckets_[bucket].first.begin(), buckets_[bucket].first.end());
    }
    for (auto bucket = 0; bucket < bucket_count_; ++bucket) {
        buckets_[bucket].second.unlock();
    }
    return result;
}
