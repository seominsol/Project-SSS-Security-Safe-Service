#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <mysql/mysql.h>
#include <stdlib.h>

#include "dbmgr.h"


#define BUF_SIZE 256
#define NAME_SIZE 20
#define ARR_CNT 5

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cv  = PTHREAD_COND_INITIALIZER;

static int full   = 0;          // 메일박스 상태
static int closed = 0;          // 종료 신호
static char slot[NAME_SIZE + BUF_SIZE + 2];   // 메일박스 버퍼



void* send_msg(void* arg);
void* recv_msg(void* arg);
void error_handling(char* msg);
void mailbox_put(const char *msg);
void mailbox_close(void);

char name[NAME_SIZE] = "[Default]";
char msg[BUF_SIZE];

int main(int argc, char* argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void* thread_return;

	if (argc != 4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "%s", argv[3]);

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	sprintf(msg, "[%s:PASSWD]", name);
	write(sock, msg, strlen(msg));
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);

	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

	if(sock != -1)
		close(sock);
	return 0;
}


void* send_msg(void* arg)
{
	int* sock = (int*)arg;
	char name_msg[NAME_SIZE + BUF_SIZE + 2];

	fputs("Send thread ready (waiting for recv input)\n", stdout);	

	while (1) {
		 // ---- 메시지 받을 때까지 대기 ----
        pthread_mutex_lock(&mtx);
        while (!full && !closed)
            pthread_cond_wait(&cv, &mtx);

        if (closed && !full) {   // 종료 신호
            pthread_mutex_unlock(&mtx);
            break;
        }

        strcpy(name_msg, slot);  // 슬롯 복사
        full = 0;

        pthread_cond_signal(&cv); // 생산자 깨우기
        pthread_mutex_unlock(&mtx);
        // -------------------------------

        // === 실제 소켓 송신 ===
        if (write(*sock, name_msg, strlen(name_msg)) <= 0) {
            *sock = -1;
            return NULL;
        }
	}
	return 0;
}

void* recv_msg(void* arg)
{
	MYSQL* conn;
	MYSQL_ROW sqlrow;
	int res;
	char sql_cmd[200] = { 0 };
	char send_msg[200] = { 0 };
	char* host = "localhost";
	char* user = "iot";
	char* pass = "pwiot";
	char* dbname = "Safe_DB";
	
	int* sock = (int*)arg;
	int i;
	char* pToken;
	char* pArray[ARR_CNT] = { 0 };

	char name_msg[NAME_SIZE + BUF_SIZE + 1];
	int str_len;

	int illu;
	float temp;
	float humi;
	conn = mysql_init(NULL);
	dbmgr_init(conn);    // MySQL 연결 핸들 포인터 전달	
	// -----------------------  connect sql-----------------------------
	puts("MYSQL startup");
	if (!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		fprintf(stderr, "ERROR : %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		exit(1);
	}
	else
		printf("Connection Successful!\n\n");

	//--------------------------------------------------------------------

	while (1) {
		memset(name_msg, 0x0, sizeof(name_msg));
		str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE);

		if (str_len <= 0)
		{
			*sock = -1;
			return NULL;
		
		}
		name_msg[str_len] = '\0';         // 널 종료 먼저!
		name_msg[strcspn(name_msg, "\n")] = '\0';  // 개행 제거
		fputs(name_msg, stdout);


//		name_msg[str_len-1] = 0;   //'\n' 제거

		pToken = strtok(name_msg, "[:@]");

		i = 0;
		while (pToken != NULL)
		{
		
			printf("%s", pToken);
			pArray[i] = pToken;
			if ( ++i >= ARR_CNT)
				break;
			pToken = strtok(NULL, "[:@]");

		}
	// ------------------ proc cmd ----------
	// [SSS_STM → SSS_SQL] SENDLOG@ID
	// [SSS_STM → SSS_SQL] LOGIN@ID@PSWD	
	// [SSS_STM → SSS_SQL] LREGIST@ID
	// [SSS_STM → SSS_SQL] CHPSWD@ID@PSWD@NEWPSWD
	// [SSS_STM → SSS_SQL] SETPSWD@PSWD
	// [SSS_STM → SSS_SQL] SYSUNLOCK@ID
	// SECURITY@1   : cnt5
	// SECURITY@2   : ball lock
	int id = atoi(pArray[2]);
	char* pswd = pArray[3];
	char* log = pArray[3];
	pswd[4] = '\0';
		if(!strcmp(pArray[1],"REGIST")){
			regist_safe(id);	
		}
		else if (!strcmp(pArray[1],"SENDLOG")){
			// Save Log at DB
			save_log(id,"Account locked (Theft detected)");
			sprintf(msg, "[SSS_ARD]SECURITY@2@%d",id);
			mailbox_put(msg);		
		}
			
		else if(!strcmp(pArray[1],"LOGIN") ){
				
			int ret = login(id, pswd);  // 성공 시 0
			if (!ret){
				sprintf(msg, "[SSS_%03d]LOGIN@SUCCESS",id);
				mailbox_put(msg);		
			}
			else{
				sprintf(msg, "[SSS_%03d]LOGIN@FAILURE",id);
				mailbox_put(msg);
				if (ret == -3) {
				    char buf[64];
  					int n = snprintf(buf, sizeof(buf), "[SSS_ARD]SECURITY@1@%d\n", id);
    				if (n > 0) {
        				ssize_t w = write(*sock, buf, (size_t)n);  // ← 반드시 *sock
        				if (w < 0) perror("write");
   					 }
				}
			}
		}
		else if(!strcmp(pArray[1],"SETPSWD") ){
			char* new_pswd = pArray[3];
			change_pswd(id, new_pswd);	
		}
		else if(!strcmp(pArray[1], "SYSUNLOCK")){
			printf("sysUnLock: %d!!\n", id);
			
			set_syslock(id, 0);
		}
		else if(!strcmp(pArray[1], "SENDLOG")) {
			
		}
		/*	
		if(!strcmp(pArray[1],"SENSOR") && (i == 5)){
			illu = atoi(pArray[2]);
			temp = atof(pArray[3]);
			humi = atof(pArray[4]);
  			sprintf(sql_cmd, "insert into sensor(name, date, time,illu, temp, humi) values('%s',now(),now(),%d,%f,%f)",pArray[0],illu, temp, humi);
			res = mysql_query(conn, sql_cmd);
			if (!res)
				printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
			else
				fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		}
		*/	
//[KSH_SQL]GETDB@LAMP
//[KSH_SQL]SETDB@LAMP@ON
//[KSH_SQL]SETDB@LAMP@ON@KSH_LIN
		if(!strcmp(pArray[1],"GETDB") && i == 3)
		{
			sprintf(sql_cmd, "select value from device where name='%s'",pArray[2]);
			
			if (mysql_query(conn, sql_cmd)) 
			{
				fprintf(stderr, "%s\n", mysql_error(conn));
				break;
			}
			MYSQL_RES *result = mysql_store_result(conn);
			if (result == NULL) 
			{
				fprintf(stderr, "%s\n", mysql_error(conn));
				break;
			}

			int num_fields = mysql_num_fields(result);
//            		printf("num_fields : %d \n",num_fields);		

			sqlrow = mysql_fetch_row(result);

			sprintf(sql_cmd,"[%s]%s@%s@%s\n",pArray[0],pArray[1],pArray[2],sqlrow[0]);
		
  			write(*sock, sql_cmd, strlen(sql_cmd));
		}
		else if(!strcmp(pArray[1],"SETDB")){
  			sprintf(sql_cmd,"update device set value='%s', date=now(), time=now() where name='%s'",pArray[3], pArray[2]);


			res = mysql_query(conn, sql_cmd);
			if (!res)
			{
				if(i==4)
					sprintf(sql_cmd,"[%s]%s@%s@%s\n",pArray[0],pArray[1],pArray[2],pArray[3]);
				else if(i==5)
					sprintf(sql_cmd,"[%s]%s@%s\n",pArray[4],pArray[2],pArray[3]);
				else
					continue;

				printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
  				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else
				fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		}
	}
	mysql_close(conn);

}

void error_handling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
// === recv 쓰레드에서 호출: 처리된 메시지를 넣어줌 ===
void mailbox_put(const char *msg) {
    pthread_mutex_lock(&mtx);
    while (full)                       // 아직 안 비워졌으면 대기
        pthread_cond_wait(&cv, &mtx);

    snprintf(slot, sizeof(slot), "%s\n", msg);
    full = 1;

    pthread_cond_signal(&cv);          // send_msg 깨우기
    pthread_mutex_unlock(&mtx);
}

// === 종료시 호출 ===
void mailbox_close(void) {
    pthread_mutex_lock(&mtx);
    closed = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mtx);
}
