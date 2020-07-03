class Decoder
{
private:
    /* data */
public:
    virtual Decoder(/* args */);
    virtual ~Decoder();
    void ProccessData(size_t fd, void* d, size_t l);
};
