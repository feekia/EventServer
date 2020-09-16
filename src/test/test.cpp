#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <algorithm>
#include <valarray>

#include "buffer.h"
using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include "cereal/archives/binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/string.hpp" //一定要包含此文件，否则无法将std::string序列化为二进制形式，请看：https://github.com/USCiLab/cereal/issues/58

using namespace std;

struct MyRecord
{
    int x, y;
    float z;

    template <class Archive>
    void serialize(Archive &ar)
    {
        ar(x, y, z);
    }

    friend std::ostream &operator<<(std::ostream &os, const MyRecord &mr);
};

std::ostream &operator<<(std::ostream &os, const MyRecord &mr)
{
    os << "MyRecord(" << mr.x << ", " << mr.y << "," << mr.z << ")\n";
    return os;
}

struct SomeData
{
    int32_t id;
    std::shared_ptr<std::unordered_map<uint32_t, MyRecord>> data;

    SomeData(int32_t id_ = 0) : id(id_), data(new std::unordered_map<uint32_t, MyRecord>)
    {
    }

    template <class Archive>
    void save(Archive &ar) const
    {
        ar(id, data);
    }

    template <class Archive>
    void load(Archive &ar)
    {
        ar(id, data);
    }

    void push(uint32_t, const MyRecord &mr)
    {
        data->insert(std::make_pair(100, mr));
    }

    void print()
    {
        std::cout << "ID : " << id << "\n";
        if (data->empty())
            return;
        for (auto &item : *data)
        {
            std::cout << item.first << "\t" << item.second << "\n";
        }
    }
};

void Serialization_XML()
{
    {
        std::ofstream os("my.xml");

        cereal::XMLOutputArchive archive(os);

        int age = 26;
        std::string name = "lizheng";

        //#define CEREAL_NVP(T) ::cereal::make_nvp(#T, T)
        archive(CEREAL_NVP(age), cereal::make_nvp("Name", name));

        //os.close();  //注意：这里不能显示关闭ofstream，否则序列化无法写入到文件
    }

    {
        std::ifstream is("my.xml");
        cereal::XMLInputArchive archive(is);

        int age;
        std::string name;

        archive(age, name);
        std::cout << "Age: " << age << "\n"
                  << "Name: " << name << "\n";
    }
}

void Serialization_JSON()
{
    {
        std::ofstream os("my.json");
        cereal::JSONOutputArchive archive(os);

        int age = 26;
        std::string name = "lizheng";

        archive(CEREAL_NVP(age), cereal::make_nvp("Name", name));
    }

    {
        std::ifstream is("my.json");
        cereal::JSONInputArchive archive(is);

        int age;
        std::string name;

        archive(age, name);
        std::cout << "Age: " << age << "\n"
                  << "Name: " << name << "\n";
    }
}

void Serialization_Binary()
{
    {
        std::ofstream os("my.binary", std::ios::binary);
        cereal::BinaryOutputArchive archive(os);

        int age = 26;
        std::string name = "lizheng";

        archive(CEREAL_NVP(age), CEREAL_NVP(name));
    }
    {
        std::ifstream is("my.binary", std::ios::binary);
        cereal::BinaryInputArchive archive(is);

        int age;
        std::string name;

        archive(age, name);
        std::cout << "Age: " << age << "\n"
                  << "Name: " << name << "\n";
    }
}

void Serialization_Obj()
{
    {
        std::ofstream os("obj.cereal", std::ios::binary);
        cereal::BinaryOutputArchive archive(os);

        MyRecord mr = {1, 2, 3.0};

        SomeData myData(1111);
        myData.push(100, mr);

        archive(myData);
    }
    {
        std::ifstream is("obj.cereal", std::ios::binary);
        cereal::BinaryInputArchive archive(is);

        SomeData myData;
        archive(myData);
        myData.print();
    }
}
#include <sstream>
void Serialization_String()
{
    std::stringstream ss(new char[1024]);
    {
        cereal::BinaryOutputArchive archive(ss);

        MyRecord mr = {1, 2, 55.0};

        SomeData myData(1111);
        myData.push(100, mr);

        archive(myData);
        std::cout << ss.rdbuf()->in_avail() << std::endl;
        std::cout << ss.rdbuf()->str() << std::endl;
    }
    {
        cereal::BinaryInputArchive archive(ss);

        SomeData myData;
        archive(myData);
        myData.print();
    }
}

#include <iostream>
#include "streambuffer.hpp"
void Serialization_Streambuffer()
{
    streambuffer sb;
    std::ostream ss(&sb);
    {
        cereal::BinaryOutputArchive archive(ss);

        MyRecord mr = {1, 2, 55.0};

        SomeData myData(1111);
        myData.push(100, mr);

        archive(myData);
    }
    // {
    //     cereal::BinaryInputArchive archive(ss);

    //     SomeData myData;
    //     archive(myData);
    //     myData.print();
    // }
}

class Req
{
public:
    Req() {}
    Req(const string &n) : name(n) {}

private:
    string name;
    friend std::ostream &operator<<(std::ostream &os, Req &r)
    {
        os << r.name;
        return os;
    }
};

#include <iostream>
#include <streambuf>
#include <locale>
#include <cstdio>

class outbuf : public std::streambuf
{
protected:
    /* central output function
	 * - print characters in uppercase mode
	 */
    virtual int_type overflow(int_type c)
    {
        if (c != EOF)
        {
            // convert lowercase to uppercase
            c = std::toupper(static_cast<char>(c), getloc());

            // and write the character to the standard output
            if (putchar(c) == EOF)
            {
                return EOF;
            }
        }
        return c;
    }
};

template <class Extractor>
class FilteringInputStreambuf : public streambuf
{
public:
    FilteringInputStreambuf(
        streambuf *source,
        Extractor x,
        bool deleteWhenFinished = false);
    FilteringInputStreambuf(
        streambuf *source,
        bool deleteWhenFinished = false);
    virtual ~FilteringInputStreambuf();
    virtual int overflow(int);
    virtual int underflow();
    virtual int sync();
    virtual streambuf *setbuf(char *p, int len);

    inline Extractor &extractor();

private:
    streambuf *mySource;
    Extractor myExtractor;
    char myBuffer;
    bool myDeleteWhenFinished;
};

template <class Extractor>
int FilteringInputStreambuf<Extractor>::underflow()
{
    int result(EOF);
    if (gptr() < egptr())
        result = *gptr();
    else if (mySource != NULL)
    {
        result = myExtractor(*mySource);
        if (result != EOF)
        {
            assert(result >= 0 && result <= UCHAR_MAX);
            myBuffer = result;
            setg(&myBuffer, &myBuffer, &myBuffer + 1);
        }
    }
    return result;
}

template <class Extractor>
int FilteringInputStreambuf<Extractor>::sync()
{
    int result(0);
    if (mySource != NULL)
    {
        if (gptr() < egptr())
        {
            result = mySource->sputbackc(*gptr());
            setg(NULL, NULL, NULL);
        }
        if (mySource->sync() == EOF)
            result = EOF;
    }
    return result;
}

class UncommentExtractor
{
public:
    UncommentExtractor(char commentChar = '#')
        : myCommentChar(commentChar)
    {
    }

    int operator()(streambuf &src)
    {
        int ch(src.sbumpc());
        if (ch == myCommentChar)
        {
            while (ch != EOF && ch != '\n')
                ch = src.sbumpc();
        }
        return ch;
    }

private:
    char myCommentChar;
};

template <class Extractor>
class FilteringIstream
    : private FilteringInputStreambuf<Extractor>,
      public istream
{
public:
    FilteringIstream(istream &source,
                     Extractor x);
    FilteringIstream(istream &source);
    FilteringIstream(
        streambuf *source,
        Extractor x,
        bool deleteWhenFinished = false);
    FilteringIstream(
        streambuf *source,
        bool deleteWhenFinished = false);
    virtual ~FilteringIstream();

    FilteringInputStreambuf<Extractor> *rdbuf();
};

// out put stream test
template <class Inserter>
class FilteringOutputStreambuf : public streambuf
{
public:
    FilteringOutputStreambuf(
        streambuf *dest,
        Inserter i,
        bool deleteWhenFinished = false)
    {
        myDest = dest;
        myDeleteWhenFinished = deleteWhenFinished;
    }
    FilteringOutputStreambuf(
        streambuf *dest,
        bool deleteWhenFinished = false)
    {
        dest = myDest;
        myDeleteWhenFinished = deleteWhenFinished;
    }
    virtual ~FilteringOutputStreambuf()
    {
    }
    virtual int overflow(int ch);
    virtual int underflow();
    virtual int sync();
    virtual streambuf *setbuf(char *p, int len);

    inline Inserter &inserter();

private:
    streambuf *myDest;
    Inserter myInserter;
    bool myDeleteWhenFinished;
};

template <class Inserter>
int FilteringOutputStreambuf<Inserter>::overflow(int ch)
{
    int result(EOF);
    if (ch == EOF)
        result = sync();
    else if (myDest != NULL)
    {
        assert(ch >= 0 && ch <= UCHAR_MAX);
        result = myInserter(*myDest, ch);
    }
    return result;
}

class TimeStampInserter
{
public:
    TimeStampInserter()
        : myAtStartOfLine(true)
    {
    }

    int operator()(streambuf &dst, int ch)
    {
        bool errorSeen(false);
        if (myAtStartOfLine && ch != '\n')
        {
            time_t t(time(NULL));
            tm *time = localtime(&t);
            char buffer[128];
            int length(
                strftime(buffer,
                         sizeof(buffer),
                         "%c: ",
                         time));
            assert(length > 0);
            if (dst.sputn(buffer, length) != length)
                errorSeen = true;
        }
        myAtStartOfLine = (ch == '\n');
        return errorSeen
                   ? EOF
                   : dst.sputc(ch);
    }

private:
    bool myAtStartOfLine;
};

int main()
{
    // create special output buffer
    outbuf ob;
    // initialize output stream with that output buffer
    std::ostream out(&ob);

    out << "31 hexadecimal: "<< std::hex << 31 << std::endl;
    // FilteringOutputStreambuf<TimeStampInserter> filebuf(cout.rdbuf(), TimeStampInserter());

    // std::ostream oo(&filebuf);
    // oo << "asdfasdfasdf"<<endl;
    stringstream ss;
    return 0;
}