#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string>
#include <iostream>

#define MAXBUFLEN 1024

// we use this after sending a response
const char * get_time_string() {
	struct tm *tm;
	time_t t;
	char str_date[100];

	t = time(NULL);
	tm = localtime(&t);

	strftime(str_date, sizeof(str_date), "%b %d %H:%M:%S", tm);
	return std::string(str_date).c_str();
}


// Returns 0 for OK, or -1 for Bad Request
int request_status(std::string request_line,std::string method,
		std::string uri,std::string version) {
	if(uri.length() < 1) {return -1;} // empty uri
	else if(strcasecmp(method.c_str(),"GET") != 0) {return -1;}
	else if(strcasecmp(version.c_str(),"HTTP/1.0") != 0) {return -1;}
	else { return 0;}
}	

int main(int argc, char *argv[]) {

	int sockfd, portno;
	socklen_t cli_len;
	char req_buffer[MAXBUFLEN];
	char resp_buffer[MAXBUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	cli_len = sizeof(cli_addr);
	int numbytes_req, numbytes_resp;
	fd_set rfds;
	struct timeval tv = { 0L, 0L };
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	
	if (argc != 3){
        	printf( "Usage: %s <port> <destination>\n", argv[0] );
        	fprintf(stderr,"ERROR, wrong # of arguments!\n");
        	return -1;
     	}
	char *serv_dir = argv[2];

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("sws: error on socket()");
		return -1;
	}
	//printf("established sockfd: %i\n", sockfd);

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("10.10.1.100");
	serv_addr.sin_port = htons(portno); // byte order
	
	// bind to a socket with reusability option
	int optval = 1;
	setsockopt(sockfd,SOL_SOCKET, (SO_REUSEPORT|SO_REUSEADDR), &optval,sizeof(optval));
	if (bind(sockfd,(struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) {
		close(sockfd);
		perror("sws: error on binding!");
		return -1;
	}
	
	printf("sws is running on UDP port %i and serving %s\nPress q + ENTER to quit\n",
		portno, serv_dir);

	bool quit = false;
	while(!quit) {
		fflush(stdout);
		// receive then format a packet
		if ((numbytes_req = recvfrom(sockfd, req_buffer, MAXBUFLEN-1, 0,
				(struct sockaddr *)&cli_addr, &cli_len)) == -1) {
			perror("sws: error on recvfrom()!");
			close(sockfd);
			return -1;
		}
		req_buffer[numbytes_req]='\0';
		
		// parse response
		// example req: "GeT /test.txt HtTp/1.0" 
		std::string request_line = std::string(req_buffer);
		int first_space = request_line.find_first_of(" \t");
		int first_new_l = request_line.find_first_of("\r\n");
		// use iteration to find first non ws before first newl
		int i = first_new_l-1;
		while (i > 0) {
			if (isspace(request_line[i]))
				break;
			i--;
		}
		int last_space  = i;
		std::string method = request_line.substr(0,first_space);
		size_t uri_len = last_space-1-method.length();
		std::string uri = request_line.substr(first_space+1,uri_len);
		size_t version_len = first_new_l-method.length()-uri.length()-2;
		std::string version = request_line.substr(last_space+1,version_len);

		// correct a special case
		if (uri == "/") {
			uri = "/index.html";
		}

		// get ready to send response
		std::string file_name_to_print = uri; 
		std::string response_string = "HTTP/1.0 ";
		int status = request_status(request_line,method,uri,version);	
		if (status) { // bad request
			response_string+= "400 Bad Request\n";
		} else if (uri.find("../") != std::string::npos) { // cannot access root
			response_string+= "404 Not Found\n";
		} else { 
			// get file contents
			char dir_buffer[MAXBUFLEN]; 
			strcpy(dir_buffer,serv_dir);
			strcat(dir_buffer,uri.c_str());
			
			if( access( dir_buffer, F_OK ) != -1 ) {
 				// file exists
				char *file_contents;
				long input_file_size;
				FILE *input_file = fopen(dir_buffer, "rb");
				fseek(input_file, 0, SEEK_END);
				input_file_size = ftell(input_file);
				rewind(input_file);
				file_contents = (char*) malloc((input_file_size+1) * (sizeof(char)));
				fread(file_contents, sizeof(char), input_file_size, input_file);
				fclose(input_file);
				file_contents[input_file_size] = 0;	
				response_string+= "200 OK\n\n";
				response_string+= file_contents;

				//needed for server output
				file_name_to_print = std::string(dir_buffer);
			} else {
				// file doesn't exist
				response_string+= "404 Not Found\n";
			}	

		}
		// the actual message we send
		strcpy(resp_buffer,response_string.c_str());		
		resp_buffer[response_string.length()] = 0; 

		// send back a packet
		if ((numbytes_resp = sendto(sockfd,resp_buffer,strlen(resp_buffer), 0,
				(struct sockaddr *)&cli_addr, cli_len)) == -1) {
			perror("sws: error in sendto():");
			close(sockfd);
			return -1;
		}

		// print details of msg and response
		req_buffer[numbytes_req-1]=0; 		// format to print
		resp_buffer[response_string.find("\n")]=0; // format to print
		printf("%s %s:%d %s; %s; %s\n",get_time_string(),
			inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
			req_buffer, resp_buffer,file_name_to_print.c_str());

		// Quit Server? 
		// You have to PRESS ENTER AND receive a packet to register a quit !!!
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		int retval = select(1,&rfds, NULL, NULL, &tv);
		if (retval) {	
			char buff[MAXBUFLEN] = {0};
			int len;
			fgets(buff, sizeof(buff), stdin);
			len = strlen(buff) - 1;
			if (buff[len] == '\n')
				buff[len] = '\0';
			printf("\n! server operator input: %s\n",buff);
			if (strchr(buff,'Q')!= NULL || strchr(buff,'q')!= NULL) {
				quit = true;
			}
		}

	}
	printf("\nServer is closed.\n");
	close(sockfd);
	return 0;
}
