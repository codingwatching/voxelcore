#pragma once

#include <memory>
#include <cstring>
#include <stdexcept>

namespace util {
    template <typename T>
    class array_queue {
    public:
        array_queue(size_t initCapacity = 8)
        : _capacity(initCapacity),
            _size(0),
            _front(0),
            _back(0),
            _data(new T[_capacity]) {
            if (initCapacity == 0 || (initCapacity & (initCapacity - 1)) != 0) {
                throw std::invalid_argument("initCapacity must be positive power of 2");
            }
        }

        ~array_queue() {
            // todo
            delete[] _data;
        }

        void push(T&& value) {
            if (_back == _front && _size) {
                grow();
            }
            _data[_back] = std::forward<T>(value);
            _back = (_back + 1) & (_capacity - 1);
            ++_size;
        }

        T& front() {
            return _data[_front];
        }

        void pop() {
            _data[_front].~T();
            _front = (_front + 1) & (_capacity - 1);
            --_size;
        }

        bool empty() const {
            return _size == 0;
        }

        size_t size() const {
            return _size;
        }

    private:
        size_t _capacity;
        size_t _size;
        size_t _front;
        size_t _back;
        T* _data;

        void grow() {
            size_t newCapacity = _capacity * 2;
            T* newData = new T[newCapacity];

            std::move(_data + _front, _data + _capacity, newData);
            std::move(_data, _data + _front, newData + (_capacity - _front));

            _back = _size;
            _front = 0;

            delete[] _data;
            _data = newData;
            _capacity = newCapacity;
        }
    };
}
