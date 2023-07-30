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
 * ServerCS.cpp
 * Department Server, stores cs.txt course information offered within the CS department
 */

//Named Constants
#define LOCALHOST "127.0.0.1"
#define ServerCS_UDP_PORT 22592
#define ServerM_UDP_PORT 24592
#define MAXDATASIZE 1024
#define FAIL -1 

//Global variables
int serverCS_sockfd;
struct sockaddr_in my_addr;
struct sockaddr_in serverM_addr;
vector<string> csText; 
map<string, vector<string> > information;
fstream database;
char recv_buf[MAXDATASIZE];
char write_result[MAXDATASIZE]; //get from server CS -> send to M

//Repurposed from Beej’s socket programming tutorial
//Creates the UDP socket for ServerCS
void create_serverCS_socket()
{
    serverCS_sockfd = socket(PF_INET, SOCK_DGRAM, 0); //creates the UDP socket
    if (serverCS_sockfd == FAIL)
    {
        perror("[ERROR] Server CS cannot open socket.");
        exit(1);
    }
}
//Repurposed from Beej’s socket programming tutorial
//initializeConnection() sets the struct sockaddr_in for serverCS with appropriate parameters and its' assigned UDP port #
void initializeConnection()
{
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    my_addr.sin_port = htons(ServerCS_UDP_PORT);
    
}
//Repurposed from Beej’s socket programming tutorial
//bindSocket() binds serverCS's UDP socket
void bindSocket() {
    if (::bind(serverCS_sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) == FAIL) {
        perror("[ERROR] Server CS failed to bind UDP socket.");
        exit(1);
    }
    printf("The Server CS is up and running using UDP on port %d. \n", ServerCS_UDP_PORT);
}

//readFile() takes in file cs.txt and loops through every line of the file storing it in vector csText 
void readFile(string fileName)
{
    database.open(fileName, ios::in);
    if (database.is_open())
    {
        string line;
        while (getline(database, line))
        {
            if ((line[line.length()-1] == '\r') || (line[line.length()-1] == '\n')) //check if there's whitespace or a newline character
            {
                line = line.substr(0,line.length()-1);
            }
            csText.push_back(line);
        }
        database.close();
    }
}

//fillMap() takes iterates through csText creating the information map structure, with keys being course code and values being a vector of the categories
void fillMap()
{
    for(auto itr: csText)
    {
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
    create_serverCS_socket(); //Create a UDP socket for Server EE
    //Create sockaddr_in struct
    initializeConnection();
    //Bind Socket
    bindSocket();
    //Read input file cs.txt to store in backend server
    readFile("cs.txt");
    //Create the information map [stores course codes as keys with a vector containing each category as an index]
    fillMap();
    bool flag = true;
    while (flag == true)
    {
        //Receive data from Server M
        char serverMBuffer[MAXDATASIZE];
        socklen_t addr_len = sizeof(serverM_addr);
        int numbytes = recvfrom(serverCS_sockfd, serverMBuffer, MAXDATASIZE - 1, 0, (struct sockaddr *) &serverM_addr, &addr_len);
        if (numbytes == FAIL)
        {
            perror("[ERROR] Server CS failed to receive data from Server M.");
            exit(1);
        }
        //Add '\0\ after the number of bytes received to indicate the end of the message
        serverMBuffer[numbytes] = '\0';
        string courseCode = serverMBuffer;
        string category = serverMBuffer;
        courseCode = courseCode.substr(0,courseCode.find_first_of(' '));
        //Truncate the buffer message so that random characters are not obtained at the end
        category = category.substr(category.find_first_of(' ') + 1, numbytes - courseCode.length()-1);
        
        printf("The ServerCS received a request from the Main Server about the <%s> of <%s>.\n", category.c_str(), courseCode.c_str());
        //Check if the course code is found within the keys of the information map
        if (information.find(courseCode) != information.end())
        {
            
            int requestIndex = checkCategory(category);
            //Extract the appropriate information based on the query from the information map
            string returnInfo = information[courseCode].at(requestIndex);
            string response = courseCode + "," + category + "," + returnInfo; 
            printf("The course information has been found: The %s of %s is %s.\n", category.c_str(), courseCode.c_str(), returnInfo.c_str());
            // Send course (courseCode), category (category), and information to server M
            //Repurposed from Beej’s socket programming tutorial
            if (sendto(serverCS_sockfd, response.c_str(), response.length(), 0, (struct sockaddr *) &serverM_addr, addr_len) == FAIL)
            {
                perror("[ERROR] Server CS failed to send data to Server M.");
                exit(1);
            }
            
        }
        else //Course code was not found within the keys of the information map, send a 0 to alert serverM
        {
            printf("Didn't find the course: %s.\n", courseCode.c_str());
            string response = "0"; 
            //Repurposed from Beej’s socket programming tutorial
            if (sendto(serverCS_sockfd, response.c_str(), response.length(), 0, (struct sockaddr *) &serverM_addr, addr_len) == FAIL)
            {
                perror("[ERROR] Server CS failed to send data to Server M.");
                exit(1);
            }
        }
        printf("The Server CS finished sending the response to the Main Server.\n");

    }
    
    return 0;
}


