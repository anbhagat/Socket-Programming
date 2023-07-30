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
#include <string>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <map>

using namespace std;

/**
 * ServerC.cpp
 * Credential Server, verifies the identity of the student, 
 *                    read corresponding cred.txt & store info in data structure
 */
//Named Constants
#define LOCALHOST "127.0.0.1" //hardcoded host name
#define ServerC_UDP_PORT 21592 // UDP port number - (Server C 21000 + 592)
#define MAXDATASIZE 1024
#define FAIL -1 


//Global variables
int serverC_sockfd; //serverC's socket descriptor
struct sockaddr_in my_addr; //serverC's struct sockaddr_in
struct sockaddr_in serverM_addr; //serverM's struct sockaddr_in
map<string, string> credText;
fstream database;

//Repurposed from Beej’s socket programming tutorial
//Creates the UDP socket for ServerC
void create_serverC_socket()
{
    serverC_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverC_sockfd == FAIL)
    {
        perror("[ERROR] Server EE cannot open socket.");
        exit(1);
    }
}
//Repurposed from Beej’s socket programming tutorial
//initializeConnection() sets the struct sockaddr_in for serverC with appropriate parameters and its' assigned UDP port #
void initializeConnection()
{
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    my_addr.sin_port = htons(ServerC_UDP_PORT);
    
}
//Repurposed from Beej’s socket programming tutorial
//bindSocket() binds serverC's UDP socket
void bindSocket() {
    if (::bind(serverC_sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) == FAIL) {
        perror("[ERROR] Server C failed to bind UDP socket");
        exit(1);
    }
    printf("The Server C is up and running using UDP on port <%d>. \n", ServerC_UDP_PORT);
}
//readFile() takes in file cred.txt and loops through every line of the file storing it in map credText with keys being usernames and values being passwords
void readFile(string fileName)
{
    database.open(fileName, ios::in);
    if (database.is_open())
    {
        string line;
        while (getline(database, line))
        {
            //splits the file line by commas to get the different parameters
            int commaPosition = line.find_first_of(',');
            string username = line.substr(0, commaPosition);
            string password = line.substr(commaPosition + 1);
            // if the password contains whitespace at the last character, remove it
            if (password[password.length() - 1] == '\r' || password[password.length() - 1] == '\n')
            {
                password = password.substr(0, password.length() - 1);
            }
            credText[username] = password;
        }
        database.close();
    }
}

int main(int argc, char *argv[]){
    //Create a UDP socket for Server EE
    create_serverC_socket(); 
    //Initialize sockaddr_in struct my_addr
    initializeConnection();
    //Bind Socket
    bindSocket();
    //Populate credential server
    readFile("cred.txt");
    bool flag = true;
    while (flag == true) {
        char buffer[MAXDATASIZE];
        socklen_t addr_len = sizeof(serverM_addr);
        //Receive data from Server M
        int numbytes = recvfrom(serverC_sockfd, buffer, MAXDATASIZE - 1, 0, (struct sockaddr *) &serverM_addr, &addr_len);
        if (numbytes == FAIL)
        {
            perror("[ERROR] Server C failed to receive data from Server M.");
            exit(1);
        }
        //Add '\0' to the end of the buffer to indicate this is the last byte of the message
        buffer[numbytes] = '\0';
        printf("The ServerC received an authentication request from the Main Server.\n");

        // get username and password from the buffer received from Server M
        // the format of the buffer is "username password"
        // then check if the username and password are in the cred.txt
        // if both passed, send "0" to Server M
        // if no, send "1" to Server M for not finding the username and 2 for an incorrect password
        string username = buffer;
        string password = buffer;
        username = username.substr(0, username.find_first_of(' '));
        //Truncate the buffer message so that random characters are not obtained at the end
        password = password.substr(password.find_first_of(' ') + 1, numbytes - username.length() - 1);
        if (credText.find(username) != credText.end() && credText[username] == password)
        {
            string response = "0";
            //Repurposed from Beej’s socket programming tutorial
            if (sendto(serverC_sockfd, response.c_str(), response.length(), 0, (struct sockaddr *) &serverM_addr, addr_len) == FAIL)
            {
                perror("[ERROR] Server C failed to send data to Server M.");
                exit(1);
            }
            printf("The ServerC finished sending the response to the Main Server.\n");
        }
        else
        {
            string response = "";
            if (credText.find(username) == credText.end())
            {
                response = "1"; //if username was not found
            } else if (credText[username] != password){
                response = "2"; //if username was found, but the password did not match
            }
            //Repurposed from Beej’s socket programming tutorial
            if (sendto(serverC_sockfd, response.c_str(), response.length(), 0, (struct sockaddr *) &serverM_addr, addr_len) == FAIL)
            {
                perror("[ERROR] Server C failed to send data to Server M.");
                exit(1);
            }
            printf("The ServerC finished sending the response to the Main Server.\n");
        }
    }
    
    return 0;
}

