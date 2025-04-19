#ifndef MINHEAP_H
#define MINHEAP_H

#include <vector>
#include <unordered_map>
#include <cstdint>  // For uint64_t

#define u64 uint64_t

struct HeapElement {
    u64 item;
    u64 count;
};

class MinHeap {
private:
    std::vector<HeapElement> heap;
    std::unordered_map<u64, size_t> itemIndexMap;  // item -> heap index
    const u64 k;

    // Maintain min-heap property by moving element down
    void siftDown(size_t index) {
        size_t smallest = index;
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;

        if (left < heap.size() && heap[left].count < heap[smallest].count) {
            smallest = left;
        }
        if (right < heap.size() && heap[right].count < heap[smallest].count) {
            smallest = right;
        }

        if (smallest != index) {
            std::swap(heap[index], heap[smallest]);
            itemIndexMap[heap[index].item] = index;
            itemIndexMap[heap[smallest].item] = smallest;
            siftDown(smallest);
        }
    }

    // Maintain min-heap property by moving element up
    void siftUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (heap[parent].count <= heap[index].count) break;

            std::swap(heap[parent], heap[index]);
            itemIndexMap[heap[parent].item] = parent;
            itemIndexMap[heap[index].item] = index;
            index = parent;
        }
    }

public:

    MinHeap(u64 k) : k(k) {}

    void insertOrUpdate(u64 item, u64 count) {
        if (itemIndexMap.find(item) != itemIndexMap.end()) {
            // Existing item
            size_t idx = itemIndexMap[item];
            if (count > heap[idx].count) {
                heap[idx].count = count;
                siftDown(idx);
            }
        } else {
            // New item
            if (heap.size() < k) {
                heap.push_back({item, count});
                itemIndexMap[item] = heap.size() - 1;
                siftUp(heap.size() - 1);
            } else if (count > heap[0].count) {
                // Evict smallest (root) and insert new
                itemIndexMap.erase(heap[0].item);
                heap[0] = {item, count};
                itemIndexMap[item] = 0;
                siftDown(0);
            }
        }
    }
  size_t size() const {
    size_t total = sizeof(k) + sizeof(heap) + sizeof(itemIndexMap);
    total += heap.capacity() * sizeof(HeapElement);
    // key (8) + value (8) + hash (8) + pointers (16)
    total += itemIndexMap.size() * (sizeof(u64) + sizeof(u64) + 16);
    return total;
  }
    std::vector<HeapElement> getTopK() const {
        return heap;
    }
};

#endif // MINHEAP_H
