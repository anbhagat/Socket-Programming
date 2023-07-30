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
#include <assert.h>
#include <string>
#include <iostream>

using namespace std;

/**
 * Client.cpp
 * The client has the main control of this socket programming project.
 * Creates a TCP socket
 * Obtains user input of the username/password [authentication phase]
 * Obtains user input of the course code/category[query phase]
 * Includes validation check for the course code (has to be EE or CS department, otherwise user is asked to re-enter code)
 */

#define LOCALHOST "127.0.0.1" //hardcoded host name
#define MAXDATASIZE 1024
#define tcpPortServerM 25592 // TCP port number - Server M (25000 + 592)
#define FAIL -1

/** 
 * Global variables
 * */

int clientFD; //client socket

struct sockaddr_in serverM_addr;
struct sockaddr_in my_addr;
char write_result[MAXDATASIZE];
//Repurposed from Beej’s socket programming tutorial
void createClientSocket()
{
    clientFD = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFD == FAIL)
    {
        perror("[ERROR] Client could not open TCP socket.");
        exit(1);
    } 
}

void initializeRequest()
{
    //Initialize TCP Connection between Client Socket & Server M, repurposed from Beej’s socket programming tutorial
    memset(&serverM_addr, 0, sizeof(serverM_addr));
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_addr.s_addr = inet_addr(LOCALHOST); //Source Address
    serverM_addr.sin_port = htons(tcpPortServerM); //Server M Port Number
}

void connectToServerM()
{
    //Repurposed from Beej’s socket programming tutorial
    int connectStatus = connect(clientFD, (struct sockaddr *) &serverM_addr, sizeof(serverM_addr));
    if (connectStatus < 0) {
        printf("[ERROR] Client could not connect to Server M\n");
    }
    printf("The client is up and running. \n");
}

int main(int argc, char *argv[]){
    createClientSocket();
    initializeRequest();
    connectToServerM();

    bzero(&my_addr, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    getsockname(clientFD, (struct sockaddr *) &my_addr, &len);
    unsigned int portNum = ntohs(my_addr.sin_port);


    bool flag = true;
    bool phasePass = false;
    int counter = 0;
    string username;
    string password;

    while (flag == true)
    {
        printf("Please enter the username: ");
        cin >> username;
        printf("Please enter the password: ");
        cin >> password;
        // send a message that is username space password
        string msg = username + " " + password;
        //Send to server M (use c_str() to convert msg into char array)
        int bytes_sent = send(clientFD, msg.c_str(), msg.length(), 0);
        
        printf("<%s> sent an authentication request to the main server.\n", username.c_str());
        //serverM sends the result of the authentication request to the client over a TCP connection
        if (recv(clientFD, write_result, sizeof(write_result), 0) == FAIL) {
            perror("[ERROR] client: fail to receive write result from  server");
            close(clientFD);
            exit(1);
        }
        printf("<%s> received the result of authentication using TCP over port <%d>.\n", username.c_str(),portNum);
        char result = write_result[0];
        if (result == '0') { // Successfully authenticated username & password
            printf("Authentication is successful.\n");
            phasePass = true; //start the next while loop
            flag = false; //end this while loop
            
        }
        else if (result == '1') // Unsucessfully authenticated username
        {
            counter ++; //3 IS THE MAX NUMBER OF ATTEMPTS TO AUTHENTICATE
            printf("Authentication failed: Username does not exist.\n");
            if (counter == 3)
            {
                flag = false;
                printf("Authentication Failed for 3 attempts. Client will shut down.\n");
                close(clientFD);
            }
            else
            {
                printf("Attempts remaining: <%d>\n", (3 - counter));
            }
        }
        else if (result == '2')
        {
            // Unsucessfully authenticated password
            counter ++;
            printf("Authentication failed: Password does not match\n");
            if (counter == 3)
            {
                flag = false;
                printf("Authentication Failed for 3 attempts. Client will shut down.\n");
                close(clientFD);
            }
            else
            {
                printf("Attempts remaining: <%d>\n", (3 - counter));
            }
        }
    }
    while (phasePass == true) //loop to allow for continuous requests
    {
        printf("Please enter the course code to query: ");
        string course_code;
        cin >> course_code;
        string checkDept = course_code.substr(0,2);
        if ((checkDept != "EE") && (checkDept != "CS")) //only the CS and EE department information queries are allowed 
        {
            printf("Please enter a course code within the CS or EE department: ");
            cin >> course_code;
        }
        printf("Please enter the category (Credit/Professor/Days/CourseName): ");
        string category;
        cin >> category;
        if (category != "Credit" && category != "Professor" && category != "Days" && category != "CourseName")
        {
            printf("Please re-enter the category (Credit/Professor/Days/CourseName): ");
            cin >> category;
        }
        string query = course_code + " " + category;
        //Send the query of course code and category (separated parameters by white space delimiter) to serverM
        int bytesSent = send(clientFD, query.c_str(), query.length(), 0);
        printf("<%s> sent a request to the main server.\n", username.c_str());
        //Receive whether the corresponding department found the course or not.
        int recvDepartment = recv(clientFD, write_result, sizeof(write_result), 0);
        if (recvDepartment == FAIL)
        {
            perror("[ERROR] client: fail to receive write result from server");
            exit(1);
        }
        write_result[recvDepartment] = '\0';
        string write_result_str = write_result;
        write_result_str = write_result_str.substr(0, recvDepartment);
        printf("The client received the response from the Main server using TCP over port <%d>.\n", portNum);
        //if first character is a 0, then it couldn't find the course
        char result = write_result[0];
        if (result == '0')
        {
            printf("Didn't find the course: %s.\n", course_code.c_str());
        }
        else // write result has [course code, category, information]
        {
            string query = write_result_str;
            int start = 0;
            int commaPos = query.find(",");
            string info = "";
            while (commaPos != -1) //loop through the substrings separated by commas to get the last one (the information)
            {
                commaPos = query.find(",");
                if (commaPos == -1) {
                    info = query;
                    break;
                } else {
                    query = query.substr(commaPos+1);
                }
            }
            //For appropriate output messages, change categories to lower case and include spaces
            if (category == "CourseName")
            {
                category = "course name";
            }
            else if (category == "Credit")
            {
                category = "credit";
            }
            else if (category == "Professor")
            {
                category = "professor";
            }
            else if (category == "Days")
            {
                category = "days";
            }
            printf("The %s of %s is %s.\n", category.c_str(), course_code.c_str(), info.c_str());
        }
        printf("-----Start a new request-----\n");
    }
    return 0;
}
