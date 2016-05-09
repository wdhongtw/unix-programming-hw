// Weida Hong wdhongtw@gmail.com
// ID: 0456110

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

extern void random_length_task();

int main(int argc, char** argv) {
	struct timeval tv;
	long finish_usec, start_usec;
	int num_work, i;

	num_work = 10;
	if (argc > 1)
		num_work = atoi(argv[1]);

	srand(time(0) ^ getpid());
	i = 0;
	gettimeofday(&tv, NULL);
	start_usec = tv.tv_sec*1000000 + tv.tv_usec;
	while(i<num_work) {
		random_length_task();

		gettimeofday(&tv, NULL);
		finish_usec = tv.tv_sec*1000000 + tv.tv_usec;
		start_usec += 100000;
		usleep(start_usec - finish_usec);
		i++;
	}

	return 0;
}
