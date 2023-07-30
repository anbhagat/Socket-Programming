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
#include <string>
#include <vector>

using namespace std;

/**
 * ServerEE.cpp
 * Department Server, stores ee.txt course information offered within the EE department
 */

//Named Constants
#define LOCALHOST "127.0.0.1"
#define ServerEE_UDP_PORT 23592
#define ServerM_UDP_PORT 24592
#define MAXDATASIZE 1024
#define FAIL -1 


//Global variables
int serverEE_sockfd; //serverEE's file descriptor
struct sockaddr_in my_addr; //struct sockaddr_in for serverEE
struct sockaddr_in serverM_addr; //struct sockaddr_in for serverM
fstream database;
char write_result[MAXDATASIZE]; //get from server EE -> send to M
vector<string> eeText; 
map<string, vector<string> > information;


//Repurposed from Beej’s socket programming tutorial
//Creates the UDP socket for ServerEE
void create_serverEE_socket()
{
    serverEE_sockfd = socket(PF_INET, SOCK_DGRAM, 0); //creates the UDP socket
    if (serverEE_sockfd == FAIL)
    {
        perror("[ERROR] Server EE cannot open socket.");
        exit(1);
    }
}
//Repurposed from Beej’s socket programming tutorial
//initializeConnection() sets the struct sockaddr_in for serverEE with appropriate parameters and its' assigned UDP port #
void initializeConnection()
{
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    my_addr.sin_port = htons(ServerEE_UDP_PORT);
    
}
//Repurposed from Beej’s socket programming tutorial
//bindSocket() binds serverEE's UDP socket
void bindSocket() {
    if (::bind(serverEE_sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) == FAIL) {
        perror("[ERROR] Server EE failed to bind UDP socket");
        exit(1);
    }
    printf("The Server EE is up and running using UDP on port %d. \n", ServerEE_UDP_PORT);
}
//readFile() takes in file ee.txt and loops through every line of the file storing it in vector eeText 
void readFile(string fileName)
{
    database.open(fileName, ios::in);
    if (database.is_open())
    {
        string line;
        while (getline(database, line))
        {
            if ((line[line.length()-1] == '\r') || (line[line.length()-1] == '\n'))
            {
                line = line.substr(0,line.length()-1);
            }
            eeText.push_back(line);
        }
        database.close();
    }
}
//fillMap() takes iterates through eeText creating the information map structure, with keys being course code and values being a vector of the categories
void fillMap()
{
    for(auto itr: eeText)
    {
        //splits the file line by commas to get the different parameters
        int comma = itr.find_first_of(',');
        string courseCode = itr.substr(0, comma);
        string courseInfo = itr.substr(comma + 1);
        vector<string> values;
        int start = 0;
        int commaPos = courseInfo.find(",");
        while (commaPos != -1)
        {
            values.push_back(courseInfo.substr(start, commaPos - start));
            start = commaPos + 1;
            if (courseInfo.find(",", start) != -1) {
                commaPos = courseInfo.find(",", start);
            } else {
            values.push_back(courseInfo.substr(start));
            break;
            }
        }
        information[courseCode] = values;
    }
}
// checkCategory(string category) returns the index of the vector that the category parameter string corresponds to
// 0 = Credit, 1 = Professor, 2 = Days, 3 = CourseName
int checkCategory(string category)
{
    int index = 0;
    if (category == "Credit")
    {
        index = 0;
    }
    else if (category == "Professor")
    {
        index = 1;
    }
    else if (category == "Days")
    {
        index = 2;
    }
    else if (category == "CourseName")
    {
        index = 3;
    }
    return index;
}


int main(int argc, char *argv[]){
    //Create a UDP socket for Server EE
    create_serverEE_socket(); 
    //Create sockaddr_in struct
    initializeConnection();
    //Bind Socket with IP address and port number
    bindSocket();
    //Read input file ee.txt to store in backend server
    readFile("ee.txt");
    //Create the information map [stores course codes as keys with a vector containing each category as an index]
    fillMap();
    bool flag = true;
    while (flag == true)
    {
        //Receive data from Server M
        char serverMBuffer[MAXDATASIZE];
        socklen_t addr_len = sizeof(serverM_addr);
        int numbytes = recvfrom(serverEE_sockfd, serverMBuffer, MAXDATASIZE - 1, 0, (struct sockaddr *) &serverM_addr, &addr_len);
        if (numbytes == FAIL)
        {
            perror("[ERROR] Server EE failed to receive data from Server M.");
            exit(1);
        }
        string courseCode = serverMBuffer;
        string category = serverMBuffer;
        courseCode = courseCode.substr(0, courseCode.find_first_of(' '));
        //Truncate the buffer message so that random characters are not obtained at the end
        category = category.substr(category.find_first_of(' ') + 1, numbytes - courseCode.length()- 1);

        printf("The ServerEE received a request from the Main Server about the <%s> of <%s>.\n", category.c_str(), courseCode.c_str());
        //Check if the course code is found within the keys of the information map
        if (information.find(courseCode) != information.end())
        {
            int requestIndex = checkCategory(category);
            //Extract the appropriate information based on the query from the information map
            string returnInfo = information[courseCode].at(requestIndex);
            string response = courseCode + "," + category + "," + returnInfo; 
            printf("The course information has been found: The %s of %s is %s.\n", category.c_str(), courseCode.c_str(), returnInfo.c_str());
            //Repurposed from Beej’s socket programming tutorial
            //Send the course code, category, and return info separated by commas back to serverM
            if (sendto(serverEE_sockfd, response.c_str(), response.length(), 0, (struct sockaddr *) &serverM_addr, addr_len) == FAIL)
            {
                perror("[ERROR] Server EE failed to send data to Server M.");
                exit(1);
            }
        }
        else //Course code was not found within the keys of the information map, send a 0 to alert serverM
        {
            printf("Didn't find the course: %s.\n", courseCode.c_str());
            string response = "0"; 
            //Repurposed from Beej’s socket programming tutorial
            if (sendto(serverEE_sockfd, response.c_str(), response.length(), 0, (struct sockaddr *) &serverM_addr, addr_len) == FAIL)
            {
                perror("[ERROR] Server EE failed to send data to Server M.");
                exit(1);
            }
        }
        printf("The Server EE finished sending the response to the Main Server.\n");
    }
    return 0;
}


