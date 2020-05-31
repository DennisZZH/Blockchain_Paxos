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

int balance = 100;
int port = 8000;
char *server_ip = "127.0.0.1";

// Helper function to connect to a server
void connectTo(int socket, struct sockaddr_in *address, int port)
{
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = inet_addr(server_ip);
    address->sin_port = htons(port);
    if (connect(socket, (struct sockaddr *)(address), sizeof(struct sockaddr_in)) != 0)
    {
        printf("Error number: %d\n", errno);
        printf("The error message is %s\n", strerror(errno));
        printf("Connection with the server failed.\n");
        exit(0);
    }
}

// Helper function to listen to a client
void listenTo()
{
}

// Main function
int main()
{
    int pid;
    std::cout << "Process #: ";
    std::cin >> pid;
    std::cout << "\n";

    // Create four sockets to connect to other four processes
    int sockets[5]; // sockets[pid - 1] should never be used
    for (int i = 0; i < 5; i++)
    {
        if (i != pid - 1)
        {
            if ((sockets[i] = socket(AF_INET, SOCK_STREAM, 0)) == 0)
            {
                std::cerr << "Failed creating socket for process " << i + 1 << std::endl;
                exit(0);
            }
        }
    }
    std::cout << "Sockets created.\n";

    // Set all the sockets except sockts[pid - 1]
    struct sockaddr_in addresses[5]; // Addresses[pid - 1] should never be used
    int new_sockets[5];              // For accepted connection. new_sockts[pid - 1] should never be used
    int addrlen[5];
    for (int i = 0; i < 5; i++)
    {
        if (i != pid - 1)
        { // Don't use sockets[pid - 1] and addresses[pid - 1]
            // Different processes have different listening/connecting sockets
            // Listen: Port # = port + i
            // Connet: Port # = port + pid - 1
            // P1: 4 listens
            // P2: 3 listens, 1 connect to P1
            // P3: 2 listens, 2 connect to P1, P2
            // P4: 1 listens, 3 connect to P1, P2, P3
            // P5: 4 connect to P1, P2, P3, P4

            // Set sockets if P1
            if (pid == 1)
            { // Ports: 8001, 8002, 8003, 8004
                std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = " << port + i << "\n";
                addresses[i].sin_family = AF_INET;
                addresses[i].sin_addr.s_addr = INADDR_ANY;
                addresses[i].sin_port = htons(port + i);
                if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
                {
                    std::cerr << "Bind failed on " << i + 1 << " socket!\n";
                    printf("Error number: %d\n", errno);
                    printf("The error message is %s\n", strerror(errno));
                    printf("Local socket connection with the server failed.\n");
                    exit(errno);
                }
                if (listen(sockets[i], 3) != 0)
                {
                    std::cerr << "Failed listening on for Process " << i + 1 << "\n";
                }
                std::cout << "Sockets are listening.\n";
                addrlen[i] = sizeof(addresses[i]);
                if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
                {
                    std::cerr << "Failed accepting process " << i + 1 << "\n";
                    printf("Error number: %d\n", errno);
                    printf("The error message is %s\n", strerror(errno));
                    printf("Local socket connection with the server failed.\n");
                    exit(errno);
                }
                std::cout << "Process " << i + 1 << " connected.\n";
            }

            // Set sockets if P2
            else if (pid == 2)
            {
                if (i == 0)
                { // Connect to P1 (8001)
                    std::cout << "Connecting to P1...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8001);
                    if (connect(sockets[0], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                    std::cout << "Connected!\n";
                }
                else
                { // Set other sockets for P3 (2), P4 (3), P5 (4)
                    std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = " << 8003 + i << "\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = INADDR_ANY;
                    addresses[i].sin_port = htons(8003 + i); // Ports: 8005, 8006, 8007
                    if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
                    {
                        std::cerr << "Bind failed on " << i + 1 << " socket!\n";
                        exit(0);
                    }
                    if (listen(sockets[i], 3) != 0)
                    {
                        std::cerr << "Failed listening on for Process " << i + 1 << "\n";
                    }
                    std::cout << "Sockets are listening.\n";
                    addrlen[i] = sizeof(addresses[i]);
                    if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
                    {
                        std::cerr << "Failed accepting process " << i + 1 << "\n";
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Local socket connection with the server failed.\n");
                        exit(errno);
                    }
                    std::cout << "Process " << i + 1 << " connected.\n";
                }
            }

            // Set sockets if P3
            else if (pid == 3)
            {
                if (i == 0)
                {
                    // Connect to P1 (8002)
                    std::cout << "Connecting to P1...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8002);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                    std::cout << "Connected!\n";
                }
                else if (i == 1)
                {
                    // Connect to P2 (8005)
                    std::cout << "Connecting to P2...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8005);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                    std::cout << "Connected!\n";
                }
                else
                { // Set sockets for P4 (3), P5 (4)
                    std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = " << 8005 + i << "\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = INADDR_ANY;
                    addresses[i].sin_port = htons(8005 + i); // Ports: 8008, 8009
                    if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
                    {
                        std::cerr << "Bind failed on " << i + 1 << " socket!\n";
                        exit(0);
                    }
                    if ((listen(sockets[i], 3)) != 0)
                    {
                        std::cerr << "Failed listening on for Process " << i + 1 << "\n";
                    }
                    std::cout << "Sockets are listening.\n";
                    addrlen[i] = sizeof(addresses[i]);
                    if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
                    {
                        std::cerr << "Failed accepting process " << i + 1 << "\n";
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Local socket connection with the server failed.\n");
                        exit(errno);
                    }
                    std::cout << "Process " << i + 1 << " connected.\n";
                }
            }

            // Set sockets if P4
            else if (pid == 4)
            {
                if (i == 0)
                { // Connect to P1 (8003)
                    std::cout << "Connecting to P1...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8003);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                }
                else if (i == 1)
                { // Connect to P2 (8006)
                    std::cout << "Connecting to P2...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8006);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                }
                else if (i == 2)
                { // Connect to P3 (8008)
                    std::cout << "Connecting to P3...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8008);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                }
                else
                { // Set socket for P5 (8010)
                    std::cout << "Setting sockets for P" << i + 1 << ":\n\tport = 8010\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8010);
                    if (bind(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])))
                    {
                        std::cerr << "Failed binding socket for Process " << i + 1 << "\n";
                        exit(0);
                    }
                    if (listen(sockets[i], 3) != 0)
                    {
                        std::cerr << "Failed listening for Process " << i + 1 << "\n";
                        exit(0);
                    }
                    std::cout << "Sockets are listening.\n";
                    addrlen[i] = sizeof(addresses[i]);
                    if ((new_sockets[i] = accept(sockets[i], (struct sockaddr *)(addresses + i), (socklen_t *)(addrlen + i))) < 0)
                    {
                        std::cerr << "Failed accepting process " << i + 1 << "\n";
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Local socket connection with the server failed.\n");
                        exit(errno);
                    }
                    std::cout << "Process " << i + 1 << " connected.\n";
                }
            }

            // Set sockets if P5
            else if (pid == 5)
            {
                if (i == 0)
                { // Connect to P1 (8004)
                    std::cout << "Connecting to P1...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8004);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                }
                else if (i == 1)
                { // Connect to P2 (8007)
                    std::cout << "Connecting to P2...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8007);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                }
                else if (i == 2)
                { // Connect to P3 (8009)
                    std::cout << "Connecting to P3...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8009);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                }
                else if (i == 3)
                { // Connect to P4 (8010)
                    std::cout << "Connecting to P4...\n";
                    addresses[i].sin_family = AF_INET;
                    addresses[i].sin_addr.s_addr = inet_addr(server_ip);
                    addresses[i].sin_port = htons(8010);
                    if (connect(sockets[i], (struct sockaddr *)(addresses + i), sizeof(addresses[i])) != 0)
                    {
                        printf("Error number: %d\n", errno);
                        printf("The error message is %s\n", strerror(errno));
                        printf("Connection with the server failed.\n");
                        exit(0);
                    }
                }
            }
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