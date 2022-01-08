#pragma once
#include <algorithm>
#include <assert.h>
#include <event.h>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <string.h>

using namespace std;
#define DEF_LEN  (128)
#define PAGS_MAX (16)

namespace es {
class Buffer {
private:
    char * _data;
    size_t _capacity;
    size_t _read_index;
    size_t _write_index;

public:
    Buffer() : _data(nullptr), _capacity(0), _read_index(0), _write_index(0) {}
    Buffer(int len) : _capacity(len), _read_index(0), _write_index(0) {
        _data = new char[_capacity];
        memset(_data, 0x0, _capacity);
    }
    /* for move */
    Buffer(Buffer &&b) {
        if (this != &b) {
            this->_data        = b._data;
            this->_capacity    = b._capacity;
            this->_capacity    = b._capacity;
            this->_read_index  = b._read_index;
            this->_write_index = b._write_index;
            b._data            = nullptr;
            b._capacity        = 0;
            b._write_index     = 0;
            b._read_index      = 0;
        }
    }
    /* for copy */
    Buffer(Buffer &b) {
        if (this != &b) {
            this->_data        = new char[b._capacity];
            this->_capacity    = b._capacity;
            this->_capacity    = b._capacity;
            this->_read_index  = b._read_index;
            this->_write_index = b._write_index;
            memcpy(this->_data, b._data, this->_capacity);
        }
    }
    ~Buffer() {
        delete[] _data;
        _data        = nullptr;
        _capacity    = 0;
        _read_index  = 0;
        _write_index = 0;
    }

    char *writebegin() const { return _data + _write_index; }

    char *readbegin() const { return _data + _read_index; }

    // size, returns number's of unread bytes of the buffer.
    size_t size() const {
        assert(_write_index >= _read_index);
        return _write_index - _read_index;
    }
    // capacity, returns number's of the buffer capacity bytes.
    size_t capacity() const { return _capacity; }

    void append(const void *b, size_t l) {
        assert(b != nullptr);
        const char *p = static_cast<const char *>(b);

        if (l < remains()) {
            memcpy(writebegin(), p, l);
            _write_index += l;
            return;
        }

        if (readApends() + remains() > l) {
            memcpy(_data, _data + _read_index, _write_index - _read_index);
            _write_index = _write_index - _read_index;
            _read_index  = 0;
            memcpy(writebegin(), p, l);
            _write_index += l;
        } else {
            if (_capacity == 0) {
                _capacity = 256 > l ? 256 : (l - (l % 8) + 8);
                _data     = new char[_capacity];
                memcpy(_data, p, l);
                _write_index += l;
                _read_index = 0;
                // cout << "buffer _capacity: " << _capacity << endl;
            } else {
                size_t n = ((_capacity << 1) > l ? (_capacity << 1) : l);
                size_t m = size();
                char * d = new char[n];
                memcpy(d, readbegin(), m);
                _write_index = m;
                _read_index  = 0;
                _capacity    = n;
                delete[] _data;
                _data = d;
                memcpy(writebegin(), p, l);
                _write_index += l;
                // cout << "buffer _capacity 22: " << _capacity << endl;
            }
        }
    }

    size_t remains() const {
        assert(_capacity >= _write_index);
        return _capacity - _write_index;
    }

    size_t      readApends() const { return _read_index; }
    const char *data() const { return _data + _read_index; }

    void reset() {
        if (_data) {
            delete[] _data;
            _data        = nullptr;
            _capacity    = 0;
            _read_index  = 0;
            _write_index = 0;
        }
    }

    void updateReadIndex(size_t l) {
        memset(_data, 0x0, _read_index + l);
        _read_index += l;
    }

    void    toString() const { cout << "buffer len: " << size() << endl; }
    ssize_t readsocket(evutil_socket_t fd);
    ssize_t writesocket(evutil_socket_t fd);
};

} // namespace es