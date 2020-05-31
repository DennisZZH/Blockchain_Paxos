 #ifndef BLOCKCHAIN_H
 #define BLOCKCHAIN_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace std;


class Transaction{
    public:
        Transaction();
        Transaction(int sender_id, int receiver_id, int amount): sid(sender_id), rid(receiver_id), amt(amount){}
        void print_Transaction();
    private:
        int sid;
        int rid;
        int amt;
};

class Block{
    public:
        Block();
        Block(vector<Transaction> Transactions): txns(Transactions){}
        ~Block();

        void set_prev(Block* ptr){prev = ptr;};
        void set_hash(string hashcode){hash = hashcode;};

        Block* get_prev(){return prev;};
        vector<Transaction> get_txns(){return txns;};
        string get_hash(){return hash;};
        string get_nonce(){return nonce;};

        static string sha256(const string str);

        string find_hash();

        void print_block();

    private:
        Block* prev;
        vector<Transaction> txns;
        string nonce;
        string hash;

        string find_nonce();
};

class Blockchain{
    public:
        Blockchain():head(NULL),curr(NULL),num_blocks(0){}
        ~Blockchain();

        void add_block(Block blo);
        void find_block();
        bool verify_and_print_block();

    private:
        Block* head;
        Block* curr;
        int num_blocks;
};

 #endif