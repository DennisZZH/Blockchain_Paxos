// Shiheng Wang
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <vector>
#include <list>
#include <queue>
#include <pthread.h>
#include "Blockchain.h"
#include "Msg.pb.h"

#define QUORUM_SIZE 5
#define QUORUM_MAJORITY 3

int port = 8000;
char *server_ip = "127.0.0.1";
int sockfd;
struct sockaddr_in servaddr, cliaddr;
bool CONNECT[QUORUM_SIZE];

int balance = 100;
int pid;
int seq_num = 0;
Blockchain bc;

Ballot ballot_num, accept_num;
Block accept_blo;
bool isPrepare = false;
std::vector<Promise> proms;

std::queue<WireMessage> events; // require lock
std::list<Transaction> queue;   // required lock

pthread_mutex_t e_lock, q_lock;

// Money transfer and send a Prepare message
void moneyTransfer(int receiver, int amount)
{
    if (balance >= amount)
    {
        Transaction newTranx(pid, receiver, amount);
        queue.push_back(newTranx);
        balance -= amount;

        if (!isPrepare)
        {
            // Push prepare to events queue with current depth and seq num
            ballot_num.set_proc_id(pid);
            ballot_num.set_seq_n(++seq_num);
            ballot_num.set_depth(bc.get_num_blocks() + 1);

            WireMessage m;
            m.prepare();
            m.prepare().set_b_num(ballot_num);

            pthread_mutex_lock(&e_lock);
            events.push(m);
            pthread_mutex_unlock(&e_lock);

            isPrepare = true;
        }
        else
        {
            std::cout << "Insufficient balance!\n";
        }
    }
}

// Fail the link
void failLink(int dst)
{
    CONNECT[dst - 1] = false;
    // Send a message to tell the process to set the bool
}

// Fix the link
void fixLink(int dst)
{
    CONNECT[dst - 1] = true;
    // Push a restore message to the queue
    WireMessage m;
    m.restore();
    m.restore().set_depth(bc.get_num_blocks);
    m.restore().set_pid(pid);

    // Push with lock/unlock
    pthread_mutex_lock(&e_lock);
    events.push(m);
    pthread_mutex_unlock(&e_lock);
}

void failProccess()
{
    // Disconnect with all the processes
    for (int i = 0; i < QUORUM_SIZE; i++)
    {
        CONNECT[i] = false;
    }

    for (int i = 0; i < QUORUM_SIZE; i++)
    {
        failLink(i + 1);
    }

    // Restore the process
    std::string str;
    std::cout << "Type anything to restore the process.\n";
    std::cin >> str;
    for (int i = 0; i < QUORUM_SIZE; i++)
    {
        fixLink(i + 1);
    }
}

// Print out the current balance
void printBalance()
{
    std::cout << "Current ballance: $" << balance << "\n";
}

// Print out the transaction queue
void printQueue()
{
    if (queue.empty())
    {
        std::cout << "The queue is empty now. Do some transactions.\n";
    }
    for (auto i = queue.begin(); i != queue.end(); i++)
    {
        std::cout << "<P" << i->get_sid() + 1 << ", P" << i->get_rid() << ", " << i->get_amt() << ">\n";
    }
}

// Print out the blockchain
void printBlockchain()
{
    bc.print_block_chain();
}

// Set up the connections
void connection_setup(int pid)
{
    std::cout << "The port of the current process is " << port << "\n";
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed!\n";
        exit(0);
    }

    memset(&servaddr, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port + pid);
    servaddr.sin_addr.s_addr = server_ip;

    // Bind the socket
    if (bind(sockfd, (const sturct sockaddr *)&servaddr, sizeof(servaaddr)) < 0)
    {
        std::cerr << "Socket bind failed!\n";
        exit(0);
    }

    std::cout << "Socket created.\n";
}

// Function to receive messages and push it into the task queue
void *receiving(void *arg)
{
    // The process this thread is listening
    int *id = (int *)arg;

    int left_size = sizeof(WireMessage), read_size = 0;
    char buf[sizeof(WireMessage)];
    std::string str_WireMessage;
    WireMessage m;
    memset(&cliaddr, 0 sizeof(cliaddr));
    while (true)
    {
        while (left_size > 0)
        {
            read_size = recvfrom(sockfd, buf, sizeof(buf), &cliaddr, sizeof(cliaddr));
            if (read_size < 0)
            {
                std::cerr << "Error: Failed to receive message from P" << id << "\n";
                exit(0);
            }
            left_size -= read_size;
            str_WireMessage.append(buf);
            bzero(buf, sizeof(buf));
        }

        m.ParseFromString(str_WireMessage);

        // Push to messages with lock/unlock
        pthread_mutex_lock(&e_lock);
        events.push(m);
        pthread_mutex_unlock(&e_lock);
    }
}

bool compare_ballot(Ballot b1, Ballot b2)
{
    // return if b1 >= b2;
    // STUB
    return false;
}

Block find_blo_with_highest_b(std::vector<Promise> proms)
{
    // STUB
}

// Set update to true if you want to update the receiver balance
Block to_block(MsgBlock src, bool update) {
    std::list<Transaction> trans_list;
    for (int i = 0; i < src.trax().size(); i++) {
        int sender = src.trax().at(i).sender();
        int receiver = src.trax().at(i).receiver();
        int amount = src.trax().at(i).amount();
        Transaction newTrans(sender, receiver, amount);
        trans_list.push_back(newTrans);
        if (update && receiver == pid) {
            balance += amount;
        }
    }
    Block result(trans_list);
    return result;
}

MsgBlock to_message(Block src) {
    std::vector<Transaction> trans_list;
    trans_list = src.get_txns();
    int hash = src.get_hash();
    int nonce = src.get_nonce();
    MsgBlock result;
    result.set_hash(hash);
    result.set_nonce(nonce);
    for (int i = 0; i < trans_list.size(); i++) {
        Txn newTxn;
        newTxn.set_sender(trans_list[i].get_sid());
        newTxn.set_receiver(trans_list[i].get_rid());
        newTxn.set_amount(trans_list[i].get_amt());
        result.trax().Add(newTxn);
    }
    return result;
}

// Function to process the event
void *process(void *arg)
{
    int num_accepted = 0;
    int num_promise = 0;
    WireMessage m, response, addback;
    std::string str_message;
    char buf[sizeof(WireMessage)];
    Block newBlock;

    while(!events.empty()){

        pthread_mutex_lock(&e_lock);
        m = events.front();
        events.pop();
        pthread_mutex_unlock(&e_lock);

        if (m.has_prepare())
        {
            if (m.prepare().b_num().proc_id() == pid)
            {
                // Boradcast prepare message
                str_message = m.SerializeAsString();

                memset(&cliaddr, 0, sizeof(cliaddr));

                cliaddr.sin_family = AF_INET;
                cliaddr.sin_addr.s_addr = server_ip;
                for (int i = 0; i < QUORUM_SIZE; i++)
                {
                    if (i == (pid + 1))
                        continue;
                    cliaddr.sin_port = htons(port + i + 1);
                    int len = sizeof(cliaddr);
                    sleep(2);
                    sendto(sockfd, str_message.c_str(), sizeof(WireMessage), &cliaddr, &len);
                }
            }
            else
            {
                if (compare_ballot(m.prepare().b_num(), ballot_num))
                {
                    ballot_num = m.prepare().b_num();
                    // Send promise back
                    response.Clear();
                    response.promise();
                    response.promise().set_b_num(ballot_num);
                    response.promise().set_ab_num(accept_num);
                    response.promise().set_ablock(to_message(accept_blo));

                    memset(&cliaddr, 0, sizeof(cliaddr));
                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = server_ip;
                    cliaddr.sin_port = htons(port + m.prepare().pid());
                    str_message = response.SerializeAsString();

                    int len = sizeof(cliaddr);
                    sendto(sockfd, str_message.c_str(), sizeof(WireMessage), &cliaddr, &len);

                    if(num_promise < QUORUM_MAJORITY){
                        ballot_num.set_seq_n(++seq_num);
                        addback.Clear();
                        addback.prepare();
                        addback.prepare().set_b_num(ballot_num);
                        pthread_mutex_lock(&e_lock);
                        events.push(addback);
                        pthread_mutex_unlock(&e_lock);
                    }
                    num_promise = 0;
                    num_accepted = 0;
                }
            }
        }
        else if (m.has_promise())
        {
            if (m.promise().b_num() == ballot_num)
            {
                if (num_promise < QUORUM_MAJORITY)
                {
                    num_promise++;
                    proms.push_back(m);
                }
                else
                {
                    if (m.promise().ablock().tranxs().size() == 0)
                    {
                        pthread_mutex_lock(&q_lock);
                        accept_blo = Block(queue);
                        while(!queue.empty()) queue.pop_front();
                        pthread_mutex_unlock(&q_lock);
                        isPrepare = false;
                    }
                    else
                    {
                        accept_blo = find_blo_with_highest_b(proms);
                        
                        ballot_num.set_seq_n(++seq_num);
                        addback.Clear();
                        addback.prepare();
                        addback.prepare().set_b_num(ballot_num);
                        pthread_mutex_lock(&e_lock);
                        events.push(addback);
                        pthread_mutex_unlock(&e_lock);
                    }
                    // Broadcast accept message
                    response.Clear();
                    response.accept();
                    response.accept().set_b_num(ballot_num);
                    response.accept().set_block(to_message(accept_blo));
                    str_message = response.SerializeAsString();

                    memset(&cliaddr, 0, sizeof(cliaddr));

                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = server_ip;
                    for (int i = 0; i < QUORUM_SIZE; i++)
                    {
                        if (i == (pid + 1))
                            continue;
                        cliaddr.sin_port = htons(port + i + 1);
                        int len = sizeof(cliaddr);
                        sleep(2);
                        sendto(sockfd, str_message.c_str(), sizeof(WireMessage), &cliaddr, &len);
                    }

                    num_promise = 0;
                    proms.clear();
                }
            }
        }
        else if (m.has_accept())
        {
            if(compare_ballot(m.accept().b_num(), ballot_num)){
                accept_num = m.accept().b_num();
                accept_blo = m.accept().block();
                // Send accepted back
                response.Clear();
                response.accepted();
                response.accepted().set_b_num(accept_num);
                response.accepted().set_block(to_message(accept_blo));

                memset(&cliaddr, 0, sizeof(cliaddr));
                cliaddr.sin_family = AF_INET;
                cliaddr.sin_addr.s_addr = server_ip;
                cliaddr.sin_port = htons(port + m.accept(().pid());
                str_message = response.SerializeAsString();

                int len = sizeof(cliaddr);
                sendto(sockfd, str_message.c_str(), sizeof(WireMessage), &cliaddr, &len);
            }

        }
        else if (m.has_accpted())
        {
            if(m.accepted().b_num() == ballot_num){
                num_accepted++;
                if (num_accepted >= QUORUM_MAJORITY)
                {
                    // Broadcast decide message
                    response.Clear();
                    response.decide();
                    response.decide().set_b_num(m.accepted().b_num());
                    response.decide().set_block(m.accepted().block());
                    str_message = response.SerializeAsString();

                    memset(&cliaddr, 0, sizeof(cliaddr));

                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = server_ip;
                    for (int i = 0; i < QUORUM_SIZE; i++)
                    {
                        if (i == (pid + 1))
                            continue;
                        cliaddr.sin_port = htons(port + i + 1);
                        int len = sizeof(cliaddr);
                        sleep(2);
                        sendto(sockfd, str_message.c_str(), sizeof(WireMessage), &cliaddr, &len);
                    }

                    num_accepted = 0;
                }
            }
        }
        else if (m.has_decide())
        {
            // Extract all the transactions and update the balance if it is needed
            newBlock = to_block(m.decide().block(), true);
            bc.add_block(newBlock);
        }
        // else if (m.has_restore())
        // {
        //     if (m.restore().pid == pid)
        //     {
        //         if (m.restore().blocks().size() == 0)
        //         {
        //             // Send the message out
        //             for (int i = 0; i < QUORUM_SIZE; i++)
        //             {
        //                 if (CONNECT[i] == true)
        //                     break;
        //             }
        //             memset(&cliaddr, 0, sizeof(cliaddr));

        //             cliaddr.sin_family = AF_INET;
        //             cliaddr.sin_addr.s_addr = server_ip;
        //             cliaddr.sin_port = htons(port + i + 1);
        //             str_message = m.SerializeAsString();

        //             int len = sizeof(cliaddr);
        //             sendto(sockfd, str_message.c_str(), sizeof(WireMessage), &cliaddr, &len);
        //         }
        //         else
        //         {
        //             // Add new blocks to blockchain
        //             for (int i = m.restore().depth() - bc.get_num_blocks - 1; i >= 0; i-) {
        //                 newBlock = to_block(m.restore().blocks().at(i), true);
        //                 bc.add_block(newBlock);
        //             }
        //         }
        //     }
        //     else
        //     {
        //         // Send a copy of blocks that the sender does not have
        //         Block* curr = bc;
        //         int i = bc.get_num_blocks() - m.restore().depth();
        //         m.restore().set_depth(bc.get_num_blocks());
        //         while (i) {
        //             newBlock = to_message(*curr);
        //             m.restore().blocks().Add(newBlock);
        //         }

        //         // Send back to the sender
        //         memset(&cliaddr, 0, sizeof(cliaddr));
        //         cliaddr.sin_family = AF_INET;
        //         cliaddr.sin_addr.s_addr = server_ip;
        //         cliaddr.sin_port = htons(port + m.restore().pid());
        //         str_message = m.SerializeAsString();

        //         int len = sizeof(cliaddr);
        //         sendto(sockfd, str_message.c_str(), sizeof(WireMessage), &cliaddr, &len);
        //     }
        // }
        else
    {
        std::cout << "ERROR: Wrong message type!" << std::endl;
        exit(0);
    }
    
    }
}

// Main function
int main()
{
    int pid;
    std::cout << "Process #: ";
    std::cin >> pid;
    std::cout << "\n";

    ballot_num.set_proc_id(0);
    ballot_num.set_seq_n(0);
    ballot_num.set_depth(0);

    accept_num.set_proc_id(0);
    accept_num.set_seq_n(0);
    accept_num.set_depth(0);

    // Initialize lock
    pthread_mutex_init(&q_lock);
    pthread_mutex_init(&e_lock);

    connection_setup();

    // STUB: open Paxos thread
    pthread_t comm, proc;
    pthread_create(comm, NULL, receiving, (void *)&pid);
    pthread_create(proc, NULL, process, NULL);

    while (true)
    {
        int job;
        std::cout << "What do you want to do?\n\t(0) Quit the session\n\t(1) New Money Transfer\n\t(2) Fail a link\n\t(3) Fix a link\n\t(4) Fail the process\n\t(5) Print blockchain\n\t(6) Print the balance\n\t(7) Print the pending transactions(queue)\n";
        std::cin >> job;

        // Quit the session
        if (job == 0)
            break;

        // New Money transfer
        else if (job == 1)
        {
            int receiver, amount;
            std::cout << "Which process do you want to send?\n";
            std::cin >> receiver;
            std::cout << "How much do you want to send?\n";
            std::cin >> amount;
            moneyTransfer(receiver, amount);
        }

        // Fail a link
        else if (job == 2)
        {
            int dst;
            std::cout << "Which link you want to fail?\n";
            std::cin >> dst;
            failLink(dst);
            std::cout << "You cannot communicate with P" << dst << " now\n";
        }

        // Fix a link
        else if (job == 3)
        {
            int dst;
            std::cout << "Which link you want to fix?\n";
            std::cin >> dst;
            fixLink(dst);
            std::cout << "You can communicate with P" << dst << " now\n";
        }

        // Fail the process
        else if (job == 4)
        {
            failProccess();
        }

        // Print blockchain
        else if (job == 5)
        {
            printBlockchain();
        }

        // Print balance
        else if (job == 6)
        {
            printBalance();
        }

        // Print queue
        else if (job == 7)
        {
            printQueue();
        }
    }

    // Close the sockets
    for (int i = 0; i < QUORUM_SIZE; i++)
    {
        if (i < pid - 1)
        {
            std::cout << "Closing the socket to P" << i + 1 << "\n";
            close(sockets[i]);
        }
        if (i >= pid)
        {
            std::cout << "Closing the socket to P" << i + 1 << "\n";
            close(new_sockets[i]);
        }
    }
}