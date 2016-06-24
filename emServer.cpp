/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <tic.h>

#include <sstream>

using namespace std;
vector<string> clients_names;


/*was created to measure time for the log*/
char buffLog[100];
fstream logFile;
time_t rawtime;
struct tm* timeInfo;

int counter_event = 1;

typedef struct{
    int id;
    string event_name;
    string event_date;
    string event_desc;
    vector<string> clientsRsvpd;
}event;


vector<event*> events_list;

vector<pthread_t> threads;

// mutexes
pthread_mutex_t clients_names_mutex;
pthread_mutex_t log_mutex;
pthread_mutex_t counter_event_mutex;
pthread_mutex_t events_list_mutex;
pthread_mutex_t threads_mutex;

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
        checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
        time(&rawtime);
        timeInfo = localtime(&rawtime);
        strftime(buffLog, 100, "%I:%M:%S", timeInfo);
        logFile << buffLog << "\tERROR\t" <<functionName<< "\t"<<errno<<".\n";
        checkResults(pthread_mutex_unlock(&log_mutex), "pthread_mutex_unlock");
        exit(1);
    }
}



int emRegister(string clientName) {
    checkResults(pthread_mutex_lock(&clients_names_mutex), "pthread_mutex_lock");
    for(auto it = clients_names.begin(); it != clients_names.end(); ++it){
        if((*it == clientName)){
            checkResults(pthread_mutex_unlock(&clients_names_mutex),
                         "pthread_mutex_unlock");
            return -1;
        }
    }
    clients_names.push_back(clientName);
    checkResults(pthread_mutex_unlock(&clients_names_mutex),
                 "pthread_mutex_unlock");
    return 0;
}

int emCreate(string eventTitle, string eventDate, string eventDescription) {
    event *emEvent = new event;
    checkResults(pthread_mutex_lock(&counter_event_mutex), "pthread_mutex_lock");
    emEvent->id = counter_event++;
    checkResults(pthread_mutex_unlock(&counter_event_mutex),
                 "pthread_mutex_unlock");
    emEvent->event_date = eventDate;
    emEvent->event_desc = eventDescription;
    emEvent->event_name = eventTitle;
    checkResults(pthread_mutex_lock(&events_list_mutex), "pthread_mutex_lock");
    events_list.push_back(emEvent);
    checkResults(pthread_mutex_unlock(&events_list_mutex),
                 "pthread_mutex_unlock");
    return emEvent->id;
}

string emTop5(){
    string top5 = "";
    int i = 0;
    checkResults(pthread_mutex_lock(&events_list_mutex), "pthread_mutex_lock");
    for(auto it = events_list.rbegin(); it != events_list.rend(); it++){
        if(i == 5){
            break;
        }
        top5 += to_string((*it)->id) +","+(*it)->event_name+","+
                (*it)->event_date+","+(*it)->event_desc+",|,";
        i++;
    }
    checkResults(pthread_mutex_unlock(&events_list_mutex),
                 "pthread_mutex_unlock");
    return top5;
}


int emSendRsvp(string eventId, string clientName){
    checkResults(pthread_mutex_lock(&events_list_mutex), "pthread_mutex_lock");
    if((int)events_list.size() < stoi(eventId)) {
        checkResults(pthread_mutex_unlock(&events_list_mutex),
                     "pthread_mutex_unlock");
        return -2;
    }
    //if the rsvp already been sent to the client
    for (auto it = events_list[stoi(eventId)-1]->clientsRsvpd.begin();
     it != events_list[stoi(eventId)-1]->clientsRsvpd.end(); it++){
        if(*it == clientName){
            checkResults(pthread_mutex_unlock(&events_list_mutex),
                         "pthread_mutex_unlock");
            return -1;
    }
}
    events_list[stoi(eventId)-1]->clientsRsvpd.push_back(clientName);
    checkResults(pthread_mutex_unlock(&events_list_mutex),
                 "pthread_mutex_unlock");
    return 0;
}


string emGetRsvpList(string eventId){
    string clientsNames = "";
    checkResults(pthread_mutex_lock(&events_list_mutex), "pthread_mutex_lock");
    if((int)events_list.size() < stoi(eventId)){
        checkResults(pthread_mutex_unlock(&events_list_mutex),
                     "pthread_mutex_unlock");
        return clientsNames;
    }
    for (auto it = events_list[stoi(eventId)-1]->clientsRsvpd.begin();
         it != events_list[stoi(eventId)-1]->clientsRsvpd.end(); it++){
        clientsNames += (*it);
        if(it != events_list[stoi(eventId)-1]->clientsRsvpd.end() -1){
            clientsNames += ",";
        }
    }
    if(clientsNames == ""){
        clientsNames = "EMPTY";
    }
    checkResults(pthread_mutex_unlock(&events_list_mutex),
                 "pthread_mutex_unlock");
    return clientsNames;
}


void emUnregister(string clientName){
    checkResults(pthread_mutex_lock(&events_list_mutex), "pthread_mutex_lock");
    for(auto it = events_list.begin(); it != events_list.end(); it++){
        int toDelete = -1;
        for(int i = 0; i < (int)(*it)->clientsRsvpd.size(); i++){
            if( (*it)->clientsRsvpd[i] == clientName){
                toDelete = i;
                break;
            }
        }
        if(toDelete >= 0) {
            (*it)->clientsRsvpd.erase((*it)->clientsRsvpd.begin() + toDelete);
        }
    }
    checkResults(pthread_mutex_unlock(&events_list_mutex),
                 "pthread_mutex_unlock");
    //remove client name from the clients list
    int index = 0;
    checkResults(pthread_mutex_lock(&clients_names_mutex), "pthread_mutex_lock");
    for(int i = 0; i < (int)clients_names.size(); i++){
        if(clients_names[i] == clientName){
            index = i;
            break;
        }
    }
    clients_names.erase(clients_names.begin()+ index);
    checkResults(pthread_mutex_unlock(&clients_names_mutex),
                 "pthread_mutex_unlock");
}

void* handle_command(void *buffer) {
    int res;
    string buff;
    vector<string> buffer_array = split(*(string*)buffer, ' ');

    string clientName = buffer_array[0];
    int sockfd = stoi(buffer_array[buffer_array.size()-1]);
    transform(clientName.begin(), clientName.end(), clientName.begin(),
              ::tolower);
    transform(buffer_array[1].begin(), buffer_array[1].end(),
              buffer_array[1].begin(), ::toupper);

    if(buffer_array[1] == "REGISTER") {
        checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
        time(&rawtime);
        timeInfo = localtime(&rawtime);
        strftime(buffLog, 100, "%I:%M:%S", timeInfo);
        if (emRegister(clientName) < 0) {
            buff = "ERROR";
            logFile << buffLog << "\tERROR: " <<clientName<<
                    "\tis already exists.\n";
        }
        else {
            buff = "OK";
            logFile << buffLog << "\t" <<clientName<<
                    "\twas registered successfully.\n";
        }
        checkResults(pthread_mutex_unlock(&log_mutex), "pthread_mutex_unlock");
        checkResults((int)write(sockfd, buff.c_str(), strlen(buff.c_str())),
                     "write");

    }

    if(buffer_array[1] == "CREATE"){
        checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
        string eventTitle = buffer_array[2];
        string eventDate = buffer_array[3];
        string eventDescription = "";
        for(int i = 4; i < (int)buffer_array.size() - 1; i++) {
            eventDescription += buffer_array[i];
            if (i != (int)buffer_array.size()-2) {
                eventDescription += " ";
            }
        }

        int id =  emCreate(eventTitle, eventDate, eventDescription);
        time(&rawtime);
        timeInfo = localtime(&rawtime);
        strftime(buffLog, 100, "%I:%M:%S", timeInfo);
        if(id < 0){
            buff = "ERROR";
        }
        else{
            buff = to_string(id);
            logFile << buffLog <<"\t" << clientName << "\tevent id "<<
                    to_string(id)
            << " was assigned to the event with title " << eventTitle<<".\n";
        }

        checkResults(pthread_mutex_unlock(&log_mutex), "pthread_mutex_unlock");
        checkResults((int)write(sockfd, buff.c_str(), strlen(buff.c_str())),
                     "write");

    }

    if(buffer_array[1] == "GET_TOP_5"){
        checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
        string top5 = emTop5();
        time(&rawtime);
        timeInfo = localtime(&rawtime);
        strftime(buffLog, 100, "%I:%M:%S", timeInfo);
        buff = top5;
        logFile << buffLog << "\t" << clientName << "\trequests the top 5 newest "
                                                            "events.\n";
        checkResults(pthread_mutex_unlock(&log_mutex), "pthread_mutex_unlock");
        if(top5 == ""){
            buff = "EMPTY";
        }
        checkResults((int)write(sockfd, buff.c_str(), strlen(buff.c_str())),
                     "write");
    }

    if(buffer_array[1] == "SEND_RSVP") {
        checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
        string eventId = buffer_array[2];
        res = emSendRsvp(eventId, clientName);
        time(&rawtime);
        timeInfo = localtime(&rawtime);
        strftime(buffLog, 100, "%I:%M:%S", timeInfo);
        if(res == -2){
            buff = "-2";
            logFile << buffLog << "\tERROR: handle_command event id doesn't "
                                          "exist.\n";
        }
        else if(res == -1){
            buff = "-1";
            logFile << buffLog << "\tERROR: handle_command client already "
                                          "rsvpd.\n";

        }
        else{
            buff = "0";
            logFile << buffLog << "\t" << clientName << "\tis RSVP to event "
                                                                "with id "
            <<eventId<< ".\n";
        }
        checkResults(pthread_mutex_unlock(&log_mutex), "pthread_mutex_unlock");
        checkResults((int)write(sockfd, buff.c_str(), strlen(buff.c_str())),
                     "write");
    }

    if(buffer_array[1] == "GET_RSVPS_LIST") {
        checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
        string eventId = buffer_array[2];
        time(&rawtime);
        timeInfo = localtime(&rawtime);
        strftime(buffLog, 100, "%I:%M:%S", timeInfo);
        string rsvpList = emGetRsvpList(eventId);
        if(rsvpList == "") {
            rsvpList = "ERROR";
            logFile << buffLog <<
            "\tERROR: handle_command event id doesn't exist.\n";
        }
        else{
            logFile << buffLog <<"\t" << clientName <<
                    "\trequests the RSVP's list for event with id " <<
                    eventId<< ".\n";
        }
        checkResults(pthread_mutex_unlock(&log_mutex), "pthread_mutex_unlock");
        checkResults((int)write(sockfd, rsvpList.c_str(),
                                strlen(rsvpList.c_str())), "write");
    }
    if(buffer_array[1] == "UNREGISTER") {
        emUnregister(clientName);
        buff = "ok";
        checkResults((int)write(sockfd, buff.c_str(), strlen(buff.c_str())),
                     "write");
        logFile << buffLog << "\t" << clientName <<
                "\twas unregistered successfully.\n";
    }

    int index = 0;
    checkResults(pthread_mutex_lock(&threads_mutex), "pthread_mutex_lock");
    for(int i = 0; i < (int)threads.size(); i++){
        if(threads[i] == pthread_self()){
            index = i;
            break;
        }
    }
    threads.erase(threads.begin() + index);
    checkResults(pthread_mutex_unlock(&threads_mutex), "pthread_mutex_unlock");
    pthread_exit(nullptr);
}

void* handle_client(void* sockfd) {
    char buffer[99999];
    pthread_t pthread;
    while(1) {
        bzero(buffer, 99999);
        checkResults((int)read(*(int*)sockfd,buffer,99998), "read");
        string strBuff(buffer);
        strBuff.append(" ");
        strBuff.append(to_string(*(int*)sockfd));
        checkResults(pthread_create(&pthread, NULL, handle_command, (void *)
                &strBuff), "pthread_create");
        checkResults(pthread_mutex_lock(&threads_mutex), "pthread_mutex_lock");
        threads.push_back(pthread);
        checkResults(pthread_mutex_unlock(&threads_mutex),
                     "pthread_mutex_unlock");
        checkResults(pthread_join(pthread, NULL), "pthread_join");

        vector<string> command = split(strBuff, ' ');

        if(command[1] == "UNREGISTER") {
            close(*(int*)sockfd);
            pthread_exit(nullptr);
        }
    }

}

void* server_input(void* sockfd) {
    while(true) {
        string command;
        cin >> command;

        if (command == "EXIT") {
            checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
            time(&rawtime);
            timeInfo = localtime(&rawtime);
            strftime(buffLog, 100, "%I:%M:%S", timeInfo);
            logFile << buffLog <<
            "\tEXIT command is typed: server is shutdown.\n";
            checkResults(pthread_mutex_unlock(&log_mutex),
                         "pthread_mutex_unlock");

            for (auto it = threads.begin(); it != threads.end(); ++it) {
                checkResults(pthread_join(*it, NULL), "pthread_join");
            }

            checkResults(pthread_mutex_lock(&events_list_mutex),
                         "pthread_mutex_lock");
            for (auto it = events_list.begin(); it != events_list.end(); ++it) {
                delete *it;
            }
            checkResults(pthread_mutex_unlock(&events_list_mutex),
                         "pthread_mutex_unlock");

            pthread_mutex_destroy(&clients_names_mutex);
            pthread_mutex_destroy(&log_mutex);
            pthread_mutex_destroy(&counter_event_mutex);
            pthread_mutex_destroy(&events_list_mutex);
            pthread_mutex_destroy(&threads_mutex);

            events_list.clear();
            clients_names.clear();

            close(*(int *) sockfd);

            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    pthread_t pthread;
    struct sockaddr_in serv_addr, cli_addr;
    logFile.open("emServer.log", fstream::app | fstream::out);
    if (argc < 2) {
        cout<<"Usage: emServer portNum\n";
        exit(1);
    }

    pthread_mutex_init(&clients_names_mutex, NULL);
    pthread_mutex_init(&log_mutex, NULL);
    pthread_mutex_init(&counter_event_mutex, NULL);
    pthread_mutex_init(&events_list_mutex, NULL);
    pthread_mutex_init(&threads_mutex, NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    checkResults(sockfd, "socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    checkResults(bind(sockfd, (struct sockaddr *) &serv_addr,
                      sizeof(serv_addr)), "bind");
    listen(sockfd,10);
    clilen = sizeof(cli_addr);

    checkResults(pthread_create(&pthread, NULL, server_input, (void*)&sockfd),
                 "pthread_create");

    while(1){
        newsockfd = accept(sockfd,
                           (struct sockaddr *) &cli_addr,
                           &clilen);

        checkResults(newsockfd, "accept");
        if(pthread_create(&pthread, NULL, handle_client,
                          (void *) &newsockfd) < 0) {
            checkResults(pthread_mutex_lock(&log_mutex), "pthread_mutex_lock");
            time(&rawtime);
            timeInfo = localtime(&rawtime);
            strftime(buffLog, 100, "%I:%M:%S", timeInfo);
            logFile << buffLog << "\tERROR\tpthread_create\t" <<errno <<".\n";
            checkResults(pthread_mutex_unlock(&log_mutex),
                         "pthread_mutex_unlock");
            exit(1);
        }

    }
}
