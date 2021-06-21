#include "functions.h"

int task_count;
char* taskfile;
char* outfile;
struct Node *head;
int outfd;

int main (int argc, char* argv[])
{
	//pobranie nazw plikow wejscia/wyjscia
	getopt(argc, argv, "");
	if (argv[optind] == NULL || argv[optind + 1] == NULL) 
	{
		printf("Mandatory arguments misssing: minicron <taskfile> <outfile>\n");
		exit(1);
	}
	taskfile = argv[optind];
	outfile = argv[optind+1];
	
	//fork do utworzenia demona i wyjscia z glownego procesu
	pid_t pid, sid;
	pid = fork();
	if (pid < 0) {
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	    exit(EXIT_FAILURE);
	}
	//zamkniecie procesu rodzica
	if (pid > 0) {
	    exit(EXIT_SUCCESS);
	} 
	sid = setsid(); 	//utworzenie SID dla procesu potomnego
	if (sid < 0) {
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
		exit(EXIT_FAILURE);
	}
	if ((chdir("/")) < 0) //zmiana katalogu roboczego
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	    exit(EXIT_FAILURE);
	}
	
	//zamkniecie deskryptorow w daemonie
	close(STDIN_FILENO);
	close(STDERR_FILENO);
	close(STDOUT_FILENO);

	umask(0); //zmiana file mode mask     
    open_log(); //otwarcie logu

	//inicjalizacja daemona
	int flush=open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644); //reset pliku outfile
	if(flush == -1)
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
		exit(EXIT_FAILURE);
	}
	if(close(flush) == -1)
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	}
	
	task_count=0; //liczba obecnie wczytanych taskow
	
	read_tasks(taskfile, &task_count); //pobranie taskow
	MergeSort(&head); //sortowanie taskow
	Chronological(); //ustawienie chronologiczne zadan
	
	//obsluga sygnalow
	if(signal(SIGUSR1, signal_handler) == SIG_ERR)
	{
		int errcode=errno;
		unsigned char* error=strerror(errcode);
		syslog (LOG_NOTICE, "%s", error);
	    exit(EXIT_FAILURE);
	}
	
	if(signal(SIGUSR2, signal_handler) == SIG_ERR)
	{
		int errcode=errno;
		unsigned char* error=strerror(errcode);
		syslog (LOG_NOTICE, "%s", error);
		exit(EXIT_FAILURE);
	}
	
	if(signal(SIGINT, signal_handler) == SIG_ERR)
	{
		int errcode=errno;
		unsigned char* error=strerror(errcode);
		syslog (LOG_NOTICE, "%s", error);
		exit(EXIT_FAILURE);
	} 
	
	struct tm* systime;
	int sleep_time;
	struct Node* tmp;
    
	while (1) 
	{
		//obliczenie czasu do kolejnego zadania, petla przechwytuje powrot z obslugi sygnalu	
		while(1) 
		{
			systime=get_time();
			sleep_time=calc_sleep(systime, head->hour, head->min);
			
			if(sleep_time>0)
			{
				sleep(sleep_time); // sleep do wykonania kolejnego programu
			}
			else
			{
				break;
			}
		}
    	
    	pid_t fork_id=fork();
    	if(fork_id == 0)
    	{
			execute_task(); //wywolanie procesu potomnego z taskiem do wykonania
		}
		else
			if(fork_id < 0)
			{
				int errcode=errno;
				syslog (LOG_NOTICE, "%s", strerror(errcode));
				exit(EXIT_FAILURE);
			}

					
		//zaladowanie kolejnego zadania
	   	if(head->next!=NULL)
	   	{
	   		tmp=head;
		   	head=head->next;
		   	free(tmp->command);
		   	free(tmp);
		   	task_count-=1;
	   	} 
	   	else
	   	{
		   	free(head->command);
		   	free(head);
		   	task_count-=1;
	   		break;
	   	}   	
	}
   	syslog (LOG_NOTICE, "Kolejka zakonczona");
   	exit(EXIT_SUCCESS);
}
