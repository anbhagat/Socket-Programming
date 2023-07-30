#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <math.h>
#include <string>
#include <iostream>

using namespace std;

#define LOCALHOST "127.0.0.1" // Host address
#define udpPortServerM 24592 // UDP port number - Server M (24000 + 592)
#define tcpPortServerM 25592 // TCP port number - Server M (25000 + 592)
#define serverEE_PORT 23592 // UDP port number - (Server EE 23000 + 592)
#define serverCS_PORT 22592 // UDP port number - (Server EE 22000 + 592)
#define serverC_PORT 21592 // UDP port number - (Server C 21000 + 592)
#define MAXDATASIZE 1024 // max number of bytes received at once
#define BACKLOG 10 // max number of incoming connections allowed
#define FAIL -1 //if any of the socket commands return -1, the command failed

int serverM_clientFD; //parent TCP socket
int serverM_udpFD; // udp socket descriptor 
int childSocketFD; //child TCP socket
struct sockaddr_in serverM_udpAddr; //serverEE
struct sockaddr_in serverMCS_udpAddr; //serverCS
struct sockaddr_in serverM_client_addr; //serverM
struct sockaddr_in destClient_addr; //parent listening socket

//Taken from Beej’s socket programming tutorial
//Initialize serverEE sockaddr_in with respective port #
void initializeToServerEE() {
    memset(&serverM_udpAddr, 0, sizeof(serverM_udpAddr));
    serverM_udpAddr.sin_family = AF_INET;
    serverM_udpAddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    serverM_udpAddr.sin_port = htons(serverEE_PORT);
}
//Taken from Beej’s socket programming tutorial
//Initialize serverCS sockaddr_in with respective port #
void initializeToServerCS() {
    memset(&serverMCS_udpAddr, 0, sizeof(serverMCS_udpAddr));
    serverMCS_udpAddr.sin_family = AF_INET;
    serverMCS_udpAddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    serverMCS_udpAddr.sin_port = htons(serverCS_PORT);
}
//Taken from Beej’s socket programming tutorial
//Initialize serverC sockaddr_in with respective port #
void initializeToServerC() {
    memset(&serverM_client_addr, 0, sizeof(serverM_client_addr));
    serverM_client_addr.sin_family = AF_INET;
    serverM_client_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    serverM_client_addr.sin_port = htons(serverC_PORT);
}
//Taken from Beej’s socket programming tutorial
//Create ServerM TCP socket 
void createTCPSocket()
{
    serverM_clientFD = socket(AF_INET, SOCK_STREAM, 0);

    if (serverM_clientFD == FAIL)
    {
        perror("[ERROR] Server M failed to create TCP SOcket for Client");
        exit(1);
    }
    int yes=1;
    //Taken from Beej’s socket programming tutorial
    //To avoid 'port already in use' error message
    if (setsockopt(serverM_clientFD,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }
    // Initialize struct
    memset(&serverM_client_addr, 0, sizeof(serverM_client_addr));
    serverM_client_addr.sin_family = AF_INET;
    serverM_client_addr.sin_addr.s_addr = inet_addr(LOCALHOST); // Host IP address
    serverM_client_addr.sin_port = htons(tcpPortServerM); // Port number for Client

    // Bind socket
    if (::bind(serverM_clientFD, (struct sockaddr *) &serverM_client_addr, sizeof(serverM_client_addr)) == FAIL) {
        perror("[ERROR] Server M failed to bind TCP socket");
        exit(1);
    }
}
//Create ServerM UDP Socket
void createUDPSocket()
{
    serverM_udpFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverM_udpFD == FAIL) {
        perror("[ERROR] Server M failed to create UDP socket.");
        exit(1);
    }
    // Initialize IP address, port number
    memset(&serverM_udpAddr, 0, sizeof(serverM_udpAddr)); //  make sure the struct is empty
    serverM_udpAddr.sin_family = AF_INET; // Use IPv4 address family
    serverM_udpAddr.sin_addr.s_addr = inet_addr(LOCALHOST); // Host IP address
    serverM_udpAddr.sin_port = htons(udpPortServerM); // Port number for client

    // // Bind socket
    // if (::bind(serverM_udpFD, (struct sockaddr *) &serverM_udpAddr, sizeof(serverM_udpAddr)) == FAIL) {
    //     perror("[ERROR] Server M failed to bind UDP socket");
    //     exit(1);
    // }
}
//Listen for incoming client requests for the TCP parent socket
void listenToClient()
{
    //Taken from Beej’s socket programming tutorial
    
    if (listen(serverM_clientFD, BACKLOG) == FAIL)
    {
        perror("[ERROR] Server M failed to listen to client socket");
        exit(1);
    }
}
/** encrypt(string msg)
     * main server encrypts info and sends it to credential server (listening via TCP port)
     *          The encryption scheme would be as follows:
     *          Offset each character and/or digit by 4. The scheme is case sensitive.
     *          Special characters (including spaces and/or the decimal point) will not be encrypted or changed.
     * 
    /*/
string encrypt(string msg)
{
    string encryptedMsg = "";
    for (int i = 0; i < msg.length(); i++)
    {
        // if the message is a digit, the digit should become (digit + 4) %10
        if (isdigit(msg[i]))
        {
            int digit = msg[i] - '0';
            digit = (digit + 4) % 10;
            encryptedMsg += to_string(digit);
        }
        // if the digit is a lower case letter, the letter should become (lower case letter + 4) % 26
        else if (islower(msg[i]))
        {
            int letter = msg[i] - 'a';
            letter = (letter + 4) % 26;
            encryptedMsg += (char)(letter + 'a');
        }
        // if the digit is a upper case letter, the letter should become (upper case letter + 4) % 26
        else if (isupper(msg[i]))
        {
            int letter = msg[i] - 'A';
            letter = (letter + 4) % 26;
            encryptedMsg += (char)(letter + 'A');
        }
        // if the digit is a special character, the character should not be changed
        else
        {
            encryptedMsg += msg[i];
        }
    }
    return encryptedMsg;
}

int main(int argc, char *argv[]){
    //Create TCP client
    createTCPSocket();
    //Start listening to client requests
    listenToClient();
    //Create UDP socket
    createUDPSocket();
    printf("The main server is up and running.\n");

    string username;
    socklen_t len = 0;
    //Char arrays for data buffer to send and receive data
    char buf[MAXDATASIZE];
    char departmentBuf[MAXDATASIZE];
    char bufEE[MAXDATASIZE];
    char replyMessageEE[MAXDATASIZE]; 
    char replyMessageCS[MAXDATASIZE];
    char replyMessageC[MAXDATASIZE];

    while(1)
    {
         //Accept connection from client using child socket
        socklen_t clientAddrSize = sizeof(destClient_addr); 
        //Accept listening socket (parent) -> Repurposed from Beej’s socket programming tutorial
        childSocketFD = ::accept(serverM_clientFD, (struct sockaddr *) &destClient_addr, &clientAddrSize);
        if (childSocketFD == FAIL)
        {
            perror("[ERROR] Server M failed to accept connection with Client.");
            exit(1);
        }
        //main server forwards the authentication request to the credentials server over UDP 
        //main server receives the authentication request result from serverC via UDP
        //main server sends result to client over TCP connection
        bool firstPhase = true;
        bool secondPhase = false;
        while (firstPhase == true)
        {
            //Receive through child socket -> Repurposed from Beej’s socket programming tutorial
            int recvClient = recv(childSocketFD, buf, MAXDATASIZE, 0);
            if (recvClient == FAIL)
            {
                perror("[ERROR] Server M failed to receive input data from client or authentication has failed.");
                exit(1);
            }
            //Obtain the username and password strings by splitting the received string by space
            string dataBuf = buf;
            //Truncate the buffer message so that random characters are not obtained
            dataBuf = dataBuf.substr(0, recvClient);
            int spacePosition = dataBuf.find_first_of(' ');
            username = dataBuf.substr(0, spacePosition);
            string password = dataBuf.substr(spacePosition + 1, recvClient - spacePosition);
            printf("The main server received the authentication for %s using TCP over port %d.\n", username.c_str(), tcpPortServerM);
            //Encrypt the data
            string encryptedData = encrypt(dataBuf);
            //Create ServerC socket to send encrypted data
            int clientC = socket(AF_INET, SOCK_DGRAM, 0);
            if (clientC == FAIL)
            {
                perror("[ERROR] Server M failed to create UDP socket for Client.");
                exit(1);
            }
            initializeToServerC();
            //Repurposed from Beej’s socket programming tutorial
            //Send to Server C
            if (sendto(clientC, encryptedData.c_str(), encryptedData.length(), 0, (struct sockaddr *) &serverM_client_addr, sizeof(serverM_client_addr)) == FAIL)
            {
                perror("[ERROR] Server M failed to send data to Server C.");
                exit(1);
            }
            printf("The main server has sent an authentication request to serverC.\n");
            //Receive whether the username & password were authenticated from Server C
            int recvC = recvfrom(clientC, replyMessageC, MAXDATASIZE, 0, (struct sockaddr *) &serverM_client_addr, &len);
            if (recvC == FAIL)
            {
                perror("[ERROR] Server M failed to receive data from Client.");
                exit(1);
            }
            replyMessageC[recvC] = '\0';
            printf("The main server has received the result of the authentication request from ServerC using UDP over port %d\n", udpPortServerM);
            char result = replyMessageC[0];
            if (send(childSocketFD, replyMessageC, MAXDATASIZE, 0) == FAIL)
            {
                perror("[ERROR] Server M failed to send data to Client.");
                exit(1);
            }
            printf("The main server sent the authentication result to the client.\n");
            //If the result of the authenticated message is 0, authentication step is complete & next phase can begin
            if (result == '0')
            {
                firstPhase = false;
                secondPhase = true;
            }
        }
        //PHASE 3
        //RECEIVE THE COURSE CODE & CATEGORY 
        while ((secondPhase == true) && (firstPhase == false))
        {
            //Receive the course code and category from client server
            int recvPhase2 = recv(childSocketFD, departmentBuf, MAXDATASIZE, 0);
            if (recvPhase2 == FAIL)
            {
                perror("[ERROR] Server M failed to receive the query information from client.");
                exit(1);
            }
            departmentBuf[recvPhase2] = '\0';
            //Use substr() to obtain the department (CS or EE), course code, and category from the user-input
            string data = departmentBuf;
            data = data.substr(0, recvPhase2);
            string courseCode = data;
            string category = data;
            string department = data.substr(0, 2);
            courseCode = data.substr(0,data.find_first_of(' '));
            category = data.substr(data.find_first_of(' ') + 1);
            printf("The main server received from %s to query course %s about %s using TCP over port %d\n", username.c_str(), courseCode.c_str(), category.c_str(), tcpPortServerM);
            if (department == "CS")
            {
                //create UDP socket for CS department
                int clientCS = socket(AF_INET, SOCK_DGRAM, 0);
                if (clientCS == FAIL)
                {
                    perror("[ERROR] Server CS failed to create UDP socket.");
                    exit(1);
                }
                initializeToServerCS();
                if (sendto(serverM_udpFD, data.c_str(), data.length(), 0, (struct sockaddr *) &serverMCS_udpAddr, sizeof(serverMCS_udpAddr)) == FAIL)
                {
                    perror("[ERROR] Server M failed to send data to ServerCS.");
                    exit(1);
                }
                printf("The main server sent a request to serverCS.\n");
                int recvCS = recvfrom(serverM_udpFD, replyMessageCS, MAXDATASIZE, 0, (struct sockaddr *) &serverMCS_udpAddr, &len);
                if (recvCS == FAIL)
                {
                    perror("[ERROR] Server M failed to receive data from ServerCS.");
                    exit(1);
                }
                replyMessageCS[recvCS] = '\0';
                printf("The main server received the response from serverCS using UDP over port %d.\n",udpPortServerM);
                
                if (send(childSocketFD, replyMessageCS, MAXDATASIZE, 0) == FAIL)
                {
                    perror("[ERROR] Server M failed to send data to client.");
                    exit(1);
                }
                printf("The main server sent the query information to the client.\n");
                
            }
            else if (department == "EE")
            {
                //create UDP socket for CS department
                int clientEE = socket(AF_INET, SOCK_DGRAM, 0);
                if (clientEE == FAIL)
                {
                    perror("[ERROR] ServerEE failed to create UDP socket.");
                    exit(1);
                }
                initializeToServerEE();
                if (sendto(serverM_udpFD, data.c_str(), data.length(), 0, (struct sockaddr *) &serverM_udpAddr, sizeof(serverM_udpAddr)) == FAIL)
                {
                    perror("[ERROR] Server M failed to send data to serverEE.");
                    exit(1);
                }
                printf("The main server sent a request to serverEE.\n");
                //Receive from serverEE
                int recvEE = recvfrom(serverM_udpFD, replyMessageEE, MAXDATASIZE, 0, (struct sockaddr *) &serverM_udpAddr, &len);
                if (recvEE == FAIL)
                {
                    perror("[ERROR] Server M failed to receive data from serverEE.");
                    exit(1);
                }
                replyMessageEE[recvEE] = '\0';
                printf("The main server received the response from server EE using UDP over port %d.\n",udpPortServerM);
                
                if (send(childSocketFD, replyMessageEE, MAXDATASIZE, 0) == FAIL)
                {
                    perror("[ERROR] Server M failed to send data to client.");
                    exit(1);
                }
                printf("The main server sent the query information to the client.\n");
            }
        }
    }
    close(childSocketFD);
    return 0;
}
