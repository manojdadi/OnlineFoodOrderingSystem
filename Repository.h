// Repository.h - a generic container that stores any kind of object which has getId()
// OOP concept: Templates (generic programming). The same class is reused for
//              menu items, users and orders:  Repository<MenuItem>, Repository<User>, Repository<Order>
#pragma once
#include <algorithm>
#include <memory>
#include <vector>

template <class T>
class Repository {
    std::vector<std::shared_ptr<T>> records;

public:
    void add(const std::shared_ptr<T>& record) { records.push_back(record); }

    // Returns nullptr when nothing matches
    std::shared_ptr<T> findById(int id) const {
        for (const auto& record : records)
            if (record->getId() == id) return record;
        return nullptr;
    }

    bool removeById(int id) {
        auto it = std::remove_if(records.begin(), records.end(),
                                 [id](const std::shared_ptr<T>& record) { return record->getId() == id; });
        bool removed = it != records.end();
        records.erase(it, records.end());
        return removed;
    }

    int getNextId(int startFrom = 1) const {
        int highest = startFrom - 1;
        for (const auto& record : records) highest = std::max(highest, record->getId());
        return highest + 1;
    }

    const std::vector<std::shared_ptr<T>>& getAll() const { return records; }
    size_t size() const { return records.size(); }
    bool isEmpty() const { return records.empty(); }
};
