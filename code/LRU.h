#include <bits/stdc++.h>
using namespace std;

class LRUCache {
public:
    LRUCache(int capacity) {
        size = capacity;
        cache.clear();
        values.clear();
    }
    
    int get(int key) {
        if(cache.find(key) == cache.end()){
            return -1;
        }
        auto res = *cache[key];
        values.erase(cache[key]);
        values.push_front(res);
        cache[key] = values.begin();
        return res.second;
    }
    
    void put(int key, int value) {
        if(cache.find(key) != cache.end()){
            auto res = *cache[key];
            res.second = value;
            values.erase(cache[key]);
            values.push_front(res);
            cache[key] = values.begin();
            return;
        }
        values.push_front(make_pair(key, value));
        cache[key] = values.begin();
        if(values.size() > size){
            auto last = values.back();
            values.pop_back();
            cache.erase(last.first);
        }
    }
private:
    unordered_map<int, list<pair<int, int>>::iterator> cache;
    list<pair<int, int>> values;
    int size;
};