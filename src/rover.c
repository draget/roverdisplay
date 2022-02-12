/* 
 * This file is part of the RoverDisplay distribution (https://github.com/draget/roverdisplay).
 * Copyright (c) 2022 Thomas H. Drage except where
 * derived from RoverGauge sourcecode (https://github.com/colinbourassa/rovergauge)
 * under the terms of the GNU GPL.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <aio.h>

#include "cuxinterface.h"

#define STR_INDIR(x) #x
#define STR(x) STR_INDIR(x)

#define COL1	0
#define COL1_D	25
#define COL1_U	31
#define	COL2	40
#define COL2_D	65
#define COL2_U	71
#define ROWS	15
#define COLS	80
#define FLEN	6

#define REFRESH	200

void exit_handler(int signum);
void alarm_handler(int signum);
void io_handler(int signum);
void do_layout();
void update_data();
void process_key();
void setup_aio_buffer(struct aiocb *aio_buf);


ecu_data dat;
bool metric;

volatile sig_atomic_t run;
volatile sig_atomic_t alarm_flag;
volatile sig_atomic_t io_flag;

struct aiocb kbcbuf;

int main(int argc, char** argv) {

	struct sigaction handler;
	sigset_t blocked;
	struct itimerval itimer;

	itimer.it_value.tv_sec = REFRESH / 1000;
	itimer.it_value.tv_usec = 1000*(REFRESH % 1000);
	itimer.it_interval.tv_sec = REFRESH / 1000;
	itimer.it_interval.tv_usec = 1000*(REFRESH % 1000);

	handler.sa_handler = alarm_handler;
	handler.sa_flags = SA_RESTART;
	sigemptyset(&blocked);
	handler.sa_mask = blocked;

	if(sigaction(SIGALRM, &handler, NULL) == -1) {
		perror("Couldn't install update handler");
	}

	handler.sa_handler = io_handler;

	if(sigaction(SIGIO, &handler, NULL) == -1) {
        	perror("Couldn't install AIO handler");
	}

	if(argc != 2) {
		printf("Usage: roverdisplay <port>, e.g. roverdisplay /dev/ttyS0\n");
		return 1;
	}

	bool res = connect_to_ecu(&dat, argv[1]);
	
	if(! res) {
		fprintf(stderr, "Could not open serial port.\n");
		return 1;
	}

	run = 1;
	metric = true;

	setup_aio_buffer(&kbcbuf);
	aio_read(&kbcbuf);
	
	sigaddset(&blocked, SIGALRM);
	handler.sa_flags = 0;
	handler.sa_handler = exit_handler;
	if(sigaction(SIGINT, &handler, NULL) == -1) {
                perror("Couldn't install exit handler");
        }

	initscr();
	cbreak();
	clear();
	curs_set(0);

	do_layout();

	setitimer(ITIMER_REAL, &itimer, NULL);

	while(run) {
		if(alarm_flag) {
			update_data();
			alarm_flag = 0;
		}
		else if(io_flag) {
			process_key();
			io_flag = 0;
		}
		else {
			pause();
		}
	}

	echo();
	endwin();

	printf("RoverDisplay - goodbye.\n");

	return 0;

}

void exit_handler(int signum) {

	struct sigaction cancel_sigalrm;
	cancel_sigalrm.sa_handler = SIG_IGN;
	cancel_sigalrm.sa_flags = 0;
	sigemptyset(&cancel_sigalrm.sa_mask);

	sigaction(SIGALRM, &cancel_sigalrm, NULL);

	run = 0;

	return;
}

void do_layout() {

	mvprintw(0, COL2 - 6, "RoverDisplay");

	mvprintw(1, 70, "MIL");
	mvprintw(2, COL1, "Engine speed:");
        mvprintw(2, COL1_U, "rpm");
	mvprintw(2, COL2, "Engine temperature:");
	mvprintw(2, COL2_U, metric ? "degC" : "degF");
	mvprintw(3, COL1, "Road speed:");
	mvprintw(3, COL1_U, metric ? "km/h" : "mph ");
	mvprintw(3, COL2, "Fuel temperature:");
        mvprintw(3, COL2_U, metric ? "degC" : "degF");
	mvprintw(4, COL1, "MAF:");
        mvprintw(4, COL1_U, "%");
	mvprintw(5, COL1, "Throttle:");
        mvprintw(5, COL1_U, "%");
	mvprintw(6, COL1, "Idle bypass:");
        mvprintw(6, COL1_U, "%");
	mvprintw(7, COL1, "Lambda trim (odd):");
	mvprintw(7, COL2, "Lamba trim (even):");
	mvprintw(8, COL1, "Injector duty cycle:");
	mvprintw(9, COL1, "Pulse width:");
	mvprintw(13, COL1, "Main voltage:");
        mvprintw(13, COL1_U, "V");

	mvprintw(ROWS - 1, COLS - 1, " ");

	refresh();

	return;

}

void alarm_handler(int signum) {
	alarm_flag = 1;
	return;
}

void io_handler(int signum) {
	io_flag = 1;
	return;
}

void update_data() {

	read_result result;

	if(run == 1) {
		attron(A_REVERSE);
		mvprintw(ROWS - 1, COLS - 1, " ");
		run++;
	}
	else {
		mvprintw(ROWS - 1, COLS - 1, " ");
		run = 1;
	}

	result = read_data(&dat);

	attron(A_REVERSE);

	if(result == readresult_failure) {
		mvprintw(ROWS - 1, COL2, "Read Err");
	}
	else {
		mvprintw(ROWS - 1, COL2, "Read Ok");
	}

	attroff(A_REVERSE);

	mvprintw(2, COL1_D, "%-" STR(FLEN) "u", dat.m_engineSpeedRPM);
	mvprintw(2, COL2_D, "%-" STR(FLEN) "d", convertTemperature(dat.m_coolantTempF, Celsius*metric));

	refresh();

	return;

}

void setup_aio_buffer(struct aiocb *aio_buf) {
	static char input[1];

	aio_buf->aio_fildes = 0;
	aio_buf->aio_buf = input;
	aio_buf->aio_nbytes = 1;
	aio_buf->aio_offset = 0;

	aio_buf->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	aio_buf->aio_sigevent.sigev_signo = SIGIO;

	return;
}

void process_key() {

	char *cp = (char *) kbcbuf.aio_buf;

	if(aio_error(&kbcbuf) != 0) {
		perror("AIO read failed.");
	}
	else {
		switch(*cp) {
			case 'U':
			case 'u':
				metric = !metric;
				do_layout();
				break;
		}

	}

	aio_read(&kbcbuf);

	return;

}

