 #ifndef BLOCKCHAIN_H
 #define BLOCKCHAIN_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>


class Transaction{
    public:
        Transaction();
        Transaction(int sender_id, int receiver_id, int amount): sid(sender_id), rid(receiver_id), amt(amount){}
        void print_Transaction();
        int get_sid(){return sid;};
        int get_rid(){return rid;};
        int get_amt(){return amt;};
        std::string serialize_transaction();
    private:
        int sid;
        int rid;
        int amt;
};

class Block{
    public:
        Block();
        Block(std::vector<Transaction> Transactions){
            prev = NULL;
            txns = Transactions;
            nonce = find_nonce();
            hash = "";
        };
        Block(Block blo){
            prev = blo.prev;
            txns = blo.txns;
            nonce = blo.nonce;
            hash = blo.hash;
        };

        void set_prev(Block* ptr){prev = ptr;};
        void set_hash(std::string hashcode){hash = hashcode;};

        Block* get_prev(){return prev;};
        std::vector<Transaction> get_txns(){return txns;};
        std::string get_hash(){return hash;};
        std::string get_nonce(){return nonce;};

        static std::string sha256(const std::string str);

        std::string find_hash();

        void print_block();

    private:
        Block* prev;
        std::vector<Transaction> txns;
        std::string nonce;
        std::string hash;

        std::string find_nonce();
};

class Blockchain{
    public:
        Blockchain():head(NULL),curr(NULL),num_blocks(0){}
        ~Blockchain();

        void add_block(Block blo);
        void find_block();
        void print_block_chain();

    private:
        Block* head;
        Block* curr;
        int num_blocks;
};

 #endif