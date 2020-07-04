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
    ssize_t _capacity;
    ssize_t _read_index;
    ssize_t _write_index;

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

    char *writebegin() const
    {
        return _data + _write_index;
    }

    char *readbegin() const
    {
        return _data+ _read_index;
    }

    // size, returns number's of unread bytes of the buffer.
    ssize_t size() const
    {
        assert(_write_index >= _read_index);
        return _write_index - _read_index;
    }
    // capacity, returns number's of the buffer capacity bytes.
    ssize_t capacity() const
    {
        return _capacity;
    }

    void append(const void *b, ssize_t l)
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

    ssize_t remains() const
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

    void updateReadIndex(ssize_t l){
        memset(_data,0x0,_read_index + l);
        _read_index += l;
    }

    void toString()const{
        cout << "buffer len: " <<size() << endl;
    }
    ssize_t readsocket(evutil_socket_t fd);
    ssize_t writesocket(evutil_socket_t fd);
};
