#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

/*was created to measure time for the log*/
char buffLog[100];
fstream logFile;
time_t rawtime;
struct tm* timeInfo;


string clientName;
char bufferRead[99999];
int sockfd;

vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}


/**
 * Checks if the system call result succeed
 * @param res The result
 * @param functionName The function to check
 */
void checkResults(int res, string functionName){
    if(res < 0){
        time(&rawtime);
        timeInfo = localtime(&rawtime);
        strftime(buffLog, 100, "%I:%M:%S", timeInfo);
        logFile << buffLog << "\tERROR\t" <<functionName<< "\t"<<errno<<".\n";
        close(sockfd);
        exit(1);
    }
}

void handleServerRes(string sendMsg){
    time(&rawtime);
    timeInfo = localtime(&rawtime);
    strftime(buffLog, 100, "%I:%M:%S", timeInfo);
    checkResults((int)write(sockfd, sendMsg.c_str(), strlen(sendMsg.c_str())),
                 "write");
}

void handleRegister(){
    string sendMsg = clientName + " REGISTER";
    handleServerRes(sendMsg);
    bzero(bufferRead,99999);

    checkResults((int)read(sockfd,bufferRead,99998),  "read");

    if(strcmp(bufferRead, "ERROR") == 0){
        logFile << buffLog << "\tERROR: the client " + clientName +
                              " was already registered.\n";
    }
    else{
        logFile << buffLog << "\tClient " + clientName +
                                      " was registered successfully.\n";
    }
}

void handleCreate(string eventTitle, string eventDate, string eventDesc){
    string sendMsg = clientName + " CREATE " + eventTitle+ " "+ eventDate+" "+
            eventDesc;
    handleServerRes(sendMsg);
    bzero(bufferRead,99999);

    checkResults((int)read(sockfd,bufferRead,99998),  "read");

    if(strcmp(bufferRead, "ERROR") == 0){
        logFile << buffLog <<"\tERROR: failed to create the event: " <<
                eventTitle << " due to deadlock.\n";
    }
    else{
        logFile << buffLog << "\tEvent id " << bufferRead <<
                " was created successfully.\n";
    }
}

void handleTop5(){
    string sendMsg = clientName + " GET_TOP_5";
    handleServerRes(sendMsg);
    bzero(bufferRead,99999);

    checkResults((int)read(sockfd,bufferRead,99998),  "read");

    if(strcmp(bufferRead, "ERROR") == 0){
        logFile << buffLog <<"\tERROR: failed to receive top 5 newest events:"
                " due to deadlock.\n";
    }
    else if(strcmp(bufferRead, "EMPTY") == 0){
        logFile << buffLog << "\tTop 5 newest events are:\n.\n";
    }
    else{
        string events;
        vector<string> arrayEvents = split(bufferRead, ',');
        for(auto it = arrayEvents.begin(); it != arrayEvents.end(); ++it){
            if((*it) == "|"){
                events = events.replace(events.end()-1, events.end(), ".\n");
            }
            else {
                events += (*it)+"\t";
            }
        }
        logFile << buffLog << "\tTop 5 newest events are:\n"<< events << ".\n";
    }
}

void handleSendRSVP(string eventId){
    string sendMsg = clientName + " SEND_RSVP " + eventId;
    handleServerRes(sendMsg);
    bzero(bufferRead,99999);

    checkResults((int)read(sockfd,bufferRead,99998), "read");

    if(strcmp(bufferRead, "-2") == 0){
        logFile << buffLog <<"\tERROR: failed to send RSVP to event id " <<
                eventId <<
        ": event doesn't exist.\n";
    }
    else if(strcmp(bufferRead, "-1") == 0)    {
        logFile << buffLog << "\tRSVP to event id " << eventId <<
        " was already sent.\n";
    }
    else{
        logFile << buffLog << "\tRSVP to event id "<< eventId <<" was received "
                "successfully.\n";
    }
}

void handleRSVPList(string eventId){
    string sendMsg = clientName + " GET_RSVPS_LIST " + eventId;
    handleServerRes(sendMsg);
    bzero(bufferRead,99999);

    checkResults((int)read(sockfd,bufferRead,99998), "read");
    if(strcmp(bufferRead, "ERROR") == 0){
        logFile << buffLog << "\tERROR: handleRSVPList event id doesn't "
                                      "exist.\n";
    }
    else if(strcmp(bufferRead, "EMPTY") == 0){
        logFile << buffLog << "\tThe RSVP's list for event id " << eventId <<
        " is: .\n";
    }
    else{
        vector<string> readList = split(bufferRead, ',');
        sort(readList.begin(), readList.end());
        string listOfNames = "";
        for(auto it = readList.begin(); it!=readList.end(); it++){
            listOfNames += (*it);
            if(it != readList.end()-1) {
                listOfNames += ",";
            }
        }
        logFile << buffLog << "\tThe RSVP's list for event id " << eventId <<
        " is: " << listOfNames <<".\n";
    }
}

int main(int argc, char *argv[])
{
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char bufferInput[99999];

    if (argc < 4) {
        cout << "Usage: emClient clientName serverAddress serverPort\n";
        exit(0);
    }
    portno = atoi(argv[3]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    checkResults(sockfd, "socket");

    server = gethostbyname(argv[2]);
    if (server == NULL) {
        cout << "Usage: emClient clientName serverAddress serverPort\n";
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    checkResults(connect(sockfd,(struct sockaddr *) &serv_addr,
                         sizeof(serv_addr)), "connect");

    //do stuff
    clientName = string(argv[1]);
    time(&rawtime);
    timeInfo = localtime(&rawtime);
    strftime(buffLog, 100, "%I%M%S", timeInfo);
    string logName = clientName + "_" + string(buffLog) + ".log";
    logFile.open(logName, fstream::app | fstream::out);

    int isRegistered = 0;

    while(1) {

        cin.getline(bufferInput, 99999);
        if(strcmp(bufferInput,"") == 0){
            time(&rawtime);
            timeInfo = localtime(&rawtime);
            strftime(buffLog, 100, "%I:%M:%S", timeInfo);
            logFile << buffLog <<
            "\tERROR: illegal command.\n";
            continue;
        }
        vector<string> bufferArray = split(bufferInput, ' ');

        string command = bufferArray[0];
        transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "REGISTER" && !isRegistered) {
            handleRegister();
            isRegistered = 1;
            continue;
        }

        if (isRegistered) {

            if (command == "CREATE") {
                time(&rawtime);
                timeInfo = localtime(&rawtime);
                strftime(buffLog, 100, "%I:%M:%S", timeInfo);
                if (bufferArray.size() < 4) {
                    logFile << buffLog <<
                    "\tERROR: missing arguments in command CREATE.\n";
                    continue;
                }
                if (bufferArray[1].length() > 30) {
                    logFile << buffLog <<
                    "\tERROR: invalid argument eventTitle in command CREATE.\n";
                    continue;
                }
                if (bufferArray[2].length() > 30) {
                    logFile << buffLog <<
                    "\tERROR: invalid argument eventDate in command CREATE.\n";
                    continue;

                }
                string eventDescription = "";
                for(int i = 3; i < (int)bufferArray.size(); i++) {
                    eventDescription += bufferArray[i];
                    if (i != (int)bufferArray.size()-1) {
                        eventDescription += " ";
                    }
                }
                if (eventDescription.length() > 256) {
                    logFile << buffLog <<
                    "\tERROR: invalid argument eventDescription in command "
                            "CREATE.\n";
                    continue;
                }

                handleCreate(bufferArray[1], bufferArray[2], eventDescription);
            }
            else if (command == "GET_TOP_5") {
                handleTop5();
            }
            else if (command == "SEND_RSVP") {
                if(bufferArray.size() != 2){
                    logFile << buffLog <<
                    "\tERROR: missing arguments in command SEND_RSVP.\n";
                    continue;
                }
                handleSendRSVP(bufferArray[1]);
            }
            else if (command == "GET_RSVPS_LIST") {
                if(bufferArray.size() != 2){
                    logFile << buffLog <<
                    "\tERROR: missing arguments in command GET_RSVPS_LIST.\n";
                    continue;
                }
                handleRSVPList(bufferArray[1]);
            }
            else if (command == "UNREGISTER") {
                string sendMsg = clientName + " UNREGISTER";
                handleServerRes(sendMsg);
                bzero(bufferRead, 99999);
                time(&rawtime);
                timeInfo = localtime(&rawtime);
                strftime(buffLog, 100, "%I:%M:%S", timeInfo);
                if(read(sockfd, bufferRead, 99998) < 0){
                    logFile << buffLog << "\tERROR\tread\t"<<errno<<".\n";
                    close(sockfd);
                    exit(1);
                }
                logFile << buffLog << "\tClient " << clientName <<
                " was unregistered successfully.\n";
                close(sockfd);
                exit(0);
            }
            else {
                time(&rawtime);
                timeInfo = localtime(&rawtime);
                strftime(buffLog, 100, "%I:%M:%S", timeInfo);
                logFile << buffLog <<
                "\tERROR: illegal command.\n";
            }
        }
        else {
            time(&rawtime);
            timeInfo = localtime(&rawtime);
            strftime(buffLog, 100, "%I:%M:%S", timeInfo);
            logFile << buffLog <<
            "\tERROR: first command must be: REGISTER.\n";
        }
    }
}
