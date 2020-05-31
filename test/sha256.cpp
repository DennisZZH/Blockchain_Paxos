/*
To install OpenSSL on Debian, Ubuntu, or other derivative versions:

$ sudo apt-get install libssl-dev

To install the OpenSSL development kit on Fedora, CentOS, or RHEL:

$ sudo yum install openssl-devel

To compile cpp

$ g++ -I/usr/local/opt/openssl@1.1/include -L/usr/local/opt/openssl@1.1/lib sha256.cpp -o sha256 -lcrypto
*/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

using namespace std;

    string sha256(const string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int main() {
    cout << sha256("1234567890_1") << endl;
    cout << sha256("1234567890_2") << endl;
    cout << sha256("1234567890_3") << endl;
    cout << sha256("1234567890_4") << endl;
    return 0;
}
