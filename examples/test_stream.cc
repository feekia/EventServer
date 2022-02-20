
#include <fstream>
#include <iostream>
#include <bitset>
#include <complex>

using namespace std;

int main() {

    {
        ofstream of("MyLog.txt"); // Creates a new file for write, if the file didn't exist

        of << "Experienceis the mother of wisdom" << endl;
        of << 234 << endl;
        of << 2.3 << endl;

        // 也可以写入STL 的数据 类型
        of << bitset<8>(14) << endl;      // 0001110
        of << complex<int>(2, 3) << endl; //  (2,3)
    }                                     // RAII

    // IO Opertation
    // formating the data < ------------ > communicating the data with external devices

    // Software Engineer Principel : Low Couping -> Reusability

    {
        ofstream of("MyLog.txt"); // Open file for write ,clear the content of the file
        ofstream o("MyLog.txt", ofstream::app); // Move the output pointer to the end of the file
        of << "Honesty is the best policy." << endl;
    }
    {
        ofstream of("MyLog.txt",ofstream::in | ofstream::out);
        of.seekp(10,ios::beg); // Move the output pointer 10 chars after begin
        of <<"12345"; // Overwriting 5 chars
        of.seekp(-5,ios::end); // Move the output pointer 5 char before end
        of <<"Nothing ventured, nothing gained." << endl;
        of.seekp(-5,ios::cur); // Move the output pointer 5 chars before current position
    }

    {
        ifstream inf("MyLog.txt");
        int i;
        inf >> i; // read on word --> failed
        // Error status: goodbit, badbit,failbit,eofbit
        inf.good();// Everything is Ok (goodbit ==1)
        inf.bad();
    }
}