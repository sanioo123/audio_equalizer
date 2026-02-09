#pragma once
#include <vector>
#include <atomic>
#include <cstring>

template<typename T>
class CircularBuffer {
public:
    explicit CircularBuffer(size_t capacity) {
        capacity_ = 1;
        while (capacity_ < capacity) capacity_ <<= 1;
        buffer_.resize(capacity_);
    }

    bool write(const T* data, size_t count) {
        size_t w = writePos_.load(std::memory_order_relaxed);
        size_t r = readPos_.load(std::memory_order_acquire);

        size_t available = capacity_ - (w - r);
        if (count > available) return false;

        size_t wMasked = w & (capacity_ - 1);
        size_t firstChunk = capacity_ - wMasked;

        if (count <= firstChunk) {
            std::memcpy(&buffer_[wMasked], data, count * sizeof(T));
        } else {
            std::memcpy(&buffer_[wMasked], data, firstChunk * sizeof(T));
            std::memcpy(&buffer_[0], data + firstChunk, (count - firstChunk) * sizeof(T));
        }

        writePos_.store(w + count, std::memory_order_release);
        return true;
    }

    bool read(T* data, size_t count) {
        size_t r = readPos_.load(std::memory_order_relaxed);
        size_t w = writePos_.load(std::memory_order_acquire);

        size_t available = w - r;
        if (count > available) return false;

        size_t rMasked = r & (capacity_ - 1);
        size_t firstChunk = capacity_ - rMasked;

        if (count <= firstChunk) {
            std::memcpy(data, &buffer_[rMasked], count * sizeof(T));
        } else {
            std::memcpy(data, &buffer_[rMasked], firstChunk * sizeof(T));
            std::memcpy(data + firstChunk, &buffer_[0], (count - firstChunk) * sizeof(T));
        }

        readPos_.store(r + count, std::memory_order_release);
        return true;
    }

    size_t readAvailable() const {
        size_t w = writePos_.load(std::memory_order_acquire);
        size_t r = readPos_.load(std::memory_order_relaxed);
        return w - r;
    }

    size_t writeAvailable() const {
        size_t w = writePos_.load(std::memory_order_relaxed);
        size_t r = readPos_.load(std::memory_order_acquire);
        return capacity_ - (w - r);
    }

    void reset() {
        writePos_.store(0, std::memory_order_relaxed);
        readPos_.store(0, std::memory_order_relaxed);
    }

private:
    std::vector<T> buffer_;
    size_t capacity_;
    std::atomic<size_t> writePos_{0};
    std::atomic<size_t> readPos_{0};
};
