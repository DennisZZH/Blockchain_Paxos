#include <cstdlib>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include "Blockchain.h"

using namespace std;


 std::vector<Transaction> Str_to_txns(std::string all_str){
    std::vector<Transaction> result;
    char copy[all_str.length() + 1];
    strcpy(copy, all_str.c_str());
    std::string txn_str;
    int sid, rid, amt;
    char* token = strtok(copy, ";");
    while(token != NULL){
        txn_str = std::string(token);
        sid = txn_str[0] - '0';
        rid = txn_str[1] - '0';
        amt = stoi(txn_str.substr(2, txn_str.length() - 2));
        result.push_back(Transaction(sid,rid,amt));
        token = strtok(NULL, ";");
    }   
    return result;
}

std::string Txns_to_str(std::vector<Transaction> txns){
    std::string txn_str = "", all_str = "";
    for(int i = 0; i < txns.size(); i++){
        txn_str += std::to_string(txns[i].get_sid());
        txn_str += std::to_string(txns[i].get_rid());
        txn_str += std::to_string(txns[i].get_amt());
        txn_str += ";";
        all_str += txn_str;
        txn_str = "";
    }
    return all_str;
}

int main(){
    Transaction txn1(1,2,25);
    Transaction txn2(2,3,60);
    Transaction txn3(4,5,90);
    Transaction txn4(4,2,110);
    Transaction txn5(1,5,20);
    Transaction txn6(3,4,290);
    Transaction txn7(4,2,45);

     vector<Transaction> l1,l2,l3;
    l1.push_back(txn1);
    l1.push_back(txn2);
    l1.push_back(txn3);

    string s1, s2, s3;
    s1 = Txns_to_str(l1);
    s2 = Txns_to_str(l2);
    s3 = Txns_to_str(l3);

    cout<<"s1 = "<<s1<<endl;
     cout<<"s2 = "<<s2<<endl;
      cout<<"s3 = "<<s3<<endl;
      cout<<endl;

     vector<Transaction> l4,l5,l6;
     l4 = Str_to_txns(s1);
     l5 = Str_to_txns(s2);
     l6 = Str_to_txns(s3);

     Block b1(l4);
     b1.print_block();
      Block b2(l5);
     b1.print_block();
      Block b3(l6);
     b1.print_block();

    return 0;
}