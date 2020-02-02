/*
    Author: Jeff Wilson
    Release Date: 4 December 2019

    Server side of the C2C appliation that will handle and maintain a list of active agents.
    List of commands that the agent can use via their own command line args:
        #JOIN: Agent is requesting to join the server.
            Server will reply with "$OK" or "$ALREADY MEMBER" depending on the agents previous status with server.
        #LEAVE: Agent is requesting to leave the server.
            Server will reply with "$OK" or "$NOT MEMBER" depending on the agents previous status with server.
        #LIST: Agent is requesting the server to list the server status (Agent must be active on server).
            Server will display a list of all active agents on the server.
            FORMATTING:
                <agent_IP, seconds_online>
                <next_agent_IP, seconds online>
        #LOG: Agent is requesting to be sent a log file from the server.
            The server will be maintaining a log file (log.txt) and can send it at an Agents request.
            FORMATTING (using example IP Addresses):
                "TIME": Received a "#JOIN" action from agent "147.26.143.22"
                "TIME": Responded to agent "147.26.143.22" with "$OK"
                "TIME": Received a "#LIST" from agent "147.26.143.22"
                "TIME": No response is supplied to agent "147.26.12.11" (agent not active)
            
*/

// TODO: format_list_entry() returns address of local variable/generates compiler warning. Fix! Might need malloc(). USING -w when compiling for now.

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
// Server API Preprocessor Directives
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 

typedef int bool;
#define true 1
#define false 0

#define BACKLOG_QUEUE_SIZE 10
#define MAX_MESSAGE_SIZE 256
#define INIT_ERROR 3
#define CONNECT_ERROR 2
#define MAX_AGENTS 10
#define TIME_BUFFER_SIZE 32
#define LOG_ENTRY_BUFFER_SIZE 1024

void debug_server_info(int);
int get_elapsed_time(time_t, time_t);
char *format_list_entry(char *, int);
char *get_server_time();

struct Connected_Agent
{
    time_t connect_Start; // Time the Agent connected to the server (epoch relative)
    char IP_Addr[INET_ADDRSTRLEN]; // Store Client IP
    bool Active; // False = Can Overwite this ENTIRE AGENT
};

// Will be used for the timestamp in the LOG files
struct DateAndTime
{
    int year;
    int month;
    int day;
    int hour;
    int minutes;
    int seconds;
    int msec;
};


int main(int argc, char const *argv[])
{

    if(argc != 2) {
        fprintf(stderr, "Usage: ./server port# \n");
        exit(INIT_ERROR);
    }

    int sockfd, newsockfd, portnum;
    socklen_t clilen;
    char message[MAX_MESSAGE_SIZE];
    struct sockaddr_in serv_addr, client_addr;
    int IO_handler; // Will be used to check if the data is actually being passed between agent/server
    struct Connected_Agent Master_Active_Agents_List[MAX_AGENTS];

    char serverTime[TIME_BUFFER_SIZE];
    strcpy(serverTime, get_server_time());

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("FATAL ERROR! Could not open socket. \n");
        exit(INIT_ERROR);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portnum = atoi(argv[1]);

    // Set up Socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; 
    serv_addr.sin_port = htons(portnum);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
         fprintf(stderr, "Fatal Error. Could not bind. \n");
         exit(CONNECT_ERROR);
    }
    listen(sockfd, BACKLOG_QUEUE_SIZE);
    clilen = sizeof(client_addr);

    // DEBUG ONLY.
    //debug_server_info(portnum);

    // Setting up for logging

    FILE *fp;

    // Initialize MASTER list to 0
    int i = 0;
    for(i; i < MAX_AGENTS; i++){
        Master_Active_Agents_List[i].Active = false;
        Master_Active_Agents_List[i].connect_Start = time(NULL);
        strcpy(Master_Active_Agents_List[i].IP_Addr, "-1.1.1.1");
    }
             
    // Begin main loop
    while(true) 
    {
        fp = fopen("log.txt", "a+");
        newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &clilen);
        if(newsockfd < 0) {
            error("Error. Could not accept incoming client.");
            goto end;
        }
        bzero(message, MAX_MESSAGE_SIZE);
        
        IO_handler = read(newsockfd, message, MAX_MESSAGE_SIZE);
        if(IO_handler < 0) {
            error("Error. Failed to read from socket.");
            goto end;
        }   

        if(strcmp(message, "#JOIN") == 0) {
            strcpy(serverTime, get_server_time());
            char IP_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), IP_str, INET_ADDRSTRLEN);
            fprintf(fp, "%s: Received a \"#JOIN\" action from agent \"%s\" \n", serverTime, IP_str); // Move to logging later
            bool already_member = false;
            int index = 0;
            for(index; index < MAX_AGENTS; index++)
            {
                if(strcmp(Master_Active_Agents_List[index].IP_Addr, IP_str) == 0 && Master_Active_Agents_List[index].Active == true) // If already in list and active, deny entry to server.
                {
                    strcpy(serverTime, get_server_time());
                    write(newsockfd,"$ALREADY MEMBER", 15);
                    fprintf(fp, "%s: Responded to agent \"%s\" with \"$ALREADY MEMBER\" \n", serverTime, IP_str);
                    already_member = true;
                    goto end;
                }
                
            }
            if(already_member == false)
            {
                index = 0;
                do
                {
                    if(Master_Active_Agents_List[index].Active != true) // Find any entry that isn't active and replace it with connecting agent
                    {
                        strcpy(Master_Active_Agents_List[index].IP_Addr, IP_str);
                        Master_Active_Agents_List[index].Active = true;
                        Master_Active_Agents_List[index].connect_Start = time(NULL);
                        strcpy(serverTime, get_server_time());
                        write(newsockfd,"$OK", 3);
                        fprintf(fp, "%s: Responded to agent \"%s\" with \"$OK\" \n", serverTime, IP_str);
                        goto end;
                    }
                    index++;
                } while(index < MAX_AGENTS);
            }
            
        }       
        else if(strcmp(message, "#LEAVE") == 0) {
            strcpy(serverTime, get_server_time());
            char IP_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), IP_str, INET_ADDRSTRLEN);
            fprintf(fp, "%s: Received a \"#LEAVE\" action from agent \"%s\" \n", serverTime, IP_str); // Move to logging later
            int index = 0;
            for(index; index < MAX_AGENTS; index++)
            {
                if(strcmp(Master_Active_Agents_List[index].IP_Addr, IP_str) == 0 && Master_Active_Agents_List[index].Active == true) // If already in list and active, deny entry to server.
                {
                    Master_Active_Agents_List[index].Active = false;
                    strcpy(serverTime, get_server_time());
                    write(newsockfd,"$OK", 3);
                    fprintf(fp, "%s: Responded to agent \"%s\" with \"$OK\" \n", serverTime, IP_str);
                    goto end;
                }
                
            }
            strcpy(serverTime, get_server_time());
            write(newsockfd,"$NOT MEMBER", 11);
            fprintf(fp, "%s: Responded to agent \"%s\" with \"$NOT MEMBER\" \n", serverTime, IP_str);
        }
        else if(strcmp(message, "#LIST") == 0) {
            strcpy(serverTime, get_server_time());
            time_t request_time = time(NULL);
            char IP_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), IP_str, INET_ADDRSTRLEN);
            fprintf(fp, "%s: Received a \"#LIST\" action from agent \"%s\" \n", serverTime, IP_str); 

            int index = 0;
            for(index; index < MAX_AGENTS; index++)
            {
                if(strcmp(Master_Active_Agents_List[index].IP_Addr, IP_str) == 0 && Master_Active_Agents_List[index].Active == true) // Is requesting agent active?
                {
                    int parser = 0;
                    int seconds_online = 0;
                    char list_entry[TIME_BUFFER_SIZE];
                    for(parser; parser < MAX_AGENTS; parser++)
                    {
                        // printf("%s %i\n", Master_Active_Agents_List[parser].IP_Addr, Master_Active_Agents_List[parser].Active); // DEBUG ONLY
                        // printf("PARSER: %i \n", parser); // DEBUG ONLY
                        if(Master_Active_Agents_List[parser].Active == true)
                        {   // printf("HIT! \n"); // DEBUG ONLY
                            seconds_online = get_elapsed_time(Master_Active_Agents_List[parser].connect_Start, request_time);
                            strcpy(list_entry, format_list_entry(Master_Active_Agents_List[parser].IP_Addr, seconds_online));
                            // printf("\n\nLIST ENTRY: %s \n\n", list_entry); // DEBUG ONLY
                            write(newsockfd, list_entry, strlen(list_entry)); 
                        }
                        
                    }
                    
                    strcpy(serverTime, get_server_time());
                    fprintf(fp, "%s: Sent a list to agent \"%s\" \n", serverTime, IP_str); 
                    goto end;
                }
                
            }
            strcpy(serverTime, get_server_time());
            fprintf(fp, "%s: No response is supplied to agent \"%s\" (agent not active) \n", serverTime, IP_str);
        }
        else if(strcmp(message, "#LOG") == 0) {
            strcpy(serverTime, get_server_time());
            char IP_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), IP_str, INET_ADDRSTRLEN);
            fprintf(fp, "%s: Received a \"#LOG\" action from agent \"%s\" \n", serverTime, IP_str); 

            int index = 0;
            for(index; index < MAX_AGENTS; index++)
            {
                if(strcmp(Master_Active_Agents_List[index].IP_Addr, IP_str) == 0 && Master_Active_Agents_List[index].Active == true) // Is this agent active?
                {
                    strcpy(serverTime, get_server_time());
                    fprintf(fp, "%s: Sent log.txt to agent \"%s\" \n", serverTime, IP_str); 

                    char line[LOG_ENTRY_BUFFER_SIZE] = "";
                    FILE* file = fopen("log.txt", "r");
                    
                    int len = 0;
                    int total_bytes = 0;
                    while (fgets(line, sizeof(line), file)) {
                        
                        // Strip the newline character
                        len = strlen(line);
                        total_bytes += len;
                        if((total_bytes) < 1024)
                        {
                            write(newsockfd, line, len); 
                        }
                        else {
                            total_bytes -= len;
                            while(total_bytes < 1024)
                            {
                                write(newsockfd, '\0', 1);
                                total_bytes++;
                            }
                            total_bytes = len;
                            write(newsockfd, line, len); 
                        }
                        
                    }
                    fclose(file);
                    goto end;
                }
                
            }
            strcpy(serverTime, get_server_time());
            fprintf(fp, "%s: No response is supplied to agent \"%s\" (agent not active) \n", serverTime, IP_str);
        }
        else{
            goto end;
        }
        
        end:
            fclose(fp);
            close(newsockfd);
    }
    
    // This should not be hit, the server will keep listening until we kill it from the command line.
    return 0;
}

/*
    ----- TO BE USED FOR DEBUGGING ONLY -----
    Function Name: debug_server_info()
    Return Type: void
    Parameters: 
        - portnum: (int) server port number

    This function is to be used for debugging only. It will be used to print the initial server info once the server is successfully created.

    Author: Jeff Wilson
*/

void debug_server_info(int portnum)
{
    printf("\n===================================\n");
    printf("EROS IP Address: 147.26.231.153 \n");
    printf("ZEUS IP Address: 147.26.231.156 \n\n");
    printf("Server Port: %i \n\n\n", portnum);
}

/*
    Function Name: get_time_elapsed()
    Return Type: int
    Parameters: 
        - start (clock_t): Time when agent joined the active list.
        - end (clock_t): Time when the list request was received.

    Will return the time elapsed from when an Agent joined the active list, to when the list request was recived. This will use a bit of math.

    Author: Jeff Wilson
*/

int get_elapsed_time(time_t start, time_t end)
{
    int elapsed_time = (end - start);
    return elapsed_time;
}

/*
    Function Name: format_list_entry()
    Return Type: char*
    Parameters: 
        - agent_IP (char*): The string of the IP Address.
        - seconds_online (double): The amount of time the agent has been active on the server.

    Will prepare the individual elements into a single buffer that will be returned in the following format. <agent_IP, seconds_online>
    This buffer will be used to write out to the requesting agent the contents of the server list.

    Author: Jeff Wilson
*/

char *format_list_entry(char *agent_IP, int seconds_online)
{
    char time_string[INET_ADDRSTRLEN];
    snprintf (time_string, sizeof(time_string), "%i", seconds_online);
    char return_string[TIME_BUFFER_SIZE] = "\n<";
    strcat(return_string, agent_IP);
    strcat(return_string, ", ");
    strcat(return_string, time_string);
    strcat(return_string, ">");
    strcat(return_string, "\n");

    return return_string;
}

/*
    Function Name: print_server_time()
    Return Type: none
    Parameters: 
        None

    Using a user-defined struct. Function will print the current date and time for the log file.

    Author: Jeff Wilson
*/

char *get_server_time()
{
    
    struct DateAndTime date_and_time;
    struct timeval tv;
    struct tm *tm;

    char dtString[TIME_BUFFER_SIZE] = "";
    char time_string[8];

    gettimeofday(&tv, NULL);

    tm = localtime(&tv.tv_sec);

    date_and_time.year = tm->tm_year + 1900;
    snprintf (time_string, sizeof(time_string), "%04d", date_and_time.year);
    strcat(dtString, time_string);
    strcat(dtString, "-");
    date_and_time.month = tm->tm_mon + 1;
    snprintf (time_string, sizeof(time_string), "%02d", date_and_time.month);
    strcat(dtString, time_string);
    strcat(dtString, "-");
    date_and_time.day = tm->tm_mday;
    snprintf (time_string, sizeof(time_string), "%02d", date_and_time.day);
    strcat(dtString, time_string);
    strcat(dtString, " ");
    date_and_time.hour = tm->tm_hour;
    snprintf (time_string, sizeof(time_string), "%02d", date_and_time.hour);
    strcat(dtString, time_string);
    strcat(dtString, ":");
    date_and_time.minutes = tm->tm_min;
    snprintf (time_string, sizeof(time_string), "%02d", date_and_time.minutes);
    strcat(dtString, time_string);
    strcat(dtString, ":");
    date_and_time.seconds = tm->tm_sec;
    snprintf (time_string, sizeof(time_string), "%02d", date_and_time.seconds);
    strcat(dtString, time_string);
    strcat(dtString, ".");
    date_and_time.msec = (int) (tv.tv_usec / 1000);
    snprintf (time_string, sizeof(time_string), "%03d", date_and_time.msec);
    strcat(dtString, time_string);

    return dtString;
}