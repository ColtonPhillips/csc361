#include <stdio.h>
#include <cstdlib>
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
#include <sys/select.h>
#include "rdp_segment.h"
#include <fstream>
#include <streambuf>
#include <algorithm>

/*Global data helped make this program easier to write.*/
int sockfd;
int isn;
char rec_buffer[MAX_RDP_PACKET_SIZE];
char send_buffer[MAX_RDP_PACKET_SIZE];
sockaddr_in sender_addr, rec_addr;
socklen_t sender_len = sizeof(sockaddr_in);
socklen_t rec_len = sizeof(sockaddr_in);
RDPState state; 
int numbytes_sent, numbytes_rec;
char event = 's';
std::string file_s; // i decided to hold the file in a string. meh. 
int window = 500;
int fptr= 0;
rdp_segment chince; 
void sendSync() { // 3W Handshake P1 
	rdp_segment f_seg; // first segment
	initialize_segment(f_seg);
	isn = f_seg.seqno;
	f_seg.SYN = true;
	rdp_payload_set(f_seg, "HSP1");
	strcpy(send_buffer, rdp_string_message(f_seg).c_str()); 

	// rdps sends HandShake1
        if ((numbytes_sent = sendto(sockfd,send_buffer,sizeof(send_buffer), 0,
	 (sockaddr *)&rec_addr, rec_len)) == -1) {
                close(sockfd);
		error("error: in sendto()");
	}
	log_event(event, "SYN", f_seg, sender_addr, rec_addr );

	// rdps waits for HandShake2
	state = HandShake2;
}

void handleHandShake2() {
	if ((numbytes_rec = recvfrom(sockfd, rec_buffer,
	 sizeof(rec_buffer), 0,
	 (sockaddr *) &rec_addr, &rec_len)) == -1) {
		error("on recvfrom()!");
	}

	rdp_segment rec_seg = generate_segment(rec_buffer);
	if (rec_seg.RST == true) {event = 'S'; state = Sync;
		log_event('r', "RST", rec_seg, sender_addr, rec_addr );
	}
	else if (rec_seg.SYN == true && rec_seg.ackno == isn+1) {
		log_event('r', "SYN+ACK", rec_seg, sender_addr, rec_addr );
		//debugprint("HandShake2 Received");
		//print_rdp_message(rec_seg);
		// send HS 3 now
		rdp_segment resp_seg = rec_seg;
		resp_seg.seqno = rec_seg.ackno+1;
		resp_seg.ackno = rec_seg.seqno+1;
		resp_seg.SYN = false;
		rdp_payload_set(resp_seg, "HSP3");
		strcpy(send_buffer, rdp_string_message(resp_seg).c_str());
		if ((numbytes_sent = sendto(sockfd,send_buffer, sizeof(send_buffer), 0,
       		  (sockaddr *) &rec_addr, rec_len)) == 1) {
        		close(sockfd);
                	error ("in sendto()");
        	}
		log_event('s', "ACK", resp_seg, sender_addr, rec_addr );
		state = WaitToSend;
		//debugprint("state change to SEND");
	} else{debugprint("HS2 failed");}
}

void sendData() { 
	
	strcpy(send_buffer, file_s.substr(fptr,window).c_str());
	fptr+=window;
	if ((numbytes_sent = sendto(sockfd,send_buffer, window , 0,
       	  (sockaddr *) &rec_addr, rec_len)) == 1) {
       		close(sockfd);
               	error ("in sendto()");
       	}
}

int main(int argc, char *argv[]) {
	srand(time(NULL));	
	if (argc != 6) {
		error("Error: Use format\nrdps sender_ip sender_port receiver_ip receiver_port sender_file_name\n");
	}

	char *sender_ip = argv[1];
	char *sender_port = argv[2];
	char *receiver_ip = argv[3];
	char *receiver_port = argv[4];
	char *sender_file_name = argv[5];
	
	// just create the file_string right now. 
	std::ifstream myf(sender_file_name);
	std::string file_s((std::istreambuf_iterator<char>(myf)),
        	std::istreambuf_iterator<char>());

	// Create communication endpoint (socket)
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		error("error: on socket()");
	}
	
	// Sender (this file) and Receiver (rdpr)  Socket Address structs
	sender_addr.sin_family = AF_INET;
	sender_addr.sin_addr.s_addr = inet_addr(sender_ip);
	sender_addr.sin_port = htons(atoi(sender_port));
	rec_addr.sin_family = AF_INET;
	rec_addr.sin_addr.s_addr = inet_addr(receiver_ip);
	rec_addr.sin_port = htons(atoi(receiver_port));
	
	// bind to a socket with reusibility option
	int optval = 1;
	setsockopt(sockfd,SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &optval, sizeof(optval));
	if (bind(sockfd,(sockaddr *) &sender_addr,
		sizeof(sender_addr)) < 0) {
		close(sockfd);
		error("error: on binding!");
	}

	debugprint("rdps server is now running");

	fd_set readfds;
	int sret = 0;
	timeval timeout;
	bool quit = false;
	while(!quit) {	
		fflush(stdout);

		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		FD_SET(0,&readfds); // stdin
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		if (state == Sync) {sendSync(); event = 's'; continue;}
		else if (state == Send) {sendData();} 
		else {sret = select(8, &readfds, NULL, NULL, &timeout);}
		
		if (sret == 0) {
			debugprint("Timeout");
			switch (state) {
			case (HandShake2):  {
				state = Sync;
				event = 'S';
				break;}
			}
		}
		else if (sret == -1) {
			error("select");
		}
		if (FD_ISSET(0, &readfds) && scrape_stdin_for_quit()) {
			quit = true;
		}
		if (FD_ISSET(sockfd, &readfds)) {
			switch (state) {
			case HandShake2: {
				handleHandShake2();
				break;}
			case WaitToSend:{
				if ((numbytes_rec = recvfrom(sockfd, rec_buffer,
				 sizeof(rec_buffer), 0,
				 (sockaddr *) &rec_addr, &rec_len)) == -1) {
					error("Send: on recvfrom()!");
				}

				rdp_segment rec_seg = generate_segment(rec_buffer);
				window = rec_seg.size;
				chince = rec_seg;
				rdp_payload_set(chince, "");
				debugprint("wnd");
				debugprint(rdp_string_message(chince).substr(0,rdp_size(chince)-20));
				debugprint(rdp_string_message(chince).substr(0,rdp_size(chince)-10));
				debugprint(rdp_string_message(chince).substr(0,rdp_size(chince)-10));
				debugprint(rdp_string_message(chince).substr(0,rdp_size(chince)));
				error("asdf");
				//print_rdp_message(rec_seg);
				state = Send;
				break;}
			}
		}
	}	
	debugprint("Server is closed");
	close(sockfd);
	return 0;
}	

