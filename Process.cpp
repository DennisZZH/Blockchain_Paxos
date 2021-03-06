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

int balance = 100;
int pid;
int seq_num = 0;
Blockchain bc;

Ballot ballot_num, accept_num;
Block accept_blo, my_blo;
bool isPrepare = false;
std::vector<WireMessage> proms;
bool isSendBack = false;
bool isAccepted = false;
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

            WireMessage m;
            m.set_type(1);
            Prepare *p = m.mutable_prepare();
            Ballot *b = p->mutable_b_num();
            b->set_depth(ballot_num.depth());
            b->set_proc_id(ballot_num.proc_id());
            b->set_seq_n(ballot_num.seq_n());

            pthread_mutex_lock(&e_lock);
            events.push_back(m);
            pthread_mutex_unlock(&e_lock);

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
        m.Clear();
        bzero(buf, sizeof(buf));
        bzero(&str_WireMessage, sizeof(str_WireMessage));
        int len = sizeof(recvaddr);
        read_size = recvfrom(sockfd, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *)&recvaddr, (socklen_t *)&len);
        if (read_size < 0)
        {
            std::cerr << "Error: Failed to receive message from P"
                      << "\n";
            exit(0);
        }
        str_WireMessage.append(buf);

        m.ParseFromString(str_WireMessage);
        // std::cout << "From receiving: " << m.DebugString();

        // Push to messages with lock/unlock
        pthread_mutex_lock(&e_lock);
        events.push_back(m);
        pthread_mutex_unlock(&e_lock);
    }
}

bool compare_ballot(Ballot b1, Ballot b2)
{
    if (b1.depth() >= b2.depth())
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

std::vector<Transaction> Str_to_txns(std::string all_str)
{
    std::vector<Transaction> result;
    char copy[all_str.length() + 1];
    strcpy(copy, all_str.c_str());
    std::string txn_str;
    int sid, rid, amt;
    char *token = strtok(copy, ";");
    while (token != NULL)
    {
        txn_str = std::string(token);
        sid = txn_str[0] - '0';
        rid = txn_str[1] - '0';
        amt = stoi(txn_str.substr(2, txn_str.length() - 2));
        result.push_back(Transaction(sid, rid, amt));
        token = strtok(NULL, ";");
    }
    return result;
}

std::string Txns_to_str(std::vector<Transaction> txns)
{
    std::string txn_str = "", all_str = "";
    for (int i = 0; i < txns.size(); i++)
    {
        txn_str += std::to_string(txns[i].get_sid());
        txn_str += std::to_string(txns[i].get_rid());
        txn_str += std::to_string(txns[i].get_amt());
        txn_str += ";";
        all_str += txn_str;
        txn_str = "";
    }
    return all_str;
}

// Set update to true if you want to update the receiver balance
Block to_block(MsgBlock src, bool update)
{
    std::vector<Transaction> trans_list;
    trans_list = Str_to_txns(src.tranxs());
    if (update)
    {
        for (int i = 0; i < trans_list.size(); i++)
        {
            if (trans_list[i].get_rid() == pid)
            {
                balance += trans_list[i].get_amt();
            }
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
    std::vector<Transaction> trans_list = src.get_txns();
    std::string trans_str = Txns_to_str(trans_list);
    std::string hash = src.get_hash();
    std::string nonce = src.get_nonce();
    MsgBlock result;
    result.set_hash(hash);
    result.set_nonce(nonce);
    result.set_tranxs(trans_str);
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
        m.Clear();
        if (!events.empty())
        {
            pthread_mutex_lock(&e_lock);
            m = events.front();
            events.pop_front();
            pthread_mutex_unlock(&e_lock);

            if (m.type() == 1)
            {
                if (isAccepted)
                {
                    pthread_mutex_lock(&e_lock);
                    events.push_back(m);
                    pthread_mutex_unlock(&e_lock);
                }
                if (m.prepare().b_num().proc_id() == pid)
                {
                    if (compare_ballot(ballot_num, m.prepare().b_num()))
                        continue;

                    // Boradcast prepare message
                    std::cout << "Broadcast " << m.DebugString();
                    str_message = m.SerializeAsString();
                    ballot_num.set_proc_id(pid);
                    ballot_num.set_seq_n(seq_num);
                    ballot_num.set_depth(bc.get_num_blocks() + 1);

                    memset(&cliaddr, 0, sizeof(cliaddr));

                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = inet_addr(server_ip);
                    for (int i = 1; i <= QUORUM_SIZE; i++)
                    {
                        if (i != pid)
                        {
                            std::cout << "Sending to P" << i << "\n";
                            cliaddr.sin_port = htons(port + i);
                            sleep(3);
                            sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
                        }
                    }
                    // Clear num accepted and promise
                    num_accepted = 0;
                    num_promise = 0;
                    // SendBack in case
                    SendBack.Clear();
                    SendBack.set_type(1);
                    myPrepare = SendBack.mutable_prepare();
                    myBallot = myPrepare->mutable_b_num();
                    myBallot->set_seq_n(++seq_num);
                    myBallot->set_depth(m.prepare().b_num().depth() + 1);
                    myBallot->set_proc_id(pid);
                }
                else if (compare_ballot(m.prepare().b_num(), ballot_num))
                {
                    std::cout << "Received " << m.DebugString();
                    ballot_num = m.prepare().b_num();
                    // Send promise back
                    response.Clear();
                    response.set_type(2);
                    myPromise = response.mutable_promise();
                    // Set acceptedBlock
                    myBlock = myPromise->mutable_ablock();
                    myBlock->CopyFrom(to_message(accept_blo));
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
                    sleep(3);
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
            // Upon receiving promise
            else if (m.type() == 2)
            {
                if (m.promise().b_num().depth() == ballot_num.depth() &&
                    m.promise().b_num().seq_n() == ballot_num.seq_n() &&
                    m.promise().b_num().proc_id() == ballot_num.proc_id())
                {
                    std::cout << "Received " << m.DebugString();
                    if (num_promise < QUORUM_MAJORITY)
                    {
                        num_promise++;
                        std::string txn = m.promise().ablock().tranxs();
                        if (txn.length() != 0)
                        {
                            // Push into vector if has transaction
                            proms.push_back(m);
                        }
                    }
                    else
                    {
                        accept_num = ballot_num;
                        if (proms.size() == 0)
                        {
                            // Send its block
                            pthread_mutex_lock(&q_lock);
                            my_blo = Block(queue);
                            while (!queue.empty())
                                queue.pop_front();
                            pthread_mutex_unlock(&q_lock);
                            isPrepare = false;
                        }
                        else
                        {
                            // Send other's block
                            my_blo = find_blo_with_highest_b(proms);
                            isSendBack = true;
                        }
                        // Broadcast accept message
                        response.Clear();
                        response.set_type(3);
                        myAccept = response.mutable_accept();
                        myBlock = myAccept->mutable_block();
                        myBlock->CopyFrom(to_message(my_blo));
                        myBallot = myAccept->mutable_b_num();
                        myBallot->CopyFrom(ballot_num);
                        myAccept->set_pid(pid);
                        std::cout << "Broadcast " << response.DebugString();
                        str_message = response.SerializeAsString();

                        memset(&cliaddr, 0, sizeof(cliaddr));

                        cliaddr.sin_family = AF_INET;
                        cliaddr.sin_addr.s_addr = inet_addr(server_ip);
                        for (int i = 1; i <= QUORUM_SIZE; i++)
                        {
                            if (i != pid)
                            {
                                std::cout << "Sending to P" << i << "\n";
                                cliaddr.sin_port = htons(port + i);
                                sleep(3);
                                sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (const sockaddr *)&cliaddr, sizeof(cliaddr));
                            }
                        }

                        num_promise = 0;
                        proms.clear();
                        isAccepted = true;
                    }
                }
            }
            // Upon receiving accept
            else if (m.type() == 3)
            {
                if (compare_ballot(m.accept().b_num(), ballot_num) || (m.accept().b_num().depth() == ballot_num.depth() && m.accept().b_num().seq_n() == ballot_num.seq_n() && m.accept().b_num().proc_id() == ballot_num.proc_id()))
                {
                    std::cout << "Received " << m.DebugString();
                    accept_num = m.accept().b_num();
                    accept_blo = to_block(m.accept().block(), false);
                    // Send accepted back
                    response.Clear();
                    response.set_type(4);
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
                    bzero(&str_message, sizeof(str_message));
                    str_message = response.SerializeAsString();

                    std::cout << "Send " << response.DebugString();

                    sleep(3);
                    sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (const sockaddr *)&cliaddr, sizeof(cliaddr));

                    isAccepted = true;
                }
            }
            // Upon receiving accepted
            else if (m.type() == 4)
            {
                if (m.accepted().b_num().depth() == ballot_num.depth() &&
                    m.accepted().b_num().seq_n() == ballot_num.seq_n() &&
                    m.accepted().b_num().proc_id() == ballot_num.proc_id())
                {
                    std::cout << "Received " << m.DebugString();
                    num_accepted++;
                    if (num_accepted >= QUORUM_MAJORITY)
                    {
                        // Broadcast decide message
                        response.Clear();
                        response.set_type(5);
                        myDecide = response.mutable_decide();
                        myBlock = myDecide->mutable_block();
                        myBlock->CopyFrom(m.accepted().block());
                        myBallot = myDecide->mutable_b_num();
                        myBallot->CopyFrom(m.accepted().b_num());

                        memset(&cliaddr, 0, sizeof(cliaddr));

                        newBlock = to_block(m.accepted().block(), true);
                        bc.add_block(newBlock);

                        str_message = response.SerializeAsString();

                        std::cout << "Broadcast " << response.DebugString();

                        cliaddr.sin_family = AF_INET;
                        cliaddr.sin_addr.s_addr = inet_addr(server_ip);
                        for (int i = 1; i <= QUORUM_SIZE; i++)
                        {
                            if (i != pid)
                            {
                                std::cout << "Sending to P" << i << "\n";
                                cliaddr.sin_port = htons(port + i);
                                sleep(3);
                                sendto(sockfd, str_message.c_str(), sizeof(WireMessage), 0, (const sockaddr *)&cliaddr, sizeof(cliaddr));
                            }
                        }

                        num_accepted = 0;
                        isAccepted = false;

                        if (isSendBack)
                        {
                            events.push_back(SendBack);
                            isSendBack = false;
                        }
                    }
                }
            }
            else if (m.type() == 5)
            {
                // Extract all the transactions and update the balance if it is needed
                std::cout << "Received " << m.DebugString();
                newBlock = to_block(m.decide().block(), true);
                bc.add_block(newBlock);

                if (isSendBack)
                {
                    events.push_back(SendBack);
                    isSendBack = false;
                }
                accept_blo = Block();
                accept_num.Clear();
                isAccepted = false;
            }

            else
            {
                std::cout << "ERROR: Wrong message type!" << std::endl;
                exit(0);
            }
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