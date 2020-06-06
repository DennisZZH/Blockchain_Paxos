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

#define QUORUM_MAJORITY 3

int balance = 100;
int pid;
int port = 8000;
char *server_ip = "127.0.0.1";
// int sockets[5];                  // sockets[pid - 1] should never be used
// struct sockaddr_in addresses[5]; // Addresses[pid - 1] should never be used
// int new_sockets[5];              // For accepted connection. new_sockts[pid - 1] should never be used
// int addrlen[5];
int sockfd;
struct sockaddr_in servaddr, cliaddr;
bool CONNECT[5];

std::queue<WireMessage> messages; // require lock
std::list<Transaction> queue;     // required lock
Blockchain Blockchain;

pthread_mutex_t m_lock, q_lock;

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
    // Send a prepare message with the current transactions in the queue
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
// int connection_setup()
// {
//     // Create four sockets to connect to other four processes
//     for (int i = 0; i < 5; i++)
//     {
//         if (i != pid - 1)
//         {
//             if ((sockets[i] = socket(AF_INET, SOCK_STREAM, 0)) == 0)
//             {
//                 std::cerr << "Failed creating socket for process " << i + 1 << std::endl;
//                 exit(0);
//             }
//         }
//     }
//     std::cout << "Sockets created.\n";

//     // Set all the sockets except sockts[pid - 1]
//     for (int i = 0; i < 5; i++)
//     {
//         if (i != pid - 1)
//         { // Don't use sockets[pid - 1] and addresses[pid - 1]
//             // Different processes have different listening/connecting sockets
//             // Listen: Port # = port + i
//             // Connet: Port # = port + pid - 1
//             // P1: 4 listens
//             // P2: 3 listens, 1 connect to P1
//             // P3: 2 listens, 2 connect to P1, P2
//             // P4: 1 listens, 3 connect to P1, P2, P3
//             // P5: 4 connect to P1, P2, P3, P4

//             // Set sockets if P1
//             if (pid == 1)
//             { // Ports: 8001, 8002, 8003, 8004
//                 std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = " << port + i << "\n";
//                 addresses[i].sin_family = AF_INET;
//                 addresses[i].sin_addr.s_addr = INADDR_ANY;
//                 addresses[i].sin_port = htons(port + i);
//                 if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
//                 {
//                     std::cerr << "Bind failed on " << i + 1 << " socket!\n";
//                     printf("Error number: %d\n", errno);
//                     printf("The error message is %s\n", strerror(errno));
//                     printf("Local socket connection with the server failed.\n");
//                     exit(errno);
//                 }
//                 if (listen(sockets[i], 3) != 0)
//                 {
//                     std::cerr << "Failed listening on for Process " << i + 1 << "\n";
//                 }
//                 std::cout << "Sockets are listening.\n";
//                 addrlen[i] = sizeof(addresses[i]);
//                 if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
//                 {
//                     std::cerr << "Failed accepting process " << i + 1 << "\n";
//                     printf("Error number: %d\n", errno);
//                     printf("The error message is %s\n", strerror(errno));
//                     printf("Local socket connection with the server failed.\n");
//                     exit(errno);
//                 }
//                 std::cout << "Process " << i + 1 << " connected.\n";
//                 CONNECT[i] = true;
//             }

//             // Set sockets if P2
//             else if (pid == 2)
//             {
//                 if (i == 0)
//                 { // Connect to P1 (8001)
//                     std::cout << "Connecting to P1...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8001);
//                     if (connect(sockets[0], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Connected!\n";
//                     CONNECT[i] = true;
//                 }
//                 else
//                 { // Set other sockets for P3 (2), P4 (3), P5 (4)
//                     std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = " << 8003 + i << "\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = INADDR_ANY;
//                     addresses[i].sin_port = htons(8003 + i); // Ports: 8005, 8006, 8007
//                     if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
//                     {
//                         std::cerr << "Bind failed on " << i + 1 << " socket!\n";
//                         exit(0);
//                     }
//                     if (listen(sockets[i], 3) != 0)
//                     {
//                         std::cerr << "Failed listening on for Process " << i + 1 << "\n";
//                     }
//                     std::cout << "Sockets are listening.\n";
//                     addrlen[i] = sizeof(addresses[i]);
//                     if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
//                     {
//                         std::cerr << "Failed accepting process " << i + 1 << "\n";
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Local socket connection with the server failed.\n");
//                         exit(errno);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//             }

//             // Set sockets if P3
//             else if (pid == 3)
//             {
//                 if (i == 0)
//                 {
//                     // Connect to P1 (8002)
//                     std::cout << "Connecting to P1...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8002);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Connected!\n";
//                     CONNECT[i] = true;
//                 }
//                 else if (i == 1)
//                 {
//                     // Connect to P2 (8005)
//                     std::cout << "Connecting to P2...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8005);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Connected!\n";
//                     CONNECT[i] = true;
//                 }
//                 else
//                 { // Set sockets for P4 (3), P5 (4)
//                     std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = " << 8005 + i << "\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = INADDR_ANY;
//                     addresses[i].sin_port = htons(8005 + i); // Ports: 8008, 8009
//                     if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
//                     {
//                         std::cerr << "Bind failed on " << i + 1 << " socket!\n";
//                         exit(0);
//                     }
//                     if ((listen(sockets[i], 3)) != 0)
//                     {
//                         std::cerr << "Failed listening on for Process " << i + 1 << "\n";
//                     }
//                     std::cout << "Sockets are listening.\n";
//                     addrlen[i] = sizeof(addresses[i]);
//                     if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
//                     {
//                         std::cerr << "Failed accepting process " << i + 1 << "\n";
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Local socket connection with the server failed.\n");
//                         exit(errno);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//             }

//             // Set sockets if P4
//             else if (pid == 4)
//             {
//                 if (i == 0)
//                 { // Connect to P1 (8003)
//                     std::cout << "Connecting to P1...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8003);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//                 else if (i == 1)
//                 { // Connect to P2 (8006)
//                     std::cout << "Connecting to P2...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8006);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//                 else if (i == 2)
//                 { // Connect to P3 (8008)
//                     std::cout << "Connecting to P3...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8008);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//                 else
//                 { // Set socket for P5 (8010)
//                     std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = 8010\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8010);
//                     if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
//                     {
//                         std::cerr << "Failed binding socket for Process " << i + 1 << "\n";
//                         exit(0);
//                     }
//                     if (listen(sockets[i], 3) != 0)
//                     {
//                         std::cerr << "Failed listening for Process " << i + 1 << "\n";
//                         exit(0);
//                     }
//                     std::cout << "Sockets are listening.\n";
//                     addrlen[i] = sizeof(addresses[i]);
//                     if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
//                     {
//                         std::cerr << "Failed accepting process " << i + 1 << "\n";
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Local socket connection with the server failed.\n");
//                         exit(errno);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//             }

//             // Set sockets if P5
//             else if (pid == 5)
//             {
//                 if (i == 0)
//                 { // Connect to P1 (8004)
//                     std::cout << "Connecting to P1...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8004);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//                 else if (i == 1)
//                 { // Connect to P2 (8007)
//                     std::cout << "Connecting to P2...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8007);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//                 else if (i == 2)
//                 { // Connect to P3 (8009)
//                     std::cout << "Connecting to P3...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8009);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//                 else if (i == 3)
//                 { // Connect to P4 (8010)
//                     std::cout << "Connecting to P4...\n";
//                     addresses[i].sin_family = AF_INET;
//                     addresses[i].sin_addr.s_addr = inet_addr(server_ip);
//                     addresses[i].sin_port = htons(8010);
//                     if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
//                     {
//                         printf("Error number: %d\n", errno);
//                         printf("The error message is %s\n", strerror(errno));
//                         printf("Connection with the server failed.\n");
//                         exit(0);
//                     }
//                     std::cout << "Process " << i + 1 << " connected.\n";
//                     CONNECT[i] = true;
//                 }
//             }
//         }
//     }
// }

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
        pthread_mutex_lock(m_lock);
        messages.push(m);
        pthread_mutex_unlock(m_lock);
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
    pthread_mutex_init(q_lock);
    pthread_mutex_init(m_lock);

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