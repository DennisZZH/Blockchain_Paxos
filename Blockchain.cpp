#include "Blockchain.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include <openssl/sha.h>


void Transaction::print_Transaction(){
    std::cout<<"Transation: "<<"P"<<sid<<" send $"<<amt<<" To P"<<rid<<endl;
}

std::string Block::find_hash(){
    return "STUB";
}

std::string Block::find_nonce(){
    return "STUB";
}

std::string Block::sha256(const std::string str){
     unsigned char hash[SHA256_DIGEST_LENGTH];
	    SHA256_CTX sha256;
	    SHA256_Init(&sha256);
	    SHA256_Update(&sha256, str.c_str(), str.size());
	    SHA256_Final(hash, &sha256);
	    std::stringstream ss;
	    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	    {
	        ss << hex << setw(2) << setfill('0') << (int)hash[i];
	    }
	    return ss.str();
}

void Block::print_block(){

}


Blockchain::~Blockchain(){

}

void Blockchain::add_block(Block blo){

}

void Blockchain::find_block(){

}

bool Blockchain::verify_and_print_block(){
    return 0;
}