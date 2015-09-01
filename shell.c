#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h> 
#include <fcntl.h>

#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

int cmd_quit(tok_t arg[]) {
  printf("Bye\n");
  exit(0);
  return 1;
}

int cmd_help(tok_t arg[]);
int cmd_cd(tok_t argl[]);
int cmd_fork(tok_t argl[]);

/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
  {cmd_cd, "cd", "change working directory"},
};

int cmd_help(tok_t arg[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

int lookup(char cmd[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;			//why cmd &&
  }
  return -1;
}

int cmd_cd(tok_t argl[]) {
	if (*argl == NULL) {
		chdir (getenv("HOME"));
	}
	else if (*argl != NULL){	
		char buf2[1024];
		
 		getcwd(buf2, sizeof(buf2));
 		strcat(buf2,"/");
 		strcat(buf2,*argl);
 		int i;
 		for(i=1;argl[i]!=NULL;i++){
 			strcat(buf2," ");
 			strcat(buf2,argl[i]);
 		}
 		if (chdir(buf2) == -1){
 			printf("No such File or Directory\n");
 		}
	}
	return 1;
}

int cmd_fork(tok_t argl[]){
	pid_t cpid, tcpid, cpgid;

	cpid = fork();	
	if (cpid == -1){
      		perror("fork error");
      		exit(EXIT_FAILURE);
        } 
        else if( cpid > 0 ) { // parent process
    		int status;
    		pid_t tcpid = wait(&status);
    		if (tcpid == -1){
    			perror("wait error");
  		}	
        }
        else if( cpid == 0 ){ // child process
        	int tokNum;
        	char file[48];
        	exFile(argl,'>',&tokNum,file);
		if(tokNum != -1){
			int fd;
			fd = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
			if(fd>-1){
				printf("Redirecting output to %s.\n",file);
				if(dup2(fd,STDOUT_FILENO)==-1){
					printf("Redirect stdout problem.\n");
				}	
			}
			else{printf("Opening redirect file problem.\n");}
			
		}
		exFile(argl,'<',&tokNum,file);
		if(tokNum != -1){
			int fd;
			fd = open(file, O_RDONLY, S_IRUSR | S_IWUSR);
			if(fd>-1){
				printf("Redirecting input from %s.\n",file);
				if(dup2(fd,STDIN_FILENO)==-1){
					printf("Redirect stdin problem.\n");
				}	
			}
			else{printf("Opening redirect file problem.\n");}
			
		}
		int position = checkTok4char(argl,'/',&tokNum);
		if(position == 0){
			execv(argl[0],argl);
		}
		else{
			tok_t *paths = getToks(getenv("PATH"));
			int i;
			char fullpathname[40];
			for (i=0 ; paths[i] != NULL ; i++){
				strcpy(fullpathname,paths[i]);
				strcat(fullpathname,"/");
				strcat(fullpathname,argl[0]);
				if((execv(fullpathname,argl))!=-1){
      					exit(EXIT_SUCCESS);	
      				}
			}
			
			perror("Error");
      			exit(EXIT_SUCCESS);
        	}
        }
}

void exFile(tok_t* argl,char sign,int* tokNum,char *file){
	int i;
	int position = checkTok4char(argl,sign,tokNum);
	if(position == 0){
		// e.g. wc shell.c >test
		if(strlen(argl[*tokNum])>1){
			strcpy(file,(argl[*tokNum]+sizeof(char)));
		}
		// e.g. wc shell.c > test
		else{strcpy(file,argl[*tokNum+1]);}
		for(i=*tokNum ; argl[i]!=NULL ; i++){
			argl[i]=NULL;
		}
	}
	else if (position > 0){
		// e.g. wc shell.c> test
		if(strlen(argl[*tokNum])-1==position){
			argl[*tokNum][position*sizeof(char)]=NULL;
			strcpy(file,argl[*tokNum+1]);
		}
		// e.g. wc shell.c>test
		else{	strcpy(file,&argl[*tokNum][(position+1)*sizeof(char)]);
			argl[*tokNum][position*sizeof(char)]=NULL;
		}
		for(i=*tokNum+1 ; argl[i]!=NULL ; i++){
			argl[i]=NULL;
		}	
	}
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){

    /* force into foreground */
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }
  /** YOUR CODE HERE */
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
  /** YOUR CODE HERE */
}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{
  /** YOUR CODE HERE */
  return NULL;
}



int shell (int argc, char *argv[]) {
  char *s = malloc(INPUT_STRING_SIZE+1);			/* user input string */
  tok_t *t;			/* tokens parsed from input */
  int lineNum = 0;
  int fundex = -1;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */
  

  init_shell();
  
  char buf[1024];
  getcwd(buf, sizeof(buf));

  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);

  
  fprintf(stdout, "%d:%s$ ", lineNum, buf);
  lineNum=1;
  while ((s = freadln(stdin))){
    t = getToks(s); /* break the line into tokens */
    fundex = lookup(t[0]); /* Is first token a shell literal */
    if(fundex >= 0) cmd_table[fundex].fun(&t[1]);
    else {
    	if(t[0]){
      		cmd_fork(t);
      	}
    }
    getcwd(buf, sizeof(buf));
    fprintf(stdout, "%d:%s$ ", lineNum, buf);
    lineNum++;
  }
  return 0;
}
