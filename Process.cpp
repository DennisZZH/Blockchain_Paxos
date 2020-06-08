// Shiheng Wang
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
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
Block accept_blo, my_blo;
bool isPrepare = false;
std::vector<WireMessage> proms;
bool isSendBack = false;
WireMessage SendBack;

std::list<WireMessage> events; // require lock
std::list<Transaction> queue;  // required lock

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

            std::cout << "000\n";
            WireMessage m;
            Prepare *p = m.mutable_prepare();
            Ballot *b = p->mutable_b_num();
            std::cout << "111\n";
            b->set_depth(ballot_num.depth());
            b->set_proc_id(ballot_num.proc_id());
            b->set_seq_n(ballot_num.seq_n());
            std::cout << "222\n";
            std::cout << "333\n";
            std::cout << events.size() << "\n";

            pthread_mutex_lock(&e_lock);
            events.push_back(m);
            pthread_mutex_unlock(&e_lock);
            std::cout << events.size() << "\n";
            std::cout << "444\n";

            isPrepare = true;
        }
        else
        {
            std::cout << "Insufficient balance!\n";
        }
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
void connection_setup()
{
    std::cout << "The port of the current process is " << port + pid << "\n";
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed!\n";
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port + pid);
    servaddr.sin_addr.s_addr = inet_addr(server_ip);

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "Socket bind failed!\n";
        exit(0);
    }

    std::cout << "Socket created.\n";
}

// Function to receive messages and push it into the task queue
void *receiving(void *arg)
{
    int read_size = 0;
    char buf[sizeof(WireMessage)];
    std::string str_WireMessage;
    WireMessage m;
    struct sockaddr_in recvaddr;
    memset(&recvaddr, 0, sizeof(recvaddr));
    while (true)
    {
        int len = sizeof(recvaddr);
        read_size = recvfrom(sockfd, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *)&recvaddr, (socklen_t *)&len);
        if (read_size < 0)
        {
            std::cerr << "Error: Failed to receive message from P"
                      << "\n";
            exit(0);
        }
        str_WireMessage.append(buf);
        bzero(buf, sizeof(buf));

        m.ParseFromString(str_WireMessage);

        // Push to messages with lock/unlock
        pthread_mutex_lock(&e_lock);
        events.push_back(m);
        pthread_mutex_unlock(&e_lock);
    }
}

bool compare_ballot(Ballot b1, Ballot b2)
{
    if (b1.depth() > b2.depth())
    {
        if (b1.seq_n() > b2.seq_n())
            return true;
        else if (b1.seq_n() == b2.seq_n())
        {
            if (b1.proc_id() > b2.proc_id())
                return true;
        }
    }
    return false;
}

// Set update to true if you want to update the receiver balance
Block to_block(MsgBlock src, bool update)
{
    std::list<Transaction> trans_list;
    for (int i = 0; i < src.tranxs_size(); i++)
    {
        int sender = src.tranxs().at(i).sender();
        int receiver = src.tranxs().at(i).receiver();
        int amount = src.tranxs().at(i).amount();
        Transaction newTrans(sender, receiver, amount);
        trans_list.push_back(newTrans);
        if (update && receiver == pid)
        {
            balance += amount;
        }
    }
    Block result(trans_list);
    return result;
}

Block find_blo_with_highest_b(std::vector<WireMessage> proms)
{
    WireMessage maxb = proms[0];
    for (int i = 0; i < proms.size() - 1; i++)
    {
        if (compare_ballot(proms[i + 1].promise().ab_num(), maxb.promise().ab_num()))
        {
            // Update maxb
            maxb = proms[i];
        }
    }
    Block result = to_block(maxb.promise().ablock(), false);
    return result;
}

MsgBlock to_message(Block src)
{
    std::vector<Transaction> trans_list;
    trans_list = src.get_txns();
    std::string hash = src.get_hash();
    std::string nonce = src.get_nonce();
    MsgBlock result;
    result.set_hash(hash);
    result.set_nonce(nonce);
    for (int i = 0; i < trans_list.size(); i++)
    {
        Txn *newTxn = result.add_tranxs();
        newTxn->set_sender(trans_list[i].get_sid());
        newTxn->set_receiver(trans_list[i].get_rid());
        newTxn->set_amount(trans_list[i].get_amt());
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

    Prepare *myPrepare;
    Promise *myPromise;
    Accept *myAccept;
    Accepted *myAccepted;
    Decide *myDecide;
    Ballot *myBallot;
    MsgBlock *myBlock;

    while (true)
    {
        if (!events.empty())
        {
            std::cout << "555\n";
            pthread_mutex_lock(&e_lock);
            m = events.front();
            events.pop_front();
            pthread_mutex_unlock(&e_lock);
            std::cout << events.size() << "\n";
            std::cout << "666\n";

            if (m.has_prepare())
            {
                std::cout << "777\n";
                if (m.prepare().b_num().proc_id() == pid)
                {
                    if (compare_ballot(ballot_num, m.prepare().b_num()))
                        continue;

                    // Boradcast prepare message
                    std::cout << "Broadcast " << m.DebugString();
                    str_message = m.SerializeAsString();

                    memset(&cliaddr, 0, sizeof(cliaddr));

                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = inet_addr(server_ip);
                    for (int i = 1; i <= QUORUM_SIZE; i++)
                    {
                        if (i != pid)
                        {
                            std::cout << "Sending to P" << i << "\n";
                            cliaddr.sin_port = htons(port + i);
                            sleep(2);
                            sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
                        }
                    }
                    // Clear num accepted and promise
                    num_accepted = 0;
                    num_promise = 0;
                    // SendBack in case
                    SendBack.Clear();
                    myPrepare = SendBack.mutable_prepare();
                    myBallot = myPrepare->mutable_b_num();
                    myBallot->set_seq_n(++seq_num);
                    myBallot->set_depth(m.prepare().b_num().depth() + 1);
                }
                else if (compare_ballot(m.prepare().b_num(), ballot_num))
                {
                    std::cout << "Received " << m.DebugString();
                    ballot_num = m.prepare().b_num();
                    // Send promise back
                    response.Clear();
                    myPromise = response.mutable_promise();
                    // Set acceptedBlock
                    myBlock = myPromise->mutable_ablock();
                    *myBlock = to_message(accept_blo);
                    // Set leader's ballot
                    myBallot = myPromise->mutable_b_num();
                    myBallot->set_seq_n(ballot_num.seq_n());
                    myBallot->set_depth(ballot_num.depth());
                    myBallot->set_proc_id(ballot_num.proc_id());
                    // Set acceptedBal
                    myBallot = myPromise->mutable_ab_num();
                    myBallot->set_proc_id(accept_num.proc_id());
                    myBallot->set_seq_n(accept_num.seq_n());
                    myBallot->set_depth(accept_num.depth());

                    memset(&cliaddr, 0, sizeof(cliaddr));
                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = inet_addr(server_ip);
                    cliaddr.sin_port = htons(port + m.prepare().b_num().proc_id());
                    str_message = response.SerializeAsString();

                    int len = sizeof(cliaddr);
                    sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (const sockaddr *)&cliaddr, len);
                    std::cout << "Send " << response.DebugString();

                    if (isPrepare)
                    {
                        isSendBack = true;
                        num_promise = 0;
                        num_accepted = 0;
                    }
                }
            }
            else if (m.has_promise())
            {
                if (m.promise().b_num().depth() == ballot_num.depth() &&
                    m.promise().b_num().seq_n() == ballot_num.seq_n() &&
                    m.promise().b_num().proc_id() == ballot_num.proc_id())
                {
                    std::cout << "Received " << m.DebugString();
                    if (num_promise < QUORUM_MAJORITY)
                    {
                        num_promise++;
                        proms.push_back(m);
                    }
                    else
                    {
                        accept_num = ballot_num;
                        if (m.promise().ablock().tranxs().size() == 0)
                        {
                            pthread_mutex_lock(&q_lock);
                            my_blo = Block(queue);
                            while (!queue.empty())
                                queue.pop_front();
                            pthread_mutex_unlock(&q_lock);
                            isPrepare = false;
                        }
                        else
                        {
                            my_blo = find_blo_with_highest_b(proms);
                            isSendBack = true;
                        }
                        // Broadcast accept message
                        response.Clear();
                        myAccept = response.mutable_accept();
                        myBlock = myAccept->mutable_block();
                        myBlock->CopyFrom(to_message(my_blo));
                        myBallot = myAccept->mutable_b_num();
                        myBallot->CopyFrom(ballot_num);
                        myAccept->set_pid(pid);
                        str_message = response.SerializeAsString();

                        std::cout << "Broadcast " << response.DebugString();

                        memset(&cliaddr, 0, sizeof(cliaddr));

                        cliaddr.sin_family = AF_INET;
                        cliaddr.sin_addr.s_addr = inet_addr(server_ip);
                        for (int i = 1; i <= QUORUM_SIZE; i++)
                        {
                            if (i != pid)
                            {
                                std::cout << "Sending to P" << i << "\n";
                                cliaddr.sin_port = htons(port + i);
                                sleep(2);
                                sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (const sockaddr *)&cliaddr, sizeof(cliaddr));
                            }
                        }

                        num_promise = 0;
                        proms.clear();
                    }
                }
            }
            else if (m.has_accept())
            {
                if (compare_ballot(m.accept().b_num(), ballot_num) || (m.accept().b_num().depth() == ballot_num.depth() && m.accept().b_num().seq_n() == ballot_num.seq_n() && m.accept().b_num().proc_id() == ballot_num.proc_id()))
                {
                    std::cout << "Received " << m.DebugString();
                    accept_num = m.accept().b_num();
                    accept_blo = to_block(m.accept().block(), false);
                    // Send accepted back
                    response.Clear();
                    myAccepted = response.mutable_accepted();
                    myBallot = myAccepted->mutable_b_num();
                    myBallot->CopyFrom(accept_num);
                    myBlock = myAccepted->mutable_block();
                    myBlock->CopyFrom(to_message(accept_blo));

                    memset(&cliaddr, 0, sizeof(cliaddr));
                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = inet_addr(server_ip);
                    std::cout << "Sending to P" << m.accept().pid() << "\n";
                    cliaddr.sin_port = htons(port + m.accept().pid());
                    str_message = response.SerializeAsString();

                    std::cout << "Send " << response.DebugString();

                    sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (const sockaddr *)&cliaddr, sizeof(cliaddr));
                }
            }
            // else if (m.has_accepted())
            // {
            //     if (m.accepted().b_num().depth() == ballot_num.depth() &&
            //         m.accepted().b_num().seq_n() == ballot_num.seq_n() &&
            //         m.accepted().b_num().proc_id() == ballot_num.proc_id())
            //     {
            //         std::cout << "Received " << m.DebugString();
            //         num_accepted++;
            //         if (num_accepted >= QUORUM_MAJORITY)
            //         {
            //             // Broadcast decide message
            //             response.Clear();
            //             myDecide = response.mutable_decide();
            //             myBlock = myDecide->mutable_block();
            //             myBlock->CopyFrom(m.accepted().block());
            //             myBallot = myDecide->mutable_b_num();
            //             myBallot->CopyFrom(m.accepted().b_num());

            //             memset(&cliaddr, 0, sizeof(cliaddr));

            //             str_message = response.SerializeAsString();

            //             std::cout << "Broadcast " << response.DebugString();

            //             cliaddr.sin_family = AF_INET;
            //             cliaddr.sin_addr.s_addr = inet_addr(server_ip);
            //             for (int i = 0; i < QUORUM_SIZE; i++)
            //             {
            //                 if (i == (pid - 1))
            //                     continue;
            //                 cliaddr.sin_port = htons(port + i + 1);
            //                 int len = sizeof(cliaddr);
            //                 sleep(2);
            //                 sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (const sockaddr*)&cliaddr, len);
            //             }

            //             num_accepted = 0;

            //             if (isSendBack)
            //             {
            //                 events.push_back(SendBack);
            //                 isSendBack = false;
            //             }
            //         }
            //     }
            // }
            // else if (m.has_decide())
            // {
            //     // Extract all the transactions and update the balance if it is needed
            //     std::cout << "Received " << m.DebugString();
            //     newBlock = to_block(m.decide().block(), true);
            //     bc.add_block(newBlock);

            //     if (isSendBack)
            //     {
            //         events.push_back(SendBack);
            //         isSendBack = false;
            //     }
            // }

            // else
            // {
            //     std::cout << "ERROR: Wrong message type!" << std::endl;
            //     exit(0);
            // }
        }
    }
}

// Main function
int main()
{
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
    pthread_mutex_init(&q_lock, NULL);
    pthread_mutex_init(&e_lock, NULL);

    connection_setup();

    // STUB: open Paxos thread
    pthread_t comm, proc;
    pthread_create(&comm, NULL, receiving, (void *)&pid);
    pthread_create(&proc, NULL, process, NULL);
    int job;

    while (true)
    {
        std::cout << "What do you want to do?\n\t(0) Quit the session\n\t(1) New Money Transfer\n\t(2) Print blockchain\n\t(3) Print the balance\n\t(4) Print the pending transactions(queue)\n";
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

        // Print blockchain
        else if (job == 2)
        {
            printBlockchain();
        }

        // Print balance
        else if (job == 3)
        {
            printBalance();
        }

        // Print queue
        else if (job == 4)
        {
            printQueue();
        }
        else
        {
            std::cout << "Undefined job #\n";
        }
    }

    return 0;
}