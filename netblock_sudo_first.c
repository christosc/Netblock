#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <ctype.h>

#define BOLD     "\033[1m"
#define OFFBOLD  "\033[0m"




static int firewall_rule;
static char* filelock_name = "/tmp/netblock.lock"; // /tmp/* are deleted 
												   // automatically upon reboot
static const char* program;
static const int default_hours = 8;
static int PID;
static bool tstp_received = false;

bool add_firewall_rule(void) {
	srand(time(NULL));
	firewall_rule = (rand() % 32000) + 1;
	char command[100];
	sprintf(command, " ipfw -q add %d deny tcp from any to any via en0", 
			firewall_rule);
	return (system(command) == 0); // In Unix 0 == success
	
}

bool delete_firewall_rule(void) {
	char command[100];
	sprintf(command, " ipfw -q delete %d", firewall_rule);
	return (system(command) == 0); // In Unix 0 == success
	
}


void print_header(void);

void sigint_sigquit_handler(int sig) {
  if(system("sudo -v") == 0) {
  /* printf("\n\nAre you sure you want to terminate netblock? (yes/no) [no] "); */
  /* char ans[3+1]; */
  /* fgets(ans, 4, stdin); */
  /* while(ans != '\n' &&  ans != 'y' && ans != 'n') { */
  /* 	printf("Are you sure you want to terminate netblock (y/n)? [n] "); */
  /* 	charRead = scanf("%c", &ans); */
  /* } */
  
  /* if(ans == '\n' || ans == 'n') { */
  /* 	print_header(); */
  /* 	return; */
  /* } */
  
  /* if(strcasecmp(ans, "yes") == 0) { */
	delete_firewall_rule();
	switch(sig) {
	case SIGINT: printf("Received SIGINT. ");
	  break;
	case SIGQUIT: printf("Received SIGQUIT. ");
	  break;
	}
	printf("Internet restored.\n");
	remove(filelock_name);
	exit(1);
  }
  else {
	/* system("tput clear"); */
	/* system("tput rc"); */
	/* system("tput ed"); */
#ifndef DEBUG
	print_header();
#endif
    
  }
}


void sigterm_handler(int sig) {
  delete_firewall_rule();
  printf("\nReceived SIGTERM. Internet restored.\n");
  remove(filelock_name);
  exit(1);
  
}

void sigtstp_handler(int sig);
void sigcont_handler(int sig, siginfo_t *si, void* context);


void sigtstp_handler(int sig) {
  /* sigset_t mask, oldmask; */

  /* sigemptyset(&mask); */
  /* sigaddset(&mask, SIGTSTP); */
  /* sigaddset(&mask, SIGCONT); */
  /* sigprocmask (SIG_BLOCK, &mask, &oldmask); // Ἀπὸ τοῦδε βάζε εἰς τὴν ἀναμονὴν τὰ ὅποια SIGTSTP καὶ SIGCONT */
  tstp_received = true;
  printf("\n\n");
  if(delete_firewall_rule()) { 


	/* signal(SIGTSTP, SIG_DFL); */
	/* signal(SIGCONT, sigcont_handler); */
	printf("Received SIGTSTP. Temporarily restoring Internet.\n");
	/* raise(SIGTSTP); */
	/* while(!sigcont_interrupt) */
	/*   sigsuspend(&oldmask); */
	raise(SIGSTOP); // στεῖλον ἑαυτῇ SIGSTOP
		
  } else { // Ἢ δὲν ὑπῆρχεν ὁ κανὼν ipfw ἢ δὲν ἐδόθη σωστὸ σύνθημα εἰς τὸ sudo
		/* system("tput clear"); */
		/* system("tput rc"); */
		/* system("tput ed"); */
#ifndef DEBUG
	  print_header();
#endif
	}
  /* sigcont_action.sa_flags &= ~SA_SIGINFO; */
  /* sigcont_action.sa_handler = SIG_DFL; */
  /* sigaction(SIGCONT, &sigcont_action, NULL); */
  /* sigprocmask(SIG_UNBLOCK, &mask, NULL); */
}



void sigcont_handler(int sig, siginfo_t *si, void *sc) {

  /* sigset_t pendsigs; */
  /* sigemptyset(&pendsigs); */
  /* sigpending(&pendsigs); */

#ifdef DEBUG  
  printf("This is %s. I reveived signal %d, si_uid = %d, si_code = %d, si_errno = %d\n", __func__, sig, si->si_uid, si->si_code, si->si_errno);
  printf("pendsigs = %d\n", pendsigs);
  fflush(stdout);
#endif
  
  /* if(si->si_uid != 0/\*sigismember(&pendsigs, SIGTERM)*\/) { */
  /* 	  /\* perror("Cannot terminate this background process."); *\/ */
  /* 	  raise(SIGSTOP); */
  /* } */
  /* else { */
	if(!tstp_received) return;
  
 
	if(add_firewall_rule()) {
	  /* system("clear"); */
#ifndef DEBUG
	  print_header();
	  system("tput sc");
#endif
	  /* system("tput ed"); */
	  /* signal(SIGTSTP, sigtstp_handler); */
	  /* signal(SIGCONT, SIG_DFL); */
	
	} else {
	  /* system("tput clear"); */
	  
	  /* system("tput rc"); */
	  /* system("tput ed"); */
#ifndef DEBUG
	  print_header();
#endif
	  
	}
  
	
	tstp_received = false;
  
}


char* format_time(double total_secs, char dst[8+1]) {
	int hours = (int)total_secs / 3600;
	int mins  = ((int)total_secs % 3600) / 60;
	int secs  = ((int)total_secs - ((int)hours*3600 + mins*60));
	sprintf(dst,"%2d:%02d:%02d", hours, mins, secs);
	return dst;
}


bool is_active() {
	/* char command[100]; */
	/* sprintf(command, 
	" ipfw list | grep \"deny tcp from not %s to not %s\" &> /dev/null", 
	 												PRINTER, PRINTER); */
	/* return !system(command);  */
	return (fopen(filelock_name, "r") ? true : false);
	
}


void print_remaining_time(double rem_time) {
	char hour[8+1];
#ifndef DEBUG	
	system("tput rc");
#endif
	/* system("tput bold"); */
	printf("Time remaining to restore Internet: ");
	fflush(stdout);
	/* system("tput sgr0"); */
	system("tput smso");
	printf("%s hour(s)", format_time(rem_time, hour));
	fflush(stdout);
	system("tput rmso");
	
}

time_t start_time;
time_t end_time;


void print_header() {
#ifndef DEBUG
  system("clear");
#endif
  int progNameLen = strlen(program);

  char capProgram[progNameLen+1];
  strcpy(capProgram, program);
  capProgram[0] = toupper(capProgram[0]);
 
  char underline[progNameLen+1];
  memset(underline, '=', progNameLen);
  underline[progNameLen] = '\0';
  printf("\t\t" BOLD "%s" OFFBOLD "\n\t\t%s\n\n", capProgram, underline);
  printf("Start at:\t" BOLD "%s" OFFBOLD, ctime(&start_time));
  printf("Finish at:\t" BOLD "%s\n" OFFBOLD, ctime(&end_time));
  fflush(stdout);
  
}


bool firewall_rule_exists(void) {
  char command[100];
  sprintf(command, " ipfw list | grep \"deny tcp from any to any\" &>/dev/null");
  return (system(command) == 0);
}




int main(int argc, char* argv[]) {
  program = basename(argv[0]);
  PID = getpid();
  
  if(getuid() != 0) {
	fprintf(stderr, "%s: Operation not permitted.\n", program);
	exit(1);
  }

  
  bool netblock_active = is_active() ? true : false;
  long interval_secs;
  if (argc == 1) {
	if(netblock_active) {
	  printf("%s already active. Aborting.\n", program);
	  exit(1);
	}

	interval_secs = default_hours * 60 * 60;
  }
	
  else if(argc == 2) {
	
	  
	  if(strcmp(argv[1], "-i") == 0  ||  strcmp(argv[1], "--info") == 0 || strcmp(argv[1], "-?") == 0) {

		int firewall_rule_active = firewall_rule_exists();
		
		
		if(netblock_active) 
		  printf("%s active.\n", program);
		else
		  printf("%s NOT active.\n", program);
		
		if(firewall_rule_active)
		  printf("Firewall rule blocking Internet connection exists.\n");
		else
		  printf("No firewall rule exists blocking Internet connection.\n");
				
		return 0;
	  }
	  
	  if(netblock_active) {
		printf("%s already active. Aborting.\n", program);
		exit(1);
	  }
	  
	  long hours;
	  sscanf(argv[1], "%ld", &hours);
	  interval_secs = hours * 60 * 60;
	}
  else {
	printf("Usage: sudo %s [-i | --info | -? | hours]", program);
	
  }
	
	
  fopen(filelock_name, "w+");
	

  struct sigaction sigtstp_action;
  sigtstp_action.sa_flags &= ~SA_SIGINFO;
  sigtstp_action.sa_handler = (void*) sigtstp_handler;
  sigaction(SIGTSTP, &sigtstp_action, NULL);



  
  struct sigaction sigcont_action;
  sigcont_action.sa_flags = SA_SIGINFO;
  sigcont_action.sa_sigaction = (void*) sigcont_handler;
  sigaction(SIGCONT, &sigcont_action, NULL);


  struct sigaction sigterm_action;
  sigterm_action.sa_flags &= ~SA_SIGINFO;
  sigterm_action.sa_handler = (void*) sigterm_handler;
  sigaction(SIGTERM, &sigterm_action, NULL);


  struct sigaction sigint_sigquit_action;
  sigint_sigquit_action.sa_flags &= ~SA_SIGINFO;
  sigint_sigquit_action.sa_handler = (void*) sigint_sigquit_handler;
  sigaction(SIGINT, &sigint_sigquit_action, NULL);
  sigaction(SIGQUIT, &sigint_sigquit_action, NULL);

  
  /* signal(SIGTSTP, sigtstp_handler); */
  /* signal(SIGINT, sigint_sigquit_handler); */
  /* signal(SIGTERM, sigterm_handler); */
  /* signal(SIGQUIT, sigint_sigquit_handler); */
  
  start_time = time(NULL);
  end_time = start_time + interval_secs;
  time_t cur_time = start_time;
  
	
  if(!add_firewall_rule())
	exit(1);
  
  
  print_header();
  system("tput sc");
  while(cur_time < end_time) {
	
		
	print_remaining_time(difftime(end_time, cur_time));
	sleep(1);
	cur_time=time(NULL);
  }
  
  print_remaining_time(0);
  printf("\n");
  if(delete_firewall_rule()) {
	printf("Internet connection restored.\n");
	
  }
  else {
	printf("Internet connection was NOT restored.\n");
	
  }
  remove(filelock_name);
  return 0;
}

