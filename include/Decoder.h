#include <stddef.h>

class Decoder
{
private:
    /* data */
public:
    Decoder(/* args */);
    virtual ~Decoder();
    void ProccessData(size_t fd, void* d, size_t l);
};
