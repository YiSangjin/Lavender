#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// macro
#define MENU "[q|Q] - Exit\n[f|F] - find base information\n[r|R] - start\n[p|P] - pause.\n"
#define FORMAT1 "%5d %-7d %-10s %8s %10s %-11s %-32s %10u %10lld %10u %10lld %10lld %10lld %-64s\n" // ksusenum, serial, ksuudlna, ksuseidl, ksusepid, ksusemnm, ksusepnm, ksusesql, ksusesqh, ksuseltm, ksusep1, ksusep2, ksusep3, EVENT_NAME[ksuseopc]
#define HEADING_FORMAT1 "%5s %-5s %-10s %8s %10s %-11s %-32s %10s %10s %10s %10s %10s %10s %-64s\n" // ksusenum, serial, ksuudlna, ksuseidl, ksusepid, ksusemnm, ksusepnm, ksusesql, ksusesqh, ksuseltm, ksusep1, ksusep2, ksusep3, EVENT_NAME[ksuseopc]
#define NOTFOUND "File not found.[%s]\n"

typedef int bool;
enum { false, true };

// global variables
static pthread_t p_thread[2];
static int thr_id;
static int status;

int SHMID = 0;
unsigned long long BASE_ADDRESS;
int SESSION_COUNT;
unsigned long long KSUSE_START;
unsigned long long *KSUSE_ADDRS;
int EVENT_COUNT;
char **EVENT_NAME;
const char sessionState[][10] = {"INACTIVE","ACTIVE","SPIPED","SNIPED","KILLED"};
bool isrunning;

// function definition
int getch();
char to_ascii(char ch);
void *key_press();
unsigned long long to_binary(char *hexadecimal);
void create_config();
void *lavender();
void start_thread();

// structures
typedef struct _ksuse_offset {
        int KSSPAFLG_OS;
        int KSSPAFLG_SZ;
        int KSUUDLNA_OS;
        int KSUUDLNA_SZ;
        int KSUSENUM_OS;
        int KSUSENUM_SZ;
        int KSUSESER_OS;
        int KSUSESER_SZ;
        int KSUSEFLG_OS;
        int KSUSEFLG_SZ;
        int KSUSEIDL_OS;
        int KSUSEIDL_SZ;
        int KSUSEPID_OS;
        int KSUSEPID_SZ;
        int KSUSEMNM_OS;
        int KSUSEMNM_SZ;
        int KSUSEPNM_OS;
        int KSUSEPNM_SZ;
        int KSUSESQL_OS;
        int KSUSESQL_SZ;
        int KSUSESQH_OS;
        int KSUSESQH_SZ;
        int KSUSESQI_OS;
        int KSUSESQI_SZ;
        int KSUSEPHA_OS;
        int KSUSEPHA_SZ;
        int KSUSELTM_OS;
        int KSUSELTM_SZ;
        int KSUSEGRP_OS;
        int KSUSEGRP_SZ;
        int KSUSEOPC_OS;
        int KSUSEOPC_SZ;
        int KSUSEP1_OS;
        int KSUSEP1_SZ;
        int KSUSEP2_OS;
        int KSUSEP2_SZ;
        int KSUSEP3_OS;
        int KSUSEP3_SZ;
} KSUSE_OFFSET;

KSUSE_OFFSET *ksuse;

// key board event (key + enter)
int getch()
{
        int c;
        struct termios oldattr, newattr;

        tcgetattr(STDIN_FILENO, &oldattr);
        newattr = oldattr;
        newattr.c_lflag &= ~(ICANON | ECHO);
        newattr.c_cc[VMIN] = 1;
        newattr.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
        c = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
        return c;
}

char to_ascii(char ch)
{
        if( (ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9') )
                return ch;
        else
                return ' ';
}

void *key_press()
{
        printf(MENU);
        int key;

        while(1)
        {
                key = getch();
                if(key=='q' || key == 'Q')
                {
                        pthread_exit((void *) 0);
                        printf("EXIT !!\n");
                        exit(1);
                }
                else if(key=='f' || key == 'F')
                {
                        if(SHMID == 0)
                        {
                                create_config();
                        }
                        else
                        {
                                printf("Already found SHMID!!\n");
                        }
                }
                else if(key=='r' || key == 'R')
                {
                        if(SHMID > 0)
                        {
                                printf("START !!\n");
                                isrunning = true;
                                start_thread();
                        }
                        else
                        {
                                printf("Not found SHMID!!\n");
                        }

                }
                else if(key=='p' || key == 'P')
                {
                        printf("PAUSE !!\n");
                        isrunning = false;
                }
        }
}


// hex to binary
unsigned long long to_binary(char *hexadecimal)
{
    unsigned long long decimal = 0;

    int position = 0;
    int i;
    for (i = strlen(hexadecimal) - 1; i >= 0; i--)
    {
        char ch = hexadecimal[i];

        if (ch >= 48 && ch <= 57)
        {
            decimal += (ch - 48) * pow(16, position);
        }
        else if (ch >= 65 && ch <= 70)
        {
            decimal += (ch - (65 - 10)) * pow(16, position);
        }
        else if (ch >= 97 && ch <= 102)
        {
            decimal += (ch - (97 - 10)) * pow(16, position);
        }

        position++;
    }

    return decimal;
}

// create base information
void create_config()
{
        char *script = {"./lavender.sh"};
        char *baseinfo = {"./.tmp"};
        ksuse = (KSUSE_OFFSET*) malloc(sizeof(KSUSE_OFFSET));

        remove(script);
        remove(baseinfo);
        FILE *f;
        f = fopen(script, "a+");
        if(f==NULL) { }

        fprintf(f, "sqlplus -s \"/as sysdba\" <<EOF\n");
        fprintf(f, "set echo off heading off pagesize 50000 feedback off\n");
        fprintf(f, "oradebug setmypid\n");
        fprintf(f, "spool .tmp\n");
        fprintf(f, "oradebug tracefile_name\n");
        fprintf(f, "oradebug ipc\n");
        fprintf(f, "select 'KSUSE_START='||to_number(min(addr),'xxxxxxxxxxxxxxxx') from x\\$ksuse ;\n");
        fprintf(f, "select 'SESSION_COUNT='||count(*) from x\\$ksuse ;\n");
        fprintf(f, "select 'OFFSET' from dual ;\n");
        fprintf(f, "select \'\'||c.kqfconam||\',\'||c.kqfcooff||\',\'||c.kqfcosiz  from x\\$kqfco c, x\\$kqfta t where t.indx = c.kqfcotab and t.kqftanam = 'X\\$KSUSE' and c.kqfconam in ('KSSPAFLG','KSUSEFLG','KSUSENUM','KSUSESER','KSUUDLNA','KSUSEIDL','KSUSEPID','KSUSEMNM','KSUSEPNM','KSUSESQL','KSUSESQH','KSUSESQI','KSUSELTM','KSUSEPHA','KSUSEGRP','KSUSEOPC','KSUSEP1','KSUSEP2','KSUSEP3')  order by c.indx;\n");
        fprintf(f, "select 'ADDR' from dual ;\n");
        fprintf(f, "select to_number(addr,'xxxxxxxxxxxxxxxxx') ADDR from x\\$ksuse;\n");
        fprintf(f, "select 'EVENT_COUNT='||count(*)from x\\$ksled;\n");
        fprintf(f, "select 'EVENT_NAME' from dual;\n");
        fprintf(f, "select kslednam from x\\$ksled order by indx;\n");
        fprintf(f, "EOF\n");
        pclose(f);


        FILE *fp;
        char line[1024];
        fp = popen("sh lavender.sh &> /dev/null", "r");
        if(fp == NULL)
        {
                printf("Failed to run command\n");
                exit(1);
        }

        int row = 0;
        while(fgets(line, sizeof(line)-1, fp) != NULL)
        {
                row++;
        }

        pclose(fp);

        FILE *trace;
        FILE *spool;
        char tracefile_name[1024];
        spool = fopen("./.tmp","r");
        if(spool == NULL)
        {
                printf(NOTFOUND,"./.tmp");
                exit(1);
        }

        while(fgets(line, sizeof(line)-1, spool) != NULL)
        {
                strncpy(tracefile_name, line, (strlen(line)-1));
                break;
        }

        if(strstr(tracefile_name,"ORA-"))
        {
                printf("Please check Database status. it is not started.\n");
                exit(1);
        }

        pclose(spool);

        trace = fopen(tracefile_name,"r");
        if(trace == NULL)
        {
                printf(NOTFOUND,tracefile_name);
                exit(1);
        }

        row = 0;
        int numline = 0;
        bool isfind = false;
        char area[1024];
        while(fgets(line, sizeof(line)-1, trace) != NULL)
        {
                if(strstr(line,"Variable Size"))
                {
                        isfind = true;
                        numline = row;
                }

                if(row == (numline+3) && isfind)
                {
                        strcpy(area,line);
                        break;
                }

                row++;
        }

        pclose(trace);

        strtok(area, " "); // tokenizer start
        char *tmp;
        char ipc[6][20];
        row=0;
        while((tmp = strtok(NULL, " ")))
        {
                strcpy(ipc[row],tmp);
                row++;
        }
        SHMID = atoi(ipc[1]);
        BASE_ADDRESS = to_binary(ipc[2]);

        // variable setting - start, offset, addr
        spool = fopen("./.tmp","r");
        if(spool == NULL)
        {
                printf(NOTFOUND,"./.tmp");
                exit(1);
        }

        isfind = false;
        row = 0;
        numline = 0;
        char linename[1024];
        while(fgets(line, sizeof(line)-1, spool) != NULL)
        {
                if(strstr(line,"KSSUE_START"))
                {
                        row = 0;
                        strtok(line, "="); // tokenizer start
                        char *tmp;
                        while((tmp = strtok(NULL, " ")))
                        {
                                KSUSE_START = atoll(tmp);
                                break;
                        }
                }

                if(strstr(line,"SESSION_COUNT"))
                {
                        row = 0;
                        strtok(line, "="); // tokenizer start
                        char *tmp;
                        while((tmp = strtok(NULL, " ")))
                        {
                                SESSION_COUNT = atoi(tmp);
                                KSUSE_ADDRS = (unsigned long long*)malloc(sizeof(unsigned long long*)  * SESSION_COUNT);

                                break;
                        }
                }

                if(strstr(line,"EVENT_COUNT"))
                {
                        row = 0;
                        strtok(line, "="); // tokenizer start
                        char *tmp;
                        while((tmp = strtok(NULL, " ")))
                        {
                                EVENT_COUNT = atoi(tmp);
                                EVENT_NAME = (char**)calloc(EVENT_COUNT,sizeof(char*));
                                int i = 0;
                                for(;i<EVENT_COUNT;i++)
                                {
                                        EVENT_NAME[i] = (char *)calloc(sizeof(char),64);
                                }
                                break;
                        }

                }

                if(strstr(line,"OFFSET"))
                {
                        row = 0;
                        numline = 0;
                        isfind = true;
                        strncpy(linename, line,(strlen(line)-1));
                }

                if(strstr(line,"ADDR"))
                {
                        row = 0;
                        numline = 0;
                        isfind = true;
                        strncpy(linename, line,(strlen(line)-1));
                }

                if(strstr(line,"EVENT_NAME"))
                {
                        row = 0;
                        numline = 0;
                        isfind = true;
                        strncpy(linename, line,(strlen(line)-1));
                }

                if(strlen(line) == 1)
                {
                        row++;
                }

                if(isfind && row == 1)
                {
                        if(strstr(linename,"OFFSET") && strlen(line) != 1)
                        {
                                strtok(line, ","); // tokenizer start
                                char *tmp;
                                numline=0;
                                while((tmp = strtok(NULL, ",")))
                                {
                                        if(strstr(line,"KSSPAFLG"))
                                        {
                                                if(numline==0) ksuse->KSSPAFLG_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSSPAFLG_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUUDLNA"))
                                        {
                                                if(numline==0) ksuse->KSUUDLNA_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUUDLNA_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSENUM"))
                                        {
                                                if(numline==0) ksuse->KSUSENUM_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSENUM_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSESER"))
                                        {
                                                if(numline==0) ksuse->KSUSESER_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSESER_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEFLG"))
                                        {
                                                if(numline==0) ksuse->KSUSEFLG_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEFLG_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEIDL"))
                                        {
                                                if(numline==0) ksuse->KSUSEIDL_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEIDL_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEPID"))
                                        {
                                                if(numline==0) ksuse->KSUSEPID_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEPID_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEMNM"))
                                        {
                                                if(numline==0) ksuse->KSUSEMNM_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEMNM_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEPNM"))
                                        {
                                                if(numline==0) ksuse->KSUSEPNM_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEPNM_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSESQL"))
                                        {
                                                if(numline==0) ksuse->KSUSESQL_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSESQL_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSESQH"))
                                        {
                                                if(numline==0) ksuse->KSUSESQH_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSESQH_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSESQI"))
                                        {
                                                if(numline==0) ksuse->KSUSESQI_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSESQI_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEPHA"))
                                        {
                                                if(numline==0) ksuse->KSUSEPHA_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEPHA_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSELTM"))
                                        {
                                                if(numline==0) ksuse->KSUSELTM_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSELTM_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEGRP"))
                                        {
                                                if(numline==0) ksuse->KSUSEGRP_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEGRP_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEOPC"))
                                        {
                                                if(numline==0) ksuse->KSUSEOPC_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEOPC_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEP1"))
                                        {
                                                if(numline==0) ksuse->KSUSEP1_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEP1_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEP2"))
                                        {
                                                if(numline==0) ksuse->KSUSEP2_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEP2_SZ = atoi(tmp);
                                        }
                                        if(strstr(line,"KSUSEP3"))
                                        {
                                                if(numline==0) ksuse->KSUSEP3_OS = atoi(tmp);
                                                if(numline==1) ksuse->KSUSEP3_SZ = atoi(tmp);
                                        }

                                        numline++;
                                }
                        }

                        if(strstr(linename,"ADDR") && strlen(line) != 1)
                        {
                                KSUSE_ADDRS[numline] = atoll(line);
                                numline++;
                        }

                        if(strstr(linename,"EVENT_NAME") && strlen(line) != 1)
                        {
                                strncpy(EVENT_NAME[numline], line,64);
                                numline++;
                        }
                }
        }

        pclose(spool);

        printf("SHMID: %lld, BASE ADDRESS: %lld\n", SHMID, BASE_ADDRESS);
        printf("START: %lld, SESSION: %d EVENT: %d\n", KSUSE_START, SESSION_COUNT, EVENT_COUNT);

        row = 0;
        for(;row<SESSION_COUNT;row++)
        {
                if(row == 0)
                {
                        printf("Min: %lld\n", KSUSE_ADDRS[row]);
                }
                if(row == SESSION_COUNT-1)
                {
                        printf("Max: %lld\n", KSUSE_ADDRS[row]);
                }
        }
        remove(script);
        remove(baseinfo);
}

// SGA Direct Access
void *lavender()
{
        pid_t pid;
        pthread_t tid;
        pid = getpid();
        tid = pthread_self();

        printf("pid:%u, tid:%x\n", (unsigned int)pid, (unsigned int)tid);

        if(isrunning && SHMID > 0)
        {
                void *begin_addr=(void *)shmat(SHMID, BASE_ADDRESS, SHM_RDONLY); // 010000 == SHM_RDONLY
                printf("ADDR: %d\n",begin_addr);

                if(begin_addr ==(void *)-1)
                {
                        printf("shmat: error attatching to SGA\n");
                        exit;
                }

                int idx = 0;

                while(1)
                {
                        system("clear");
                        printf(HEADING_FORMAT1,"SID","serial#", "DBUSER", "STATUS", "PID", "MACHINE", "PROGRAM", "SQL", "SQH", "WAIT", "P1", "P2", "P3", "EVENT");
                        idx=0;
                        int pos=0;
                        long ksspaflg = 0;
                        long ksuseflg = 0;
                        unsigned short ksusenum = 0;
                        int serial = 0;
                        char ksuudlna[ksuse->KSUUDLNA_SZ];
                        unsigned short ksuseidl; // ksuse idl & 11 : active , ksuseflg & 4096 : 0-inactive, 1-active, 2-sniped,3-sniped,4-killed
                        char ksusepid[ksuse->KSUSEPID_SZ];
                        char ksusemnm[ksuse->KSUSEMNM_SZ];
                        char ksusepnm[ksuse->KSUSEPNM_SZ];
                        unsigned long long ksusesql;
                        unsigned int ksusesqh;
                        char ksusesqi[ksuse->KSUSESQI_SZ+1];
                        unsigned int ksuseltm;
                        unsigned int ctime;
                        unsigned int wtime;
                        unsigned int ksusepha;
                        char ksusegrp[ksuse->KSUSEGRP_SZ];
                        int ksuseopc;
                        long long ksusep1;
                        long long ksusep2;
                        long long ksusep3;

                        unsigned long stime = time(NULL); // unixtime

                        for(;idx<SESSION_COUNT;idx++)
                        {
                                ksspaflg = *(long *)((long long)KSUSE_ADDRS[idx]);
                                ksuseflg = *(long *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEFLG_OS);
                                serial = *(int *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSESER_OS);
                                ksusenum = *(unsigned short *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSENUM_OS);

                                pos = 0;
                                for(;pos<ksuse->KSUUDLNA_SZ;pos++)
                                {
                                        ksuudlna[pos] = *(char *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUUDLNA_OS+pos);
                                }

                                ksuseidl = *(unsigned short *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEIDL_OS);

                                pos = 0;
                                for(;pos<ksuse->KSUSEPID_SZ;pos++)
                                {
                                        ksusepid[pos] = *(char *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEPID_OS+pos);
                                }
                                pos = 0;
                                for(;pos<ksuse->KSUSEMNM_SZ;pos++)
                                {
                                        ksusemnm[pos] = *(char *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEMNM_OS+pos);
                                }
                                pos = 0;
                                for(;pos<ksuse->KSUSEPNM_SZ;pos++)
                                {
                                        ksusepnm[pos] = *(char *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEPNM_OS+pos);
                                }

                                ksusesql = *(unsigned long long *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSESQL_OS);
                                ksusesqh = *(unsigned int *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSESQH_OS);

                                pos = 0;
                                for(;pos<ksuse->KSUSESQI_SZ;pos++)
                                {
                                        ksusesqi[pos] = to_ascii(*(char *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSESQI_OS+pos));
                                }

                                ksuseltm = *(unsigned int *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSELTM_OS);
                                ctime = *(unsigned int *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSELTM_OS-8);
                                wtime = *(unsigned int *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSELTM_OS-4);

                                pos = 0;
                                for(;pos<ksuse->KSUSEGRP_SZ;pos++)
                                {
                                        ksusegrp[pos] = to_ascii(*(char *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEGRP_OS+pos));
                                }

                                ksuseopc = *(int *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEOPC_OS);
                                ksusep1 = *(long long *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEP1_OS);
                                ksusep2 = *(long long *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEP2_OS);
                                ksusep3 = *(long long *)((long long)KSUSE_ADDRS[idx]+ksuse->KSUSEP3_OS);


                                if((ksspaflg & 1 != 0) && (ksuseflg & 1 != 0) && (serial >= 1))
                                {
                                        //if(((ksuseflg & 19) != 17) && (ctime < wtime) && (ksuseopc != 0))  // ksuseflg : 17-'BACKGROUND', 1-'USER', 2-'RECURSIVE', '?'
                                        if(((ksuseflg & 19) != 17) && (ksuseopc != 0) && ((ksuseidl & 11)== 1))  // ksuseflg : 17-'BACKGROUND', 1-'USER', 2-'RECURSIVE', '?'
                                        {
                                                printf(FORMAT1, ksusenum, serial, ksuudlna, sessionState[(ksuseidl & 11)], ksusepid, ksusemnm, ksusepnm, ksusesql, ksusesqh, stime-wtime, ksusep1, ksusep2, ksusep3, EVENT_NAME[ksuseopc]);
                                        }
                                }
                        }

                        sleep(1);

                        if(!isrunning)
                        {
                                break;
                        }
                }
        }
}

// key board thread
void start_thread()
{
        thr_id = pthread_create(&p_thread[1],NULL,lavender,NULL);

        if(thr_id < 0)
        {
                perror("thread create error : ");
                exit(0);
        }

        pthread_join(p_thread[0],(void **)NULL);
}


int main(int argc, char *arvg[])
{
        isrunning = false;

        sleep(1);

        thr_id = pthread_create(&p_thread[0],NULL,key_press,NULL);
        if(thr_id < 0)
        {
                perror("thread create error: ");
                exit(0);
        }

        pthread_join(p_thread[0],(void **)&status);

        printf("Exit!\n");

        return 0;
}