#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>


static char* PRINTER = "192.168.1.15";
static int firewall_rule;

void add_firewall_rule(void) {
  srand(time(NULL));
  firewall_rule = (rand() % 32000) + 1;
  char command[100];
  sprintf(command, "sudo ipfw -q add %d deny tcp from not %s to not %s", firewall_rule, PRINTER, PRINTER);
  system(command);
  
}

void delete_firewall_rule(void) {
  char command[100];
  sprintf(command, "sudo ipfw -q delete %d", firewall_rule);
  system(command);
}


void ctrl_c_handler(int sig) {
  system("sudo -k");
  printf("\n");
  delete_firewall_rule();
  printf("Received SIGINT. Internet restored.\n");
  /* fflush(stdout); */
  exit(1);
}

void ctrl_z_handler(int sig);
void sigcont_handler(int sig);


void ctrl_z_handler(int sig) {
  delete_firewall_rule();
  signal(SIGTSTP, SIG_DFL);
  signal(SIGCONT, sigcont_handler);
  printf("\nReceived SIGTSTP. Temporarily restoring Internet.\n");
  raise(SIGTSTP);
}

void sigcont_handler(int sig) {
  system("clear");
  add_firewall_rule();
  signal(SIGTSTP, ctrl_z_handler);
  signal(SIGCONT, SIG_DFL);
  raise(SIGCONT);
  
}


char* format_time(double total_secs, char dst[8+1]) {
  int hours = (int)total_secs / 3600;
  int mins  = ((int)total_secs % 3600) / 60;
  int secs  = ((int)total_secs - ((int)hours*3600 + mins*60));
  sprintf(dst,"%2d:%2d:%2d", hours, mins, secs);
  return dst;
}


int netblock_active() {
  char command[100];
  sprintf(command, "sudo ipfw list | grep \"deny tcp from not %s to not %s\" &> /dev/null", PRINTER, PRINTER);
  return !system(command); 
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

  if(netblock_active()) {
	printf("netblock already active. Aborting.\n");
	exit(1);
  }
  
  time_t start_time = time(NULL);

  long interval_secs;

  if (argc == 1)
	interval_secs = 8*60*60;
  else {
	long interval; 
	sscanf(argv[1], "%ld", &interval);
	interval_secs = interval*60*60;
  }


  signal(SIGTSTP, ctrl_z_handler);
  signal(SIGCONT, sigcont_handler);
  signal(SIGINT, ctrl_c_handler);

  time_t end_time = start_time + interval_secs;
  time_t cur_time = start_time;
  add_firewall_rule();
  system("clear");
  system("tput sc");
  while(cur_time < end_time) {


	print_remaining_time(difftime(end_time, cur_time));
	sleep(1);
	cur_time=time(NULL);
  }

  print_remaining_time(0);
  printf("\nInternet connection restored.\n");

  return 0;
}

