all: 
	echo "you can't do that: try make rdps or make rpdr"
rdps:
	g++ -o rdps rdps.cpp rdp_segment.h util.h
rdpr:
	g++ -o rdpr rdpr.cpp rdp_segment.h util.h
tcpclient:
	gcc -o tcpclient tcpclient.c
tcpserver:
	gcc -o tcpserver tcpserver.c
clean:
	rm rdpr rdps tcpserver tcpclient
