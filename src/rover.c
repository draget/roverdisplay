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
#include <panel.h>
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
void write_faults();
void process_key();
void setup_aio_buffer(struct aiocb *aio_buf);

ecu_data dat;
bool metric;
PANEL *mainp;
PANEL *popupp;
WINDOW *popupw;

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

	mainp = new_panel(stdscr);
	popupw = newwin(ROWS - 1, COLS, 1, 0);
	popupp = new_panel(popupw);
	hide_panel(popupp);

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
	int row = 0;

	mvprintw(row, COL2 - 6, "RoverDisplay");
	row++;
	mvprintw(row, COL1, "MIL:");
	row++;
	mvprintw(row, COL1, "Engine speed:");
	mvprintw(row, COL1_U, "rpm");
	mvprintw(row, COL2, "Engine temperature:");
	mvprintw(row, COL2_U, metric ? "degC" : "degF");
	row++;
	mvprintw(row, COL1, "Road speed:");
	mvprintw(row, COL1_U, metric ? "km/h" : "mph ");
	mvprintw(row, COL2, "Fuel temperature:");
	mvprintw(row, COL2_U, metric ? "degC" : "degF");
	row++;
	row++;
	mvprintw(row, COL1, "MAF:");
	mvprintw(row, COL1_U, "%%");
	row++;
	mvprintw(row, COL1, "Throttle:");
	mvprintw(row, COL1_U, "%%");
	mvprintw(row, COL2, "Rev limit:");
	mvprintw(row, COL2_U, "rpm");
	row++;
	mvprintw(row, COL1, "Idle bypass:");
	mvprintw(row, COL1_U, "%%");
	mvprintw(row, COL2, "Idle target:");
	mvprintw(row, COL2_U, "rpm");
	row++;
	row++;
	mvprintw(row, COL1, "Lambda trim (odd):");
	mvprintw(row, COL1_U, "%%");
	mvprintw(row, COL2, "Lamba trim (even):");
	mvprintw(row, COL2_U, "%%");
	row++;
	mvprintw(row, COL1, "Injector duty cycle:");
	mvprintw(row, COL1_U, "%%");
	mvprintw(row, COL2, "Pulse width:");
	mvprintw(row, COL2_U, "ms");
	row++;
	row++;
	mvprintw(row, COL1, "Main voltage:");
	mvprintw(row, COL1_U, "V");
	mvprintw(row, COL2, "Fuel pump relay:");

	attron(A_REVERSE);
	mvprintw(ROWS - 1, COL1, "U");
	mvprintw(ROWS - 1, COL1 + 7, "C");
	mvprintw(ROWS - 1, COL1 + 14, "F");
	mvprintw(ROWS - 1, COL1 + 20, "L");
	mvprintw(ROWS - 1, COL1 + 25, "I");
	attroff(A_REVERSE);
	mvprintw(ROWS - 1, COL1 + 1, "nits");
	mvprintw(ROWS - 1, COL1 + 8, "odes");
	mvprintw(ROWS - 1, COL1 + 15, "uel");
	mvprintw(ROWS - 1, COL1 + 21, "og");
	mvprintw(ROWS - 1, COL1 + 26, "nfo");

	mvprintw(ROWS - 1, COLS - 1, " ");

	update_panels();	
	doupdate();

	return;
}

void codes_window() {
	read_result result;

	wclear(popupw);
	box(popupw, 0, 0);
	wattron(popupw, A_REVERSE);
	mvwprintw(popupw, ROWS - 2, 0, "Esc");
	wattroff(popupw, A_REVERSE);

	mvwprintw(popupw, 1, 1, "* ");

	result = read_fault_codes(&dat);
	if(result == readresult_failure) {
		wprintw(popupw, "Read Err");
	}
	else {
		write_faults();
	}

	show_panel(popupp);
	update_panels();	
	doupdate();
	
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
	int row;
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
		mvprintw(ROWS - 1, COL2, "Read Ok ");
	}

	attroff(A_REVERSE);

	row = 1;
	if(dat.m_milOn) attron(A_REVERSE);
	mvprintw(row, COL1_D, "%-" STR(FLEN) "s", dat.m_milOn ? "On" : "Off" );
	attroff(A_REVERSE);
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) "u", dat.m_engineSpeedRPM);
	mvprintw(row, COL2_D, "%-" STR(FLEN) "d", convertTemperature(dat.m_coolantTempF, Celsius*metric));
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) "u", convertSpeed(dat.m_roadSpeedMPH, KPH*metric));
	mvprintw(row, COL2_D, "%-" STR(FLEN) "d", convertTemperature(dat.m_fuelTempF, Celsius*metric));
	row++;
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) ".1f", dat.m_mafReading);
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) ".1f", dat.m_throttlePos);
	mvprintw(row, COL2_D, "%-" STR(FLEN) "u", dat.m_rpmLimit);
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) ".1f", dat.m_idleBypassPos);
	if(dat.m_idleMode) attron(A_REVERSE);
	mvprintw(row, COL2_D, "%-" STR(FLEN) "u", dat.m_targetIdleSpeed);
	attroff(A_REVERSE);
	row++;
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) "d", dat.m_lambdaTrimOdd);
	mvprintw(row, COL2_D, "%-" STR(FLEN) "d", dat.m_lambdaTrimEven);
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) ".1f", (dat.m_injectorPulseWidthMs / (60.0 / (float)dat.m_engineSpeedRPM * 1000.0)) * 100);
	mvprintw(row, COL2_D, "%-" STR(FLEN) ".2f", dat.m_injectorPulseWidthMs);
	row++;
	row++;
	mvprintw(row, COL1_D, "%-" STR(FLEN) ".2f", dat.m_mainVoltage);	
	if(dat.m_fuelPumpRelayOn) attron(A_REVERSE);
	mvprintw(row, COL2_D, "%-" STR(FLEN) "s", dat.m_fuelPumpRelayOn ? "On" : "Off" );
	attroff(A_REVERSE);

	update_panels();
	doupdate();

	return;
}

void write_faults() {
	int i = 1;

	if(dat.m_faultCodes.ROM_Checksum_Failure) mvwprintw(popupw, i++, 1, "* (29) ECU checksum error");
	if(dat.m_faultCodes.Lambda_Sensor_Odd) mvwprintw(popupw, i++, 1, "* (44) Lambda sensor (odd)");
	if(dat.m_faultCodes.Lambda_Sensor_Even) mvwprintw(popupw, i++, 1, "* (45) Lambda sensor (even)");
	if(dat.m_faultCodes.Misfire_Odd_Bank) mvwprintw(popupw, i++, 1, "* (40) Misfire (odd)");
	if(dat.m_faultCodes.Misfire_Even_Bank) mvwprintw(popupw, i++, 1, "* (50) Misfire (even)");
	if(dat.m_faultCodes.Airflow_Meter) mvwprintw(popupw, i++, 1, "* (12) Airflow meter");
	if(dat.m_faultCodes.Tune_Resistor_Out_of_Range) mvwprintw(popupw, i++, 1, "* (21) Tune resistor out of range");
	if(dat.m_faultCodes.Injector_Odd_Bank) mvwprintw(popupw, i++, 1, "* (34) Injector bank (odd)");
	if(dat.m_faultCodes.Injector_Even_Bank) mvwprintw(popupw, i++, 1, "* (36) Injector bank (even)");
	if(dat.m_faultCodes.Coolant_Temp_Sensor) mvwprintw(popupw, i++, 1, "* (14) Coolant temp sensor");
	if(dat.m_faultCodes.Throttle_Pot) mvwprintw(popupw, i++, 1, "* (17) Throttle pot");
	if(dat.m_faultCodes.Throttle_Pot_Hi_MAF_Lo) mvwprintw(popupw, i++, 1, "* (18) Throttle pot hi / MAF lo");
	if(dat.m_faultCodes.Throttle_Pot_Lo_MAF_Hi) mvwprintw(popupw, i++, 1, "* (19) Throttle pot lo / MAF hi");
	if(dat.m_faultCodes.Purge_Valve_Leak) mvwprintw(popupw, i++, 1, "* (88) Purge valve leak");
	if(dat.m_faultCodes.Mixture_Too_Lean) mvwprintw(popupw, i++, 1, "* (26) Mixture too lean");
	if(dat.m_faultCodes.Intake_Air_Leak) mvwprintw(popupw, i++, 1, "* (28) Intake air leak");
	if(dat.m_faultCodes.Low_Fuel_Pressure) mvwprintw(popupw, i++, 1, "* (23) Low fuel pressure");
	if(dat.m_faultCodes.Idle_Valve_Stepper_Motor) mvwprintw(popupw, i++, 1, "* (48) Idle Air Control stepper motor");
	if(dat.m_faultCodes.Road_Speed_Sensor) mvwprintw(popupw, i++, 1, "* (68) Road speed sensor");
	if(dat.m_faultCodes.Neutral_Switch) mvwprintw(popupw, i++, 1, "* (69) Neutral (gear selector) switch");
	if(dat.m_faultCodes.Low_Fuel_Pressure_or_Air_Leak) mvwprintw(popupw, i++, 1, "* (58) Ambiguous: low fuel pressure or air leak");
	if(dat.m_faultCodes.Fuel_Temp_Sensor) mvwprintw(popupw, i++, 1, "* (15) Fuel temp sensor");
	if(dat.m_faultCodes.Battery_Disconnected) mvwprintw(popupw, i++, 1, "* (02) RAM contents unreliable (battery disconnected)");
	if(dat.m_faultCodes.RAM_Checksum_Failure) mvwprintw(popupw, i++, 1, "* (03) Bad checksum on battery-backed RAM");

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
			case 'C':
			case 'c':
				codes_window();
				break;
			case 27:
				hide_panel(popupp);
				update_panels();
				doupdate();
				break;
		}
	}

	aio_read(&kbcbuf);

	return;
}

