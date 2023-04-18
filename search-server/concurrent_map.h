#pragma once

#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <vector>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>,
            "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : mutexMapVector(bucket_count) {}

    Access operator[](const Key& key) {
        auto& s = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return {std::lock_guard(s.first), s.second[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<std::pair<std::mutex, std::map<Key, Value>>> buckets_;

};