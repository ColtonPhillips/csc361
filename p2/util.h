#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>


void debugprint(std::string s) {
	std::cout << s << std::endl;
}
void debugprint(int s) {
	std::cout << s << std::endl;
}
int random_seq() {
 /* generate secret number between 1 and 1000: */
  return rand() % 1000 + 1;
}

// throw a fit!
void error(std::string e) {
	std::cout << e << std::flush;
        exit(0);
	return;
}

// is a string 0? true! else? False.
bool to_bool(std::string const& s) {
     return s != "0";
}

// we use this after sending a response
std::string get_time_string() {
        struct tm *tm;
        time_t t;
	timespec justnow;
        char str_date[100];

        t = time(NULL);
        tm = localtime(&t);

        strftime(str_date, sizeof(str_date), "%H:%M:%S.", tm);
	clock_gettime(CLOCK_MONOTONIC,&justnow);
	std::stringstream ss;
	int us = justnow.tv_nsec / 1000;
	ss << str_date << us;
	return ss.str();
}

bool scrape_stdin_for_quit() {
	char buff[100];
	fgets(buff,sizeof(buff),stdin);
        if (strchr(buff,'Q') != NULL || strchr(buff, 'q') != NULL)
		return true;
	else
		return false;

}

#endif
