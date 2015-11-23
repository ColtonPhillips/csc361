#include <iostream>
#include <cstdlib>
#include <string>
#include "rdp_segment.h"
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
#include <sys/select.h>

int sockfd;
int isn;
char rec_buffer[MAX_RDP_PACKET_SIZE];
char send_buffer[MAX_RDP_PACKET_SIZE];
sockaddr_in rec_addr, sender_addr;
socklen_t rec_len = sizeof(sockaddr_in);
socklen_t sender_len = sizeof(sockaddr_in);
RDPState state;
int numbytes_rec, numbytes_sent;

void handleHandShake1() {
	if ((numbytes_rec = recvfrom(sockfd, rec_buffer, 
	 sizeof(rec_buffer),0,
	 (sockaddr *) &sender_addr, &sender_len)) == -1) {
		error("error: on recvfrom()!");
	}
	rdp_segment rec_seg = generate_segment(rec_buffer);
	if (rec_seg.SYN != true) {debugprint("HS1 failed"); } 
	else { 
		//debugprint("HandShake 1 received");
		log_event('r', "SYN", rec_seg,sender_addr,rec_addr);
		//print_rdp_message(rec_seg);
		
		// 3W-HandShake Part 2
		rdp_segment resp_seg = rec_seg;
		resp_seg.ackno = rec_seg.seqno+1;
		resp_seg.seqno = random_seq();
		isn = resp_seg.seqno;
		rdp_payload_set(resp_seg,"HSP2");
		strcpy(send_buffer, rdp_string_message(resp_seg).c_str());
		if ((numbytes_sent = sendto(sockfd,send_buffer, sizeof(send_buffer), 0,
		 (sockaddr *) &sender_addr, sender_len)) == 1) {
			close(sockfd);
			error ("in sendto()");
		}
		log_event('s', "SYN+ACK", resp_seg, sender_addr, rec_addr);
		state = HandShake3;
		//debugprint("state change to HandShake3");
	}	
}

void handleHandShake3() {
	if ((numbytes_rec = recvfrom(sockfd, rec_buffer, 
	 sizeof(rec_buffer),0,
	 (sockaddr *) &sender_addr, &sender_len)) == -1) {
		error("error: on recvfrom()!");
	}
	rdp_segment rec_seg = generate_segment(rec_buffer);
	if (rec_seg.SYN == false && rec_seg.ackno == isn+1) {
		log_event('r', "ACK", rec_seg, sender_addr, rec_addr);
	//	debugprint("HandShake3 received");
	//	print_rdp_message(rec_seg);
		// Start up the File Transfer!
		rdp_segment resp_seg = rec_seg;
		strcpy(send_buffer, rdp_string_message(resp_seg).c_str());
		if ((numbytes_sent = sendto(sockfd,send_buffer, sizeof(send_buffer), 0,
	 	 (sockaddr *) &sender_addr, sender_len)) == 1) {
			close(sockfd);
			error ("in sendto()");
		}
		log_event('s', "ACK", resp_seg, sender_addr, rec_addr);
		state = Receive;
	//	debugprint("change state");
	}else {debugprint("HS3 failed"); }
}

int main(int argc, char * argv[]) {
	srand(time(NULL));	
	if (argc != 4) {	
		error("Error: Use format\nrdps receiver_ip receiver_port sender_file_name\n");
	}
	char * receiver_ip = argv[1];
	char * receiver_port = argv[2];
	char * file_name = argv[3];

	// Create communication endpoint (socket) and two addresses
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		error("error: on socket()");
	}
	memset(&rec_addr, 0, sizeof(rec_addr));
	memset(&sender_addr, 0, sizeof(sender_addr));
	rec_addr.sin_family = AF_INET;
	rec_addr.sin_family = AF_INET;
	rec_addr.sin_addr.s_addr = inet_addr(receiver_ip);
	rec_addr.sin_port = htons(atoi(receiver_port));
	
	// bind to a socket with reusibility option
	int optval = 1;
	setsockopt(sockfd,SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &optval, sizeof(optval));
	if (bind(sockfd,(sockaddr *) &rec_addr,
		sizeof(rec_addr)) < 0) {
		close(sockfd);
		error("error: on binding!");
	}
	debugprint("rdpr is now running");
	
	state = HandShake1;
	int sret = 0;
	timeval timeout;
	fd_set readfds;
	bool quit = false;
	while(!quit) {
		fflush(stdout);
		
		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		FD_SET(0,&readfds); // stdin
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		sret = select(8, &readfds, NULL, NULL, &timeout);
		if (sret == 0){
			debugprint("Timeout");
		}
		else if (sret == -1){
			error("select");	
		}
		if (FD_ISSET(0, &readfds) && scrape_stdin_for_quit()){
			quit = true;
		}
		if (FD_ISSET(sockfd, &readfds)){
			switch (state) {
			case HandShake1: {
				handleHandShake1();
			break;}
			case HandShake3: {
				handleHandShake3();
			break;}
			case Receive: {
		debugprint("f");
				if ((numbytes_rec = recvfrom(sockfd, rec_buffer,
                                 sizeof(rec_buffer), 0,
                                 (sockaddr *) &rec_addr, &rec_len)) == -1) {
                                        error("Send: on recvfrom()!");
                                }

		debugprint(rec_buffer);
                                rdp_segment rec_seg = generate_segment(rec_buffer);

		debugprint("qqf");
                                print_rdp_message(rec_seg);

		debugprint("asdf");
				break;}
			}
		}
	}	
	close(sockfd);
	return 0;
}

