#include <streambuf>
#include <sstream>

using namespace std;

class streambuffer : public std::streambuf
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
