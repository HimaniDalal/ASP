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

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 8080
#define MIRROR_PORT 4040
#define MIRROR_IP "127.0.0.1"

//Struct AddressInfo to tranfer Mirror IP & Port no. 
typedef struct {
    char ip_address[INET_ADDRSTRLEN];
    int port_number;
} Addr_info;
 
 int client_count =0;
 //send message to client
 int send_msg(int clientsd, char* msg){
	
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
}


void handle_client(int clientsd) {

    char cmd_buff[MAX_BUFFER_SIZE];
	char responseText[MAX_BUFFER_SIZE];

    while (1){

        // Clear the command buffer
        memset(cmd_buff, 0, sizeof(cmd_buff)); 

        //read the command buffer
        int Bytes_rec = read(clientsd, cmd_buff, MAX_BUFFER_SIZE);

        if (Bytes_rec <= 0) { 
           printf("Disconnected Client Number: %d\n", client_count);
           break; 
		}

        printf("Command from client: %s\n", cmd_buff);
		
		char *cmd_arg[KV_ARGS_MAX];
		int num_arguments = 0;
		
		// Parse the command received from client
		char* token = strtok(cmd_buff, " "); // Tokenize command using space as delimiter
		char* comm_hd = token; // Store the first token in cmd
		
		while (token != NULL) {
			token = strtok(NULL, " "); // Get the next token
			if (token != NULL) { // Check if token is not NULL before storing it
				cmd_arg[num_arguments++] = token; // Store the token in the array
			}
		}
		cmd_arg[num_arguments] = NULL; // Set the last element of the array to NULL	
		
		// Process the command and generate response
		if (strcmp(comm_hd, "getfn") == 0)
		{
			// Call the function to handle request
			getfn(clientsd, cmd_arg);
		}
		else if (strcmp(comm_hd, "tarfgetz") == 0)
		{
			// Call the function to handle request
			int resultsgetfiles = tarfgetz(clientsd, cmd_arg);
			
			// Check the result of the function call
               if (resultsgetfiles == 1) {
					send_msg(clientsd, "Exception while runnig command.");
					printf("Exception while runnig: tarfgetz\n");
				}
		}
		else if (strcmp(comm_hd, "gatfda") == 0)
           {
			   // Call the function to handle request
               int resultdgetfiles = gatfda(clientsd, cmd_arg);
			   
			   // Check the result of the function call
               if (resultdgetfiles == 1) {
					send_msg(clientsd, "Exception while runnig command.");
					printf("Exception while runnig: gatfda\n");
				}
		}
		else if (strcmp(comm_hd, "fgets") == 0)
		{	
			// Call the function to handle request
			int result = hdfgets(clientsd, cmd_arg, num_arguments);
			
			// Check the result of the function call
			if (result == 1) {
				printf("Exception while runnig: hdfgets\n");
			}
		}
		else if (strcmp(comm_hd, "getft") == 0)
		{
   			// Call the function to handle request
			int result = getft(clientsd, cmd_arg, num_arguments);

			// Check the result of the function call
			if (result == 1) {
				
				printf("Exception while runnig: getft\n");
			}

		}

		else if (strcmp(comm_hd, "quitc") == 0)
		{		
			printf("Client entered quit\n");
	
			// Acknoledge quit request and close the socket
            close(clientsd);
            break;
            exit(0); 
		}
		
		else
		{	
			// Invalid command, inform client 
			send_msg(clientsd, "invalid command\n");
			continue;	// Continue to next iteration of loop to wait for new command
		}
        


    }
    
    		

}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    int connect_type, conn_stat;

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening for connections");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    while (1) {

        client_count++;

        char connect_type = kvSwitchClientMirror(client_count);

        // Accept a connection from a client
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }
        printf("Connected");

        
        if (!fork()){	 
			handle_client(client_socket);
			close(client_socket);
			exit(0);
			}
		waitpid(0, &conn_stat, WNOHANG); //returns if there are no child process termination 
		
        //connection to  mirro server
        else if(connectType == 'M') {
			client_socket = accept(server_socket, (struct sockaddr *) NULL, NULL);
			
			
			// Fork a child process to redirect client
			if (!fork()){	
				redirectToMirror(client_socket);
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
