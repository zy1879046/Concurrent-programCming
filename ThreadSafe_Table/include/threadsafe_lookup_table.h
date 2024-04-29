#pragma once

#include<iostream>
#include<list>
#include<vector>
#include<memory>
#include<mutex>
#include<shared_mutex>
#include<iterator>
#include<map>

template <typename Key,typename Value,typename Hash = std::hash<Key>>
class threadsafe_lookup_table{
private:
    class bucket_type{
            friend class threadsafe_lookup_table;
        private:
            using bucket_value = std::pair<Key,Value>;
            using bucket_data = std::list<bucket_value>;
            using bucket_iterator = typename bucket_data::iterator;
            bucket_data data;
            mutable std::shared_mutex mutex;
            bucket_iterator find_entry_for(const Key& key){
                return std::find_if(data.begin(),data.end(),[&](bucket_value const &item){
                        return item.first == key;
                        });
            }
        public:
            Value value_for(Key const& key,Value const& default_value){
                std::shared_lock<std::shared_mutex> lock(mutex);
                bucket_iterator const found_entry = find_entry_for(key);
                return (found_entry == data.end()) ? default_value : found_entry->second;
            }

            void add_or_updata_mapping(Key const& key,Value const& value){
                std::unique_lock<std::shared_mutex> lock(mutex);
                auto found_entry = find_entry_for(key);
                if(found_entry == data.end()){
                    data.push_back(bucket_value(key,value));
                }else{
                    found_entry->second = value;
                }
            }

            void remove_mapping(Key const& key){
                std::unique_lock<std::shared_mutex> lock(mutex);
                auto found_entry = find_entry_for(key);
                if(found_entry != data.end()){
                    data.erase(found_entry);
                }
        }
    };
private:
    std::vector<std::unique_ptr<bucket_type>> buckets;
    Hash hasher;
    bucket_type& get_bucket(Key const& key){
        std::size_t const bucket_index = hasher(key) % buckets.size();
        return *(buckets[bucket_index]);
    }
public:
    threadsafe_lookup_table(unsigned bucket_size = 19,Hash const& hasher_ = Hash()) : buckets(bucket_size),hasher(hasher_){
        for(unsigned i = 0; i < bucket_size ; ++i){
            buckets[i].reset(new bucket_type);
        }
    }
    threadsafe_lookup_table(const threadsafe_lookup_table&) = delete;
    threadsafe_lookup_table& operator =(const threadsafe_lookup_table&) = delete;

    Value value_for(Key const& key,Value const& default_value = Value()){
        return get_bucket(key).value_for(key,default_value);
    }
    void add_or_updata_mapping(Key const& key,Value const& value){
        get_bucket(key).add_or_updata_mapping(key,value);
    }
    void remove_mapping(Key const& key){
        get_bucket(key).remove_mapping(key); 
    }

    std::map<Key,Value> get_map(){
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for(unsigned i = 0; i < buckets.size(); ++i){
            locks.push_back(std::unique_lock<std::shared_mutex>(buckets[i]->mutex));
        }
        std::map<Key,Value> res;
        for(unsigned i = 0; i < buckets.size(); ++i){
            typename bucket_type::bucket_iterator it = buckets[i]->data.begin();
            while(it != buckets[i]->data.end()){
                res.insert(*it);
                ++it;
            }
        }
        return res;
    }
};