#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <stdio.h>
#include <string.h>

#include <openssl/sha.h>

using namespace std;

string sha256(const string str){
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

string findNounce(){
	int amount = 1;
	string sender = "A";
	string receiver = "B";

	string tempNounce;
	string allInfo;
	string hashInfo;


	do{
	srand(time(NULL));
	tempNounce = string(1, char(rand()%26 + 97));
	allInfo = to_string(amount) + sender + receiver + tempNounce;
	hashInfo = sha256(allInfo);
	}while( (*hashInfo.rbegin() != '0') && (*hashInfo.rbegin() != '1') && (*hashInfo.rbegin() != '2') && (*hashInfo.rbegin() != '3') && (*hashInfo.rbegin() != '4'));
	
	

	cout<<tempNounce<<endl;
	cout<<allInfo<<endl;
	cout<<hashInfo<<endl;
	
	return tempNounce;
}

int main(){
	
	//cout<<hashInfo<<endl;
	findNounce();

	return 0;
}
