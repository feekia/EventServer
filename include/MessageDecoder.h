#include <memory>
#include <vector>
#include "Decoder.h"

using namespace std;
class MessageDecoder
{
private:
    std::shared_ptr<Decoder> proccessor;

public:
    MessageDecoder();
    ~MessageDecoder();

public:
    void AddDecoder(shared_ptr<Decoder> &p)
    {
        proccessor = p;
    }

    bool proccess(size_t fd,void *d, size_t l);
};

MessageDecoder::MessageDecoder() : proccessor(nullptr)
{
}

MessageDecoder::~MessageDecoder()
{
}

bool MessageDecoder::proccess(size_t fd,void *d, size_t l){
    proccessor->ProccessData(fd,d,l);
}
