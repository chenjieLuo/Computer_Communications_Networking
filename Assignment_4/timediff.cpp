#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>

using namespace std;

double TimeDiffToNow (string t) {
	int t_length = t.length();
	double difference;
	char* time_in = new char[t_length + 1];
	time_t curr_time;
	time_t tm_in;
	struct tm* now;
	struct tm tm;
	
	// current time
	curr_time = time(NULL);
	now = gmtime(&curr_time);
	curr_time = mktime(now);
	// input time
	strcpy(time_in, t.c_str());
	strptime(time_in, "%a, %d %b %Y %H:%M:%S %Z", &tm);
	tm_in = mktime(&tm);
	difference = difftime(curr_time, tm_in);
	
	char* cur;
	char* in;
	cur = ctime(&curr_time);
	cout << "Current time is: " << cur << endl;
	in = ctime(&tm_in);
	cout << "Input time is: " << in << endl;
	
	delete[] time_in;
	return difference;
	
}


int main() {
	string input = "Sat, 16 Nov 2019 04:36:25 GMT";
	double diff;
	diff = TimeDiffToNow(input);
	printf("Difference is %2f\n", diff);
}