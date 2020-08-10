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

void test2(int arg)
{
    cout << "测试 2" << endl;
}
static void (*f3(int t))(int)
{
    cout << "this is f2 " << t << endl;
    return (test2);
}
typedef void (*F_TYPE2)(int);
int main()
{
    F_TYPE2 f = nullptr; // 使用函数指针类型，定义一个函数变量，并将该函数变量置为nullptr(也可以：F_TYPE f = 0;)
    f = f3(8);           //调用函数f3，返回一个返回类型为void 参数为int的函数地址，这里返回的是test2的函数，
    f(2);                //执行该函数变量

    streambuffer sb;
	// initialize output stream with that output buffer
	std::ostream out(&sb);

	out << "31 hexadecimal: "
	    << std::hex << 31 << std::endl;
    return 0;
}
