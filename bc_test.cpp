#include <cstdlib>
#include <iostream>
#include <list>
#include "Blockchain.h"

using namespace std;

int main(){

    Transaction txn1(1,2,25);
    Transaction txn2(2,3,60);
    Transaction txn3(4,5,90);

    cout<<"Test: print_Transaction(): "<<endl;
    txn1.print_Transaction();
    txn2.print_Transaction();
    txn3.print_Transaction();

    list<Transaction> l;
    l.push_back(txn1);
    l.push_back(txn2);
    l.push_back(txn3);

    Block b1(l);
    Block b2(b1);

    cout<<"Test: print_Block(): "<<endl;
    b1.print_block();
    b2.print_block();

    Blockchain bc;
    bc.add_block(b1);
    bc.add_block(b2);
    cout<<"Test: print_block_chain(): "<<endl;
    bc.print_block_chain();
    
    return 0;
}

