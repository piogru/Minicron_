#include "functions.h"

extern struct Node* head;
extern char* taskfile;
extern char* outfile;
extern int outfd;
extern int task_count;

struct tm* get_time(void)
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	return timeinfo;
}

int calc_sleep(struct tm* systime, int task_hour, int task_min)
{
	int sys = systime->tm_hour * 3600 + systime->tm_min * 60; //czas systemowy w sek.
	int task = task_hour * 3600 + task_min * 60; //czas zadania w sek.
	
	if(sys == task)
	{
		return 0;
	}
	if(sys < task)
	{
		return task - sys - systime->tm_sec; //task pozniej niz obecny czas
	}
	else
	{
		return 24 * 3600 - sys + task - systime->tm_sec; //task wczesniej niz obecny czas -> kolejna doba
	}
}

void read_tasks(char* taskfile, int* task_count)
{
	head=NULL;
	struct Node* current=NULL;
	int errcode=errno;
	bool more_lines=true;
	
	int filedes=open(taskfile, O_RDONLY);
	if(filedes == -1)
	{
    	int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
		exit(EXIT_FAILURE);
	}
	while(more_lines)
	{
		unsigned char buffer[8];
		size_t offset = 0;
		size_t bytes_read;
		int i;
		int hour, minute, mode;
		char buffer_1[1];
		int prog_count;
		
		if(head==NULL)
		{
			current=(struct Node*)malloc(sizeof *current);
			head=current;
		}
		else
		{
			current->next=(struct Node*)malloc(sizeof *current->next);
			current=current->next;
		}
	
		//odczyt godziny
		if( read(filedes, buffer, 6) == -1)
		{
	    	int errcode=errno;
			syslog (LOG_NOTICE, "%s", strerror(errcode));
		}
		hour = (buffer[0]-48) *10 + buffer[1]-48;
		minute = (buffer[3]-48) *10 + buffer[4]-48;
		
		if(hour > 24)
		{
			hour=23;
		}
		if(minute > 59)
		{
			minute=59;
		}
		
		//odczyt command
		size_t read_offset=0;

		prog_count=1;
		do
		{
			read(filedes, buffer_1, 1);
			read_offset++;
			if(buffer_1[0] == '|')
			{
				prog_count+=1;
			}
		}while(buffer_1[0]!=':');
		
		current->command=(char*)malloc(sizeof(char)*read_offset);
		
		if( lseek(filedes, read_offset *(-1), SEEK_CUR) == -1)
		{
	    	int errcode=errno;
			syslog (LOG_NOTICE, "%s", strerror(errcode));
		}
		
		int b_read = read(filedes, current->command, read_offset-1);
		if(b_read == -1)
		{
			int errcode=errno;
			syslog (LOG_NOTICE, "%s", strerror(errcode));
		}
		current->command[b_read]='\0';
		
		if( lseek(filedes, 1, SEEK_CUR) == -1) //przesuniecie za znak ':'
		{
			int errcode=errno;
			syslog (LOG_NOTICE, "%s", strerror(errcode));
		}
		
		//odczyt mode
		if(read(filedes, buffer_1, 1) == -1)
		{
			int errcode=errno;
			syslog (LOG_NOTICE, "%s", strerror(errcode));
		}
		mode=buffer_1[0]-48; //konwersja znak -> liczba
		if(lseek(filedes, 1, SEEK_CUR) ==-1) //przesuniecie za znak konca linii
		{
			int errcode=errno;
			syslog (LOG_NOTICE, "%s", strerror(errcode));
		}
		
		mode=mode%3;
		
		current->hour=hour;
		current->min=minute;
		current->mode=mode;
		current->next=NULL;
		current->prog_count=prog_count;
		
		
		int read_value = read(filedes, buffer_1, 1);
		if(read_value == 0) //sprawdzenie konca pliku
		{
			more_lines = false;
		}
		else
		{
			if(read_value == -1)
			{
				int errcode=errno;
				syslog (LOG_NOTICE, "%s", strerror(errcode));
			}
			
			if(lseek(filedes, -1, SEEK_CUR) == -1) //powrot przed stan sprawdzenia
			{
				int errcode=errno;
				syslog (LOG_NOTICE, "%s", strerror(errcode));
			}
		}
		
		*task_count+=1;
	}
	
	if(close(filedes)==-1) 	//zamkniecie pliku
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	}
}

/*
void print_task(struct Node* task) //wypisanie pojedynczego Node
{
	printf("TASK: %p %d:%d:%s:%d\n", (void*)task, task->hour, task->min, task->command, task->mode);
}

void print_tasks(void) //wypisanie calej listy od head 
{
    struct Node* current = head;

    while (current != NULL) {
		print_task(current);
        current = current->next;
    }
} */

void delete_tasks(void) //usuniecie listy taskow
{	
	struct Node* tmp;
	while(1)
	{
		if(head->next!=NULL)
	   	{
	   		tmp=head;
		   	head=head->next;
		   	free(tmp->command);
		   	free(tmp);
	   	} 
	   	else
	   	{
		   	free(head->command);
		   	free(head);
	   		break;
	   	}   	
	}
}

void open_log (void)
{
	openlog ("Minicron", LOG_PID, LOG_SYSLOG);
	syslog (LOG_INFO, "Uruchomiono program: Minicron; User: %d", getuid());
}

void signal_handler(int signum)
{
	int errcode;
	if(signum==SIGUSR1) //ponowne wczytanie zadan
	{
		syslog (LOG_NOTICE, "SIGUSR1: Ponowne kolejkowanie zadan.");
		delete_tasks();
		task_count=0;
		read_tasks(taskfile, &task_count);
		MergeSort(&head);
		Chronological();
	}
	
	if(signum==SIGUSR2) //wypisanie zadan
	{
		struct Node* current=head;

		syslog (LOG_NOTICE, "SIGUSR2: Stan kolejki:");
		while(current!=NULL)
		{
			syslog (LOG_INFO, "Zakolejkowane zadanie: %d:%d:%s:%d", current->hour, current->min, current->command, current->mode);
			current=current->next;
		}
	}

	if(signum==SIGINT) //przerwanie dzialania programu
	{
		syslog (LOG_NOTICE, "SIGINT: Zatrzymanie Minicron");
		
		delete_tasks();
		
		if(close(outfd)==-1)
		{
			int errcode=errno;
			syslog (LOG_NOTICE, "%s", strerror(errcode));
		}

		exit(signum);
	}
}

//SORTOWANIE
void MergeSort(struct Node** headRef)
{
    struct Node* head = *headRef;
    struct Node* a;
    struct Node* b;
 
    // jesli lista jest pusta lub 1 elementowa - brak potrzeby sortowania
    if ((head == NULL) || (head->next == NULL)) 
    {
        return;
    }
 
    // podzial na 2 listy
    FrontBackSplit(head, &a, &b);
 
    // sortowanie 2 czesci listy
    MergeSort(&a);
    MergeSort(&b);
 
    // zlaczenie 2 list
    *headRef = SortedMerge(a, b);
}
 
struct Node* SortedMerge(struct Node* a, struct Node* b)
{
    struct Node* result = NULL;
 
    if (a == NULL)
        return (b);
    else if (b == NULL)
        return (a);
 
 	int a_time = a->hour *60 +a->min;
	int b_time = b->hour *60 +b->min;
	
    if (a_time <= b_time)  
    {
        result = a;
        result->next = SortedMerge(a->next, b);
    }
    else {
        result = b;
        result->next = SortedMerge(a, b->next);
    }
    return (result);
}
 
// Funkcje pomocnicze do sortowania:
// podzial listy na 2 czesci
void FrontBackSplit(struct Node* source, struct Node** frontRef, struct Node** backRef)
{
    struct Node* fast;
    struct Node* slow;
    slow = source;
    fast = source->next;
 
    // fast - przeskok o 2 elementy
    // slow - przeskok o 1 element
    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }
 	// gdy fast osiaga koniec listy - podzial na 2 czesci w miejscu slow (2 polowy)
    *frontRef = source;
    *backRef = slow->next;
    slow->next = NULL;
}

//przeniesienie czesci listy ktora ma: czas < czas_systemowy, na koniec listy
void Chronological(void) 
{
    struct Node* current=head;
    struct Node* previous;
    struct Node* newhead;
    struct tm* system_time;
	bool shift=false;
 	   	
    if(head==NULL)
    {
    	return;
    }
    if(task_count==1)
    {
    	return;
    }
	
	system_time=get_time();
	int systime = system_time->tm_hour * 3600 + system_time->tm_min * 60;
	int tasktime = head->hour * 3600 + head->min * 60;
	
	while( current != NULL )
	{
		tasktime = current->hour * 3600 + current->min * 60;
		if(systime > tasktime)
		{
			previous = current;
			current = current->next;
		}
		else
		{
			if(current != head) //jesli nie ma czego przesuwac
				shift=true;	
			break;	
		}
	}
	
	if(shift==false)
	{
		return;
	}
	
	previous->next=NULL; //przeciecie listy
	newhead=current; //nowy poczatek listy
	
	while(current->next!=NULL)
	{
		current=current->next;	
	}
	current->next=head; //poczatek listy na koniec
	head=newhead; //nowy poczatek za przecieciem
}

// wydzielenie programow do uruchomienia
char** get_prog(char* input, char** prog)
{
	int arg_count=0;
	int i;
	int read_offset=0;
	int length;
	char delimiter[]=" ";
	char** args;
	
	length=strlen(input);
	for(i=0; i<length; i++) //zliczenie ilosci argumentow w komendzie
	{
		if(input[i] == ' ')
		{
			arg_count+=1;
		}
	}
	*prog=strtok(input, delimiter); //oddzielenie nazwy programu
	args= (char**)malloc(sizeof(char*) * (arg_count+2)); //+2 zamiast +1 na nazwe progr.
	
	args[0]=*prog;

	for(i=1; i<=arg_count; i++)
	{
		args[i]=strtok(NULL, delimiter); //wydzielenie poszczegolnych argumentow
	}
	args[arg_count+1]=NULL;	//terminacja tablicy nullem
	
	return args;
}

// usuniecie spacji przed i po ciagu znakow
size_t trimwhitespace(char *out, size_t len, const char *str)
{
  if(len == 0)
    return 0;

  const char *end;
  size_t out_size;

  // spacje przed
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0) 
  {
    *out = 0;
    return 1;
  }

  // spacje po
  end = str + strlen(str);
  while(end > str && isspace((unsigned char)*end)) end--;
  end++; 

  // dlugosc ciagu wyjsciowego
  out_size = (end - str) < len ? (end - str) : len;

  // kopia skroconego ciagu i znak konca ciagu
  memcpy(out, str, out_size);
  out[out_size] = 0;

  return out_size;
}

void execute_task(void)
{	
	//pobranie programow do wykonania
	outfd=open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
	
	if(outfd == -1)
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	}
	
	//wypisanie komendy do pliku
	char mode[3]={':', head->mode+48, '\0'};
	if(write(outfd, head->command, strlen(head->command)) == -1) 
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	}
	if(write(outfd, mode, 2) == -1) 
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	}
	if(write(outfd, "\n", 1) == -1)
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	}
	
	syslog (LOG_INFO, "Zadanie: %d:%d:%s:%d", head->hour, head->min, head->command, head->mode);

	char** prog=(char**)malloc(sizeof(char*)*head->prog_count);
	char*** args=(char***)malloc(sizeof(char**)*head->prog_count);
	char** command=(char**)malloc(sizeof(char**)*head->prog_count);
	
	//wydzielenie programow do potoku
	command[0]=strtok(head->command, "|");
	trimwhitespace(command[0], strlen(command[0]), command[0]);
	
	for(int i=1; i<head->prog_count; i++)
	{
		command[i]=strtok(NULL , "|");
		trimwhitespace(command[i], strlen(command[i]), command[i]);
	}
	for(int i=0; i<head->prog_count; i++)
	{	
		args[i] = get_prog(command[i], &prog[i]);
		
	}

	//Obsluga potoku	
	int prog_count;
	int devnull;
    int p[2];
    pid_t fk_id;
    int fd_in=0;
    
	dup2(outfd, 2);
    
	for (prog_count = 0; prog_count < head->prog_count-1; prog_count++)
    {
		pipe(p);
		if ((fk_id = fork()) == -1)
		{
			exit(EXIT_FAILURE);
		}
		else if (fk_id == 0)
		{
			dup2(fd_in, 0); //przekierowanie wejscia

			int devnull;
		
			switch(head->mode)
			{		
				case 0:
					dup2(p[1], STDOUT_FILENO);
					close(STDERR_FILENO);
				break;
				
				case 1:
					dup2(p[1], STDOUT_FILENO);
					devnull=open("/dev/null", O_WRONLY);
					if(devnull == -1)
					{
						int errcode=errno;
						syslog (LOG_NOTICE, "%s", strerror(errcode));
						exit(EXIT_FAILURE);
					}
					dup2(devnull, STDOUT_FILENO);
				break;
				
				case 2:
					dup2(p[1], STDOUT_FILENO);
				break;
			}
			
			close(p[0]);
			
			execvp(args[prog_count][0], args[prog_count]);
			exit(EXIT_FAILURE);
		}
		else
		{
			wait(NULL);
			close(p[1]);
			fd_in = p[0]; //wejscie pipe dla kolejnej komendy
		}
    }

    
	//ostatni program w potoku    
    int pid=fork();
    if(pid > 0)
    {
    	int status;
    	wait(&status);
    	if(WIFEXITED(status))
    	{
    		syslog (LOG_INFO, "Kod wyjscia: %d", WEXITSTATUS(status));
		}
    	else
    	{
   			syslog (LOG_INFO, "Kod wyjscia: proces zakonczony bez exit");
    	}

    	for(int i=0; i<head->prog_count; i++)
    	{
    		free(args[i]);
    	}
		free(prog);
		free(args);
		free(command);
    	exit(EXIT_SUCCESS);
    }
    if(pid == 0)
    {
    	int devnull;
    	
		switch(head->mode)
		{	
			case 0:
				dup2 (outfd, STDOUT_FILENO);
				close(STDERR_FILENO);
			break;
			
			case 1:
				dup2(p[1], STDOUT_FILENO);
				devnull=open("/dev/null", O_WRONLY);
				if(devnull == -1)
				{
					int errcode=errno;
					syslog (LOG_NOTICE, "%s", strerror(errcode));
					exit(EXIT_FAILURE);
				}
				dup2 (devnull, STDOUT_FILENO);
			break;
			
			case 2:
				dup2 (outfd, STDOUT_FILENO);
			break;
		}
		
		dup2(fd_in, STDIN_FILENO);
		close(fd_in);
		
		execvp(args[prog_count][0], args[prog_count]);
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	}
	else
	{
		int errcode=errno;
		syslog (LOG_NOTICE, "%s", strerror(errcode));
	    exit(EXIT_FAILURE);
	}
	
}

