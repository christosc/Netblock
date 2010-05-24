#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>


static char* PRINTER = "192.168.1.15";
static int firewall_rule;
static char* filelock_name = "/tmp/netblock.lock"; // /tmp/* are deleted 
												   // automatically upon reboot

bool add_firewall_rule(void) {
	srand(time(NULL));
	firewall_rule = (rand() % 32000) + 1;
	char command[100];
	sprintf(command, "sudo ipfw -q add %d deny tcp from not %s to not %s", 
			firewall_rule, PRINTER, PRINTER);
	return (system(command) == 0); // In Unix 0 == success
	
}

bool delete_firewall_rule(void) {
	char command[100];
	sprintf(command, "sudo ipfw -q delete %d", firewall_rule);
	return (system(command) == 0); // In Unix 0 == success
	
}


void sig_int_term_quit_handler(int sig) {
	system("sudo -k");
	printf("\n");
	if(delete_firewall_rule()) {
		switch(sig) {
			case SIGINT: printf("Received SIGINT. ");
				break;
			case SIGTERM: printf("Received SIGTERM. ");
				break;
			case SIGQUIT: printf("Received SIGQUIT. ");
				break;
		}
		printf("Internet restored.\n");
		remove(filelock_name);
		exit(1);
	}
	else {
		system("tput clear");
		system("tput sc");
	}
}

void ctrl_z_handler(int sig);
void sigcont_handler(int sig);


void ctrl_z_handler(int sig) {
	printf("\n");
	if(delete_firewall_rule()) {
		signal(SIGTSTP, SIG_DFL);
		signal(SIGCONT, sigcont_handler);
		printf("Received SIGTSTP. Temporarily restoring Internet.\n");
		raise(SIGTSTP);
	} else {
		system("tput clear");
		system("tput sc");
	}
}

void sigcont_handler(int sig) {
	
	if(add_firewall_rule()) {
		system("clear");
		system("tput sc");
		signal(SIGTSTP, ctrl_z_handler);
		signal(SIGCONT, SIG_DFL);
		raise(SIGCONT);
	} else {
		system("tput clear");
		system("tput sc");
	}
	
}


char* format_time(double total_secs, char dst[8+1]) {
	int hours = (int)total_secs / 3600;
	int mins  = ((int)total_secs % 3600) / 60;
	int secs  = ((int)total_secs - ((int)hours*3600 + mins*60));
	sprintf(dst,"%2d:%2d:%2d", hours, mins, secs);
	return dst;
}


bool is_active() {
	/* char command[100]; */
	/* sprintf(command, 
	"sudo ipfw list | grep \"deny tcp from not %s to not %s\" &> /dev/null", 
	 												PRINTER, PRINTER); */
	/* return !system(command);  */
	return (fopen(filelock_name, "r") ? true : false);
	
}


void print_remaining_time(double rem_time) {
	char hour[8+1];
	
	system("tput rc");
	system("tput bold");
	printf("Time remaining to restore Internet: ");
	fflush(stdout);
	system("tput sgr0");
	system("tput smso");
	printf("%s hours", format_time(rem_time, hour));
	fflush(stdout);
	system("tput rmso");
	
}


int main(int argc, char* argv[]) {
	
	
	long interval_secs;
	if (argc == 1) 
		interval_secs = 8 * 60 * 60;
	else {
		if(strcmp(argv[1], "-i") == 0  ||  strcmp(argv[1], "--info") == 0) {
			if(is_active())
				printf("netblock active.\n");
			else
				printf("netblock NOT active.\n");
			return 0;
		}
		
		if(is_active()) {
			printf("netblock already active. Aborting.\n");
			exit(1);
		}
		
		long hours;
		sscanf(argv[1], "%ld", &hours);
		interval_secs = hours * 60 * 60;
	}
	
	
	fopen(filelock_name, "w+");
	
	
	
	
	signal(SIGTSTP, ctrl_z_handler);
	signal(SIGINT, sig_int_term_quit_handler);
	signal(SIGTERM, sig_int_term_quit_handler);
	signal(SIGQUIT, sig_int_term_quit_handler);
	
	time_t start_time = time(NULL);
	time_t end_time = start_time + interval_secs;
	time_t cur_time = start_time;
	
	if(!add_firewall_rule())
		exit(1);
	
	
	system("clear");
	system("tput sc");
	while(cur_time < end_time) {
		
		
		print_remaining_time(difftime(end_time, cur_time));
		sleep(1);
		cur_time=time(NULL);
	}
	
	print_remaining_time(0);
	printf("\nInternet connection restored.\n");
	remove(filelock_name);
	return 0;
}

