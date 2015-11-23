// Locally we store a segment in a nice struct

#ifndef RDP_SEGMENT_H
#define RDP_SEGMENT_H

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "util.h"

// the name implies what the program is expecting next
enum RDPState {
	Sync,
	HandShake1,
	HandShake2,
	HandShake3,
	WaitToSend,
	Send,
	Receive
};

#define MAX_RDP_PACKET_SIZE 1024
struct rdp_segment {
  std::string magic;
  bool DAT;
  bool ACK;
  bool SYN;
  bool FIN;
  bool RST;
  int seqno;
  int ackno;
  int length; // of payload
  int size;  // of window
  std::string payload;
};

void rdp_payload_set (rdp_segment &p, std::string s) {
	p.payload = s;
	p.length = p.payload.size();
}

// A handy function to init a segment
void initialize_segment (rdp_segment &p) {
        p.magic = "_magic_";
        p.DAT = false;
        p.ACK = false;
        p.SYN = false;
        p.FIN = false;
        p.RST = false;
        p.seqno = random_seq();
        p.ackno = 90;
        p.size = 500; // a small window
	rdp_payload_set(p,"Colton Is Cool"); // sets length field
}

// creates the actual string message that will be sent
std::string rdp_string_message(rdp_segment &p) {
        std::stringstream ss;
        ss << p.magic << std::endl
           << p.DAT << std::endl
           << p.ACK << std::endl
           << p.SYN << std::endl
           << p.FIN << std::endl
           << p.RST << std::endl
           << p.seqno << std::endl
           << p.ackno << std::endl
           << p.length << std::endl
           << p.size << std::endl
           << p.payload << std::endl
                    << std::endl;
        return ss.str();
}

// Take a nicely formatted string and create the segment struct
rdp_segment generate_segment(std::string s) {
        rdp_segment p;
        initialize_segment(p);
        std::istringstream is(s);
        std::vector<std::string> strings; 
        std::string _s;
        while (std::getline(is, _s, '\n')) {
                strings.push_back(_s);
        }
        p.magic = strings[0];
        p.DAT = to_bool(strings[1]);
        p.ACK = to_bool(strings[2]);
        p.SYN = to_bool(strings[3]);
        p.FIN = to_bool(strings[4]);
        p.RST = to_bool(strings[5]);
        std::istringstream(strings[6]) >> p.seqno;
        std::istringstream(strings[7]) >> p.ackno;
        std::istringstream(strings[8]) >> p.length;
        std::istringstream(strings[9]) >> p.size;
        p.payload = strings[10];
        return p;
}

void print_rdp_message(rdp_segment s) {
	std::cout << rdp_string_message(s);
}

void log_event(char event, std::string packet_type , rdp_segment s, sockaddr_in sender_addr, sockaddr_in rec_addr){
	std::stringstream ss;
	ss << get_time_string() << " "
	   << event << " "
	   << sender_addr.sin_addr.s_addr << ":"
	   << sender_addr.sin_port << " "
	   << rec_addr.sin_addr.s_addr << ":"
	   << rec_addr.sin_port << " "
	   << packet_type << " "
	   << s.seqno << "/"
	   << s.ackno << " "
	   << s.length << "/"
	   << s.size;  // window
	debugprint(ss.str());
}

int rdp_size(rdp_segment p) {
	return rdp_string_message(p).size();
}


#endif
