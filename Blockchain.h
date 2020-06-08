 #ifndef BLOCKCHAIN_H
 #define BLOCKCHAIN_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <list>


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
        Block(){
            prev = NULL;
            nonce = "";
            hash = "";
            h = "";
        };
        Block(std::list<Transaction> trans_list){
            prev = NULL;
            while(!trans_list.empty()){
                txns.push_back(trans_list.front());
                trans_list.pop_front();
            }
            nonce = find_nonce();
            hash = "";
        };
        Block(const Block &blo){
            prev = blo.prev;
            txns = blo.txns;
            nonce = blo.nonce;
            hash = blo.hash;
            h = blo.h;
        };

        void set_prev(Block* ptr){prev = ptr;};
        void set_hash(std::string hashcode){hash = hashcode;};

        Block* get_prev(){return prev;};
        std::vector<Transaction> get_txns(){return txns;};
        std::string get_hash(){return hash;};
        std::string get_nonce(){return nonce;};
        std::string get_h(){return h;};

        static std::string sha256(const std::string str);

        std::string find_hash();

        void print_block();

    private:
        Block* prev;
        std::vector<Transaction> txns;
        std::string nonce;
        std::string hash;
        std::string h;

        std::string find_nonce();
};

class Blockchain{
    public:
        Blockchain():head(NULL),curr(NULL),num_blocks(0){}
        ~Blockchain();

        void add_block(Block blo);
        void find_block();
        void print_block_chain();
        int get_num_blocks(){return num_blocks;};
        Block* get_curr(){return curr;};

    private:
        Block* head;
        Block* curr;
        int num_blocks;
};

 #endif