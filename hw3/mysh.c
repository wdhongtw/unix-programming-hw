#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#define CMD_LEN 1024
#define ARGV_NUM 64

pid_t shell_pid, shell_pgid;
int shell_terminal;

/* A process is a single process.  */
typedef struct process
{
    struct process *next;       /* next process in pipeline */
    char *command, *argv[ARGV_NUM];                /* for exec */
    pid_t pid;                  /* process ID */
    int status;                 /* reported status value */
    bool first, last;
    char *infilename, *outfilename;
    
} process;

/* A job is a pipeline of processes.  */
typedef struct job
{
    struct job *next;           /* next active job */
    char *command;              /* command line, used for messages */
    process *first_process;     /* list of processes in this job */
    pid_t pgid;                 /* process group ID */
    int stdin, stdout, stderr;  /* standard i/o channels */
} job;

/* The active jobs are linked into a list.  This is its head.   */
job *first_j = NULL;

bool prompt(void) {
    struct passwd *pw = getpwuid(getuid());
    char *prompt_username, *prompt_sign;
    char prompt_cwd[PATH_MAX];

    if (pw->pw_uid!= 0) {
        prompt_username = pw->pw_name;
        prompt_sign = "$";
    } else {
        prompt_username = "root";
        prompt_sign = "#";
    }
    if (getcwd(prompt_cwd, PATH_MAX)==NULL) perror("getcwd");
    printf("%s$@%s %s ", prompt_username, prompt_cwd, prompt_sign);
    fflush(stdout);

    return true;
}

void init_process(process *p) {
    p->next = NULL;
    p->pid = 0;
    p->first = p->last = false;
    p->infilename = p->outfilename = NULL;
}

job *parse_cmd(char* cmd_str) {
    int i;
    process *first_p, *current_p;
    job *j;
    char *current_cmd;
    
    j = (job *)malloc(sizeof(job));
    
    current_cmd = strtok(cmd_str, "|\n");
    current_p = first_p = (process *)malloc(sizeof(process));
    init_process(current_p);
    
    current_p->command = current_cmd;
    current_p->first = true;
    i = 0;
    while ((current_cmd=strtok(NULL,"|\n"))!=NULL) {
        current_p->next = (process *)malloc(sizeof(process));
        current_p = current_p->next;
        init_process(current_p);
        
        current_p->command = current_cmd;
        i++;
    }
    current_p->last = true;
    
    current_p = first_p;
    while (current_p!=NULL) {
        
        current_p->command = strtok(current_p->command, "<");
        if ((current_p->infilename=strtok(NULL, "<"))!=NULL) {
            current_p->infilename = strtok(current_p->infilename, " ");
        }
        
        current_p->command = strtok(current_p->command, ">");
        if ((current_p->outfilename=strtok(NULL, ">"))!=NULL) {
            current_p->outfilename = strtok(current_p->outfilename, " ");
        }
        
        i = 0;
        (current_p->argv)[0] = strtok(current_p->command, " \t\n"); i++;
        while (((current_p->argv)[i]=strtok(NULL, " \t\n"))!=NULL) {
            i++;
            if (i==ARGV_NUM-1) {
                fputs("Too many tokens.\n", stderr);
                break;
            }
        }
        (current_p->argv)[i] = NULL;
        current_p = current_p->next;
    }
    
    j->first_process = first_p;
    j->pgid=0;
    return j;
}

bool run_simple(char *cmd, pid_t *child_group, bool first) {
    char *argv[ARGV_NUM];
    int iter;
    pid_t child = 0;

    iter = 0;
    argv[0] = strtok(cmd, " \t\n"); iter++;
    while ((argv[iter]=strtok(NULL, " \t\n"))!=NULL) {
        iter++;
        if (iter==ARGV_NUM-1) {
            fputs("Too many tokens.\n", stderr);
            return false;
        }
    }
    argv[iter] = NULL;

    if ((child=fork())==-1) {
        perror("fork");
        return false;
    }
    if (child==0) { // child
        if(first) setpgid(0, 0);
        execvp(argv[0], argv);
    } else {        // parent 
        if (first) {
            *child_group = child;
            
        } else {
            setpgid(child, *child_group);
        }
    }

    return true;
}

void run_process(process *p, pid_t pgid, int infile, int outfile) {
    
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    
    setpgid(0, pgid);
    
    if (infile!=STDIN_FILENO) {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if (outfile!=STDOUT_FILENO) {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    
    execvp(p->argv[0], p->argv);
    perror("execvp");
    exit(1);
    
    return;
}

void run_job(job *j) {
    process *p;
    pid_t child_pid;
    int mypipe[2], infile, outfile;
    int child_num;
    int status;
    
    p = j->first_process;
    infile = STDIN_FILENO;
    if (p->infilename!=NULL) 
        infile = open(p->infilename, O_RDONLY);
    
    j->pgid = 0;
    child_num = 0;
    while (p!=NULL) {
        if (p->next) {
            if (pipe(mypipe)<0) {
                perror("pipe");
                exit(1);
            }
            outfile = mypipe[1];
        } else {
            outfile = STDOUT_FILENO;
            if (p->outfilename!=NULL)
                outfile = open(p->outfilename, O_WRONLY|O_CREAT, 0666);
        }
        
        child_pid = fork();
        if (child_pid==0) {
            run_process(p, j->pgid, infile, outfile);
        } else if (child_pid<0) {
            perror("fork");
        } else {
            if(p->first) j->pgid = child_pid;
            child_num++;
        }
        
        if (infile!=STDIN_FILENO) close(infile);
        if (outfile!=STDOUT_FILENO) close(outfile);
        infile=mypipe[0];
        p = p->next;
    }

    if (tcsetpgrp(STDIN_FILENO, j->pgid) == -1) {
        perror("tcsetpgrp");
        exit(1);
    }
    
    while (child_num!=0) {
        wait(&status);
        child_num--;
    }
    
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) == -1) {
        perror("tcsetpgrp");
        exit(1);
    }
    
    return;
}

void clean_job(job *j) {
    process *p, *n;
    
    p = j->first_process;
    while (p->next) {
        n = p->next;
        free(p);
        p = n;
    }
    free(p);
    free(j);
}

bool run_cmd(char *cmd) {
    char *cmd_buff = (char *)malloc(CMD_LEN);
    pid_t child_group;
    int child_num;
    int status;
    FILE *tty;

    if ((tty=fopen("/dev/tty", "w"))==NULL) {
        perror("fopen");
    }
    sprintf(cmd_buff, cmd);
    child_num = 1;
    run_simple(cmd_buff, &child_group, true);
    
    if (tcsetpgrp(fileno(tty), child_group) == -1) {
        perror("tcsetpgrp");
        return false;
    }
    
    while (child_num!=0) {
        wait(&status);
        child_num--;
    }
    
    if (tcsetpgrp(fileno(tty), getpgid(0)) == -1) {
        perror("tcsetpgrp");
        return false;
    }
    

    free(cmd_buff);
    return true;
}

void ignore_signal(int sig) {
    return;
}

char *get_cmd(char* cmd_str) {
    prompt();
    if (fgets(cmd_str, CMD_LEN, stdin)==NULL) return NULL;
    return cmd_str;
}

void init_shell(void) {
	signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
	
	shell_terminal = STDIN_FILENO;
    shell_pid = getpid();

	if (setpgid(shell_pid, shell_pgid) < 0) {
		perror("setpgid");
		exit(1);
	}
    shell_pgid = shell_pid;
    
    tcsetpgrp(shell_terminal, shell_pgid);
}

int main(int argc, char *argv[]) {
    //struct passwd *pw = getpwuid(getuid());
    char *cmd_str;

    //chdir(pw->pw_dir);
	init_shell();

    cmd_str = (char *)malloc(CMD_LEN);
    while ((get_cmd(cmd_str))!=NULL) {
        first_j = parse_cmd(cmd_str);
        run_job(first_j);
        clean_job(first_j);
    }

    free(cmd_str);
    printf("\n");
    return 0;
}
