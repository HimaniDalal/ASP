#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>		//to store address information
#include <netinet/ip.h>
#include <sys/socket.h>		//for using socket API's
#include <sys/signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

//MACROS
#define MAX_BUFF 1024
#define MAX_RESPONSE 1024
#define SERVER_PORT 8080
#define MIRROR_PORT 4040
#define MIRROR_IP "127.0.0.1"
#define MAX_ARGS 410

//global variables
const char* HOMEDIR = NULL;
//Struct AddressInfo to tranfer Mirror IP & Port no. 
typedef struct {
    char ip_address[INET_ADDRSTRLEN];
    int port_number;
} Addr_info;
 
 //send message to client
/*int send_msg(int clientsd, char* msg){
	
	// Send response type AS text response
	long response = 1;

		//send response type to client and return error in case of failure
    if (send(clientsd, &response, sizeof(response), 0) == -1) {
        perror("Error sending response type");
        return -1;
    }

    printf("Sending Message to client : %s\n", msg);

    // Send response text to Client
    size_t msglen = strlen(msg);
    ssize_t bytes_send = 0;

    	//while number of bytes sent is less than actual lenth of message send the response to client
    while (bytes_send < msglen) {
        ssize_t no_ofbytes_sent = send(clientsd, msg + bytes_send, msglen - bytes_send, 0);
        if (no_ofbytes_sent == -1) {
            perror("Error sending message");
            return -1;
        }
        bytes_send += no_ofbytes_sent;
    }

    return 0;
}*/

void hdremoveNewLine(char *str) {
    size_t len = strlen(str);

    for (size_t i = 0; i < len; i++) {
       
        if (str[i] == '\n') {
            str[i] = '\0';
        }
    }
}
void handle_client(int clientsd, int client_count) {

    char cmd_buff[MAX_BUFF];
	//char responseText[MAX_BUFF];
    char *cmd_arg[7]; //to store tokens
	int args_count = 0;

    char message[] = "Enter command";
    (send(clientsd, message, sizeof(message), 0));

    for(;;){
       // to reset the buffer to '\0'
        memset(cmd_buff, 0, sizeof(cmd_buff)); 

        //read the command buffer
        int bytes_recv = read(clientsd, cmd_buff, MAX_BUFF);

        if (bytes_recv <= 0) { 
           printf("nothing received from client %d\n", client_count);
           break; 
		}

        printf("Client Query: %s\n", cmd_buff);
		
		//tokenizing command from client
		char* token = strtok(cmd_buff, " "); // using space delomiter to tokenize
		char* base_cmd = token; // Store the first token in cmd
		
		while (token != NULL) {
			token = strtok(NULL, " "); // continue tokenixng
			if (token != NULL) { 
				cmd_arg[args_count++] = token; 
			}
		}
		cmd_arg[args_count] = '\0';
		
		// check for required cmd
		if (strcmp(base_cmd, "getfn") == 0)
		{
			// invoke required function
			getfn(clientsd, cmd_arg); //fn to request file properties
		}
		else if (strcmp(base_cmd, "getfz") == 0)
		{
            getfz(clientsd, cmd_arg);
			/* moify func
            int resultsgetfiles = getfz(clientsd, cmd_arg);

			
			// Check the result of the function call
               if (resultsgetfiles == 1) {
					send_msg(clientsd, "Exception while runnig command.");
					printf("Exception while runnig: getfz\n");
				}
            */
		}

        else if (strcmp(base_cmd, "getft") == 0)
		{
            //fn to pass extension list
            getft(clientsd, cmd_arg, args_count);
   		
			/*int result = getft(clientsd, cmd_arg, args_count);

			// Check the result of the function call
			if (result == 1) {
				
				printf("Exception while runnig: getft\n");
			}*/

		}
		else if (strcmp(base_cmd, "getfda") == 0)
           {
			   // returns newer files
               int resultdgetfiles = getfda(clientsd, cmd_arg);
			   
			   // Check the result of the function call
               if (resultdgetfiles == 1) {
					send_msg(clientsd, "Exception while runnig command.");
					printf("Exception while runnig: getfda\n");
				}
		}

        else if (strcmp(base_cmd, "getfdb") == 0)
           {
			   // returns newer files
               int resultdgetfiles = getfdb(clientsd, cmd_arg);
			   
			   // Check the result of the function call
               if (resultdgetfiles == 1) {
					send_msg(clientsd, "Exception while runnig command.");
					printf("Exception while runnig: getfdb\n");
				}
		}
		/*else if (strcmp(base_cmd, "fgets") == 0)
		{	
			// Call the function to handle request
			int result = hdfgets(clientsd, cmd_arg, args_count);
			
			// Check the result of the function call
			if (result == 1) {
				printf("Exception while runnig: hdfgets\n");
			}
		}*/
		
		else if (strcmp(base_cmd, "quitc") == 0)
		{		
			printf("Exited\n");
            close(clientsd); //close the client socket
            exit(0); 
            break;
		}
		
		else
		{    //not a valid option
			(send(clientsd, "Exited", sizeof(Exited) + 1, 0));
			continue;	// Continue to next iteration of loop to wait for new command
		}
        
    }
    
}

int redirect_mirror(int clientsd, int client_count){

		//structure to hold ip addr adn port of miror
	Addr_info mirr_add;
	
	 // Send response type for the mirror struct
    /*long int response = 2;
    if (send(clientsd, &response, sizeof(response), 0) == -1) {
        perror("Error");
        return -1;
    }*/
    
    // copy ip address and port number to structure 
    strcpy(mirr_add.ip_address, MIRROR_IP);
    mirr_add.port_number = MIRROR_PORT;

    // Send mirror IP and Port to client
    if (send(clientsd, &mirr_add, sizeof(Addr_info), 0) == -1) {
        perror("Mirror side error");
        return -1;
    }

    printf("Redirected Client %d to mirror server \n",client_count);

    return 0;
}

void getfn(int clientsd, char** hdargs) {
	
    char* file_name = hdargs[0]; //here first argument will be the filename 

    char response[MAX_RESPONSE];

    HOMEDIR = getenv("HOME");
    int length = strlen(HOMEDIR) + strlen(file_name);
    char comm_buff[length + 30];
    sprintf(comm_buff, "find %s -name '%s' -print -quit", HOMEDIR, file_name);

    printf("Execute findFile command: %s\n", comm_buff);

    //call 'kvExecuteCommand' function and get output in 'result'
    char* result = kvExecuteCommand(comm_buff);

    //if command executed successfully
    if (result != NULL) {

        struct stat sr; //stat to get info of result
        
        if (stat(result, &sr) == 0) { //success

            time_t file_time = sr.st_mtime; //returns last modified time of file

            char* time = ctime(&file_time); //readable format

            hdremoveNewLine(time);

            //stored the info retrived in response buffer
            sprintf(response, "%s \n File Name: %s\n File Size: %lld bytes\n,Creation date: %s", result,file_name, (long long)sr.st_size, time);
        } else {

            sprintf(response, "Unable to get file information for %s", result);
        }

    } else {

        sprintf(response, "File not found.");
    }

    (send(clientsd, response, sizeof(response), 0));
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_count =0;
    int conn_stat;
    char connect_type, curr_type ;

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error: Socket not created");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
        perror("Socket Binding Error");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Port Listening Error ");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    for(;;) {

        client_count++;

        if (client_count >= 1 && client_count <= 4){

            connect_type == "s";
        }
        else if (client_count >= 5 && client_count <= 8) {

                connect_type == "m";
        }
        
        else if (connect_type == 'm') {
            connect_type = 's';
        } 
        else {
            connect_type = 'm';
        }

        if(connect_type = 's'){
            
        // Accept a connection from a client
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }
        printf("Client %d Connected \n",client_count);

        //child process for client requests
			if (!fork()){	 
				handle_client(client_socket, client_count);
				close(kvClientScoketDes);
				exit(0);
			}
			waitpid(0, &kvConnStatus, WNOHANG); //returns if there are no child process termination 
    
        }
        //char connect_type = kvSwitchClientMirror(client_count);

        //connection to  mirro server
        else if(connectType == 'M') {
			client_socket = accept(server_socket, (struct sockaddr *) NULL, NULL);
            if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }
			
			// Fork a child process to redirect client to mirror server
			if (!fork()){	
				redirect_mirror(client_socket, client_count);
				close(client_socket);
				exit(0);
			}
			waitpid(0, &conn_stat, WNOHANG);
		}
        // Handle the client in a separate function
       // handle_client(client_socket);
    }
    // Close the server socket
    close(server_socket);

    return 0;
}
