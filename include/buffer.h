#pragma once
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <functional>
#include <iostream>
#include <event.h>
using namespace std;
#define DEF_LEN (128)
#define PAGS_MAX (16)
class buffer
{
private:
    char *_data;
    size_t _capacity;
    size_t _read_index;
    size_t _write_index;

private:
    char *begin() const
    {
        return _data;
    }
    char *writebegin() const
    {
        return begin() + _write_index;
    }

    char *readbegin() const
    {
        return begin() + _read_index;
    }

public:
    buffer() : _data(nullptr), _capacity(0), _read_index(0), _write_index(0) {}
    buffer(int len) : _capacity(len), _read_index(0), _write_index(0)
    {
        _data = new char[_capacity];
        memset(_data, 0x0, _capacity);
        _capacity = _capacity;
    }
    /* for move */
    buffer(buffer &&b)
    {
        if (this != &b)
        {
            this->_data = b._data;
            this->_capacity = b._capacity;
            this->_capacity = b._capacity;
            this->_read_index = b._read_index;
            this->_write_index = b._write_index;
            b._data = nullptr;
            b._capacity = 0;
            b._write_index = 0;
            b._read_index = 0;
        }
    }
    /* for copy */
    buffer(buffer &b)
    {
        if (this != &b)
        {
            this->_data = new char[b._capacity];
            this->_capacity = b._capacity;
            this->_capacity = b._capacity;
            this->_read_index = b._read_index;
            this->_write_index = b._write_index;
            memcpy(this->_data, b._data, this->_capacity);
        }
    }
    ~buffer()
    {
        delete[] _data;
        _data = nullptr;
        _capacity = 0;
        _read_index = 0;
        _write_index = 0;
    }

    // size, returns number's of unread bytes of the buffer.
    size_t size() const
    {
        assert(_write_index >= _read_index);
        return _write_index - _read_index;
    }
    // capacity, returns number's of the buffer capacity bytes.
    size_t capacity() const
    {
        return _capacity;
    }

    void append(const void *b, size_t l)
    {
        assert(b != nullptr);
        const char *p = static_cast<const char *>(b);

        if (l < remains())
        {
            memcpy(writebegin(), p, l);
            _write_index += l;
            return;
        }

        if (_capacity > 0 && _read_index > 0)
        {
            if (_read_index == _write_index)
            {
                _read_index = 0;
                _write_index = 0;
            }
            else
            {
                memcpy(_data, _data + _read_index, _write_index - _read_index);
                _write_index = _write_index - _read_index;
                _read_index = 0;
            }
        }

        if (remains() > l)
        {
            memcpy(writebegin(), p, l);
            _write_index += l;
            return;
        }

        for (int i = 1; i < PAGS_MAX + 1; i++)
        {
            if (i * DEF_LEN + remains() > 0)
            {
                _data =  static_cast<char *>(realloc(_data, i * DEF_LEN + _capacity));
                _capacity += _capacity + i * DEF_LEN;
                memcpy(writebegin(), p, l);
                _write_index += l;
                break;
            }
        }
    }

    size_t remains() const
    {
        assert(_capacity >= _write_index);
        return _capacity - _write_index;
    }

    const char *data() const
    {
        return _data + _read_index;
    }

    void reset(){
        delete[] _data;
        _data = nullptr;
        _capacity = 0;
        _read_index = 0;
        _write_index = 0;
    }

    void toString(){
        cout << size() << endl;
    }
    size_t readsocket(evutil_socket_t fd);
    size_t writesocket(evutil_socket_t fd);
};
