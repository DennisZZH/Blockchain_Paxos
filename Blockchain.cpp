#include "Blockchain.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include <openssl/sha.h>


void Transaction::print_Transaction(){
    std::cout<<"Print Transation: "<<"P"<<sid<<" send $"<<amt<<" To P"<<rid<<endl;
}

std::string Transaction::serialize_transaction(){
	return std::to_string(sid) + std::to_string(rid) + std::to_string(amt);
}

std::string Block::find_hash(){
    std::string allInfo = "";
	for(int i = 0; i < txns.size(); i++){
		allInfo += txns[i].serialize_transaction();
	}
	allInfo += nonce;
	allInfo += hash;
	return sha256(str);
}

std::string Block::find_nonce(){
    std::string tempNounce;
	std::string allInfo = "";
	std::string hashInfo;
	do{
		srand(time(NULL));
		tempNounce = string(1, char(rand()%26 + 97));
		for(int i = 0; i < txns.size(); i++){
			allInfo += txns[i].serialize_transaction();
		}
		allInfo += tempNounce;
		hashInfo = sha256(allInfo);
	}while( (*hashInfo.rbegin() != '0') && (*hashInfo.rbegin() != '1') && (*hashInfo.rbegin() != '2') && (*hashInfo.rbegin() != '3') && (*hashInfo.rbegin() != '4'));

	return tempNounce;
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
	std::cout<<"Print Block: "<<std::endl;
	for(int i = 0; i < txns.size(); i++){
		std::cout<<"	";
		txns[i].print_Transaction();
	}
	std::cout<<"Hash = "<<hash<<"; Nonce = "<<nonce<<std::endl;
}


Blockchain::~Blockchain(){
	for(auto i = curr; i != head;){
		auto prevp = i->get_prev();
		delete i;
		i = prevp;
	}
	delete head;
}

void Blockchain::add_block(Block blo){
	if(num_blocks == 0){
		head = new Block(blo);
		curr = head;
		num_blocks++;
	}else{
		Block* newblo = new Block(blo);
		newblo->set_prev(curr);
		newblo->set_hash(curr->find_hash());
		curr = newblo;
		num_blocks++;
	}

	cout<<"-------One Block has been added-------"<<endl;
	curr->print_block();
	cout<<"The previous Block's hash value is "<<newblo.get_hash()<<endl;
}

Block* Blockchain::currBlock() {
	return curr;
}

void Blockchain::find_block(){

}

void Blockchain::print_block_chain(){
	std::cout<<"Print Block Chain: "<<std::endl;
    for(auto i = curr; i != head; i = i->get_prev()){
		std::cout<<"	";
		i->print_block();
	}
}