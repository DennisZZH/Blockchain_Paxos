#include <cstdlib>
#include <iostream>
#include <list>
#include "Blockchain.h"

using namespace std;

int main(){

    Transaction txn1(1,2,25);
    Transaction txn2(2,3,60);
    Transaction txn3(4,5,90);
    Transaction txn4(4,2,110);
    Transaction txn5(1,5,20);
    Transaction txn6(3,4,290);
    Transaction txn7(4,2,45);

    cout<<"Test: print_Transaction(): "<<endl;
    txn1.print_Transaction();
    txn2.print_Transaction();
    txn3.print_Transaction();
     txn4.print_Transaction();
      txn5.print_Transaction();
       txn6.print_Transaction();
        txn7.print_Transaction();
    cout<<endl;

    list<Transaction> l1,l2,l3;
    l1.push_back(txn1);
    l1.push_back(txn2);
    l1.push_back(txn3);

    l2.push_back(txn4);
     l2.push_back(txn5);

      l3.push_back(txn6);
     l3.push_back(txn7);


    Block b1(l1);
    Block b2(l2);
    Block b3(l3);

    cout<<"Test: print_Block(): "<<endl;
    b1.print_block();
    b2.print_block();
    b3.print_block();
    cout<<endl;

    Blockchain bc;
    bc.add_block(b1);
    bc.add_block(b2);
    bc.add_block(b3);
    cout<<"Test: print_block_chain(): "<<endl;
    bc.print_block_chain();

    return 0;
}

