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

int balance = 100;
int pid;
int port = 8000;
char *server_ip = "127.0.0.1";
int sockfd;
struct sockaddr_in servaddr, cliaddr;
bool CONNECT[5];
bool isPrepare = false;
int seq_n = 1;

std::queue<WireMessage> events; // require lock
std::list<Transaction> queue;     // required lock
Blockchain Blockchain;

pthread_mutex_t e_lock, q_lock;

// Money transfer and send a Prepare message
void moneyTransfer(int receiver, int amount)
{
    Transaction newTranx(pid, receiver, amount);

    if (balance >= amount)
    {
        queue.push_back(newTranx);
        balance -= amount;
    }
    else
    {
        std::cout << "Insufficient balance!\n";
    }
    // Check if prepare is in the queue
    if (!isPrepare && !(queue.empty())) {
        // Push prepare to events queue with current depth of queue
        int depth = queue.size();
        Ballot ballot;
        ballot.set_seq_n(seq_n);
        ballot.set_proc_id(pid);
        ballot.set_depth(depth);

        WireMessage m;
        m.prepare();
        m.prepare.set_type(1);
        m.prepare.set_b_num(ballot);
        seq_n += 1;

        // Push with lock/unlock
        pthread_mutex_lock(&e_lock);
        events.push(m);
        pthread_mutex_unlock(&e_lock);
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
    // Send a message to tell the process to set the bool
}

void failProccess()
{
    // Disconnect with all the processes
    for (int i = 0; i < 5; i++)
    {
        CONNECT[i] = false;
    }

    for (int i = 0; i < 5; i++)
    {
        failLink(i + 1);
    }

    // Restore the process
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
    std::cout << "STUB\n";
}

// Set up the connections
void connection_setup(int pid)
{
    port += pid;
    std::cout << "The port of the current process is " << port << "\n";
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed!\n";
        exit(0);
    }

    memset(&servaddr, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
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

// Function to process the event
void *process(void *arg)
{

}

// Main function
int main()
{
    int pid;
    std::cout << "Process #: ";
    std::cin >> pid;
    std::cout << "\n";

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
    for (int i = 0; i < 5; i++)
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