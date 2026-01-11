#include <stdio.h>
#include "dbmgr.h"


char sql_cmd[512] = { 0 };
MYSQL* conn = NULL;
MYSQL_ROW sqlrow;  // tuple 을 답는 구조체
int get_syslock(int id);
void dbmgr_init(MYSQL* conn_){
	conn = conn_;

}
void regist_safe(int id){
	if (!conn)	{
		printf("not inintialized dbmgr\n");
		return;
	}
	sprintf(sql_cmd, 
	"insert into Safe_Info(ID, PSWD, SysLock, Fail_Count)" 
	" values(%d,'0000', 0, 0)", id);
	int res = mysql_query(conn, sql_cmd);
    if (!res)
        printf("INFO_TB: inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
    else
        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
	
	save_log(id, "Registration successful");
	
}
int try_login(int id, const char* pswd){
	if (!conn) {
        printf("not initialized dbmgr\n");
		return -1 ;
    }
	char realPSWD[5];	
	get_pswd(id, realPSWD);
	if (!strcmp(pswd, realPSWD)){
		return 1;	
	}
	return 0;	
	
}

void change_pswd(int id,  const char* new_pswd){
	//success = try_login(id, pswd);
	//if (success){
	if (1){
		char sql_cmd[256];

        // Safe_Info 테이블의 PSWD 업데이트
        sprintf(sql_cmd,
                "UPDATE Safe_Info "
                "SET PSWD='%s', Fail_Count=0 "
                "WHERE ID=%d;",
                new_pswd, id);

        int res = mysql_query(conn, sql_cmd);
        if (res == 0) {
            printf("Password changed successfully for ID=%d\n", id);
        }
		else {
            printf("Password change failed for ID=%d: %s\n", id, mysql_error(conn));
        }	
	}
	
}

void save_log(int id, const char* log){
	if (!conn) {
		printf("not initialized dbmgr\n");
	}
	sprintf(sql_cmd, 
	"insert into Safe_Log(ID, Log, Date, Time)"
	" values(%d, '%s', now(), now())", id, log);
	
	int res = mysql_query(conn, sql_cmd);
    if (!res)
        printf("LOG_TABLE: inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
    else
        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
}

void set_pswd(int id, const char* pswd){
  	if (!conn) {
        printf("not initialized dbmgr\n");
        return;
    }

    // SQL 문자열 안전하게 구성
    snprintf(sql_cmd, sizeof(sql_cmd),
        "UPDATE Safe_Info SET PSWD='%s' WHERE ID=%d",
        pswd, id);

    // 실행
    int res = mysql_query(conn, sql_cmd);
    if (res == 0) {
        printf("Safe_Info: updated %lu rows\n",
               (unsigned long)mysql_affected_rows(conn));
    } else {
        fprintf(stderr, "ERROR: %s[%d]\n",
                mysql_error(conn), mysql_errno(conn));
    }	
}

int login(int id, const char* pswd){
	int fail_ret = 5;
	if (!conn) {
        printf("not initialized dbmgr\n");
        return -1;
    }
	if (get_syslock(id))
	{
		printf("Login blocked (account locked)");
		return -2;
	}
	int success = try_login(id, pswd);	
	if (success == 1){
		printf("Login Success");
		save_log(id, "Login success");
		set_syslock(id, 0);
		return 0;	
	}
	else if (!success){
		printf("Login Fail");
		char sql_cmd[128];

		//fail coount 증가
        sprintf(sql_cmd,
                "UPDATE Safe_Info "
                "SET Fail_Count=COALESCE(Fail_Count,0)+1 "
                "WHERE ID=%d;",
                id);
        if (mysql_query(conn, sql_cmd)) {
            fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
        }
		 // 현재 Fail_Count 읽기
        MYSQL_RES *res;
        MYSQL_ROW row;
        sprintf(sql_cmd,
                "SELECT Fail_Count FROM Safe_Info WHERE ID=%d;",
                id);
        if (mysql_query(conn, sql_cmd) == 0) {
            res = mysql_store_result(conn);
            if (res && (row = mysql_fetch_row(res))) {
                int fail_count = row[0] ? atoi(row[0]) : 0;
                mysql_free_result(res);

                if (fail_count >= 5) {
                    // SysLock = 1 설정
                    sprintf(sql_cmd,
                            "UPDATE Safe_Info SET SysLock=1 WHERE ID=%d;",
                            id);
                    if (mysql_query(conn, sql_cmd)) {
                        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
                    }
                    printf("Account locked due to 5 failed attempts (ID=%d)\n", id);
                    save_log(id, "Account locked (5 failed attempts)");
					fail_ret = -3;
					
                } 
				else {
                    save_log(id, "Login failed (wrong password)");
					fail_ret = -4;
                }
            } 
			else {
                if (res) mysql_free_result(res);
            }
        }
		return fail_ret;
	}
	return -5;
}
int set_syslock(int id, int value) {
    if (!conn) {
        printf("not initialized dbmgr\n");
        return -1; // DB 미초기화
    }

    char sql_cmd[128];

    if (value == 0) {
        // 해제할 때는 Count도 0으로 초기화
        sprintf(sql_cmd,
            "UPDATE Safe_Info SET SysLock=%d, Fail_Count=0 WHERE ID=%d;",
            value, id);
    } else {
        // 잠글 때는 SysLock만 1로
        sprintf(sql_cmd,
            "UPDATE Safe_Info SET SysLock=%d WHERE ID=%d;",
            value, id);
    }

    if (mysql_query(conn, sql_cmd)) {
        fprintf(stderr, "ERROR: %s[%d]\n",
                mysql_error(conn), mysql_errno(conn));
        return -1;
    }

    // 영향받은 행 있는지 확인
    if (mysql_affected_rows(conn) == 0) {
        return 0; // 해당 ID 없음
    }

    return 1; // 성공
}

void get_pswd(int id, char *pswd) {
    if (!conn) {
        printf("not initialized dbmgr\n");
        return;
    }

    snprintf(sql_cmd, sizeof(sql_cmd),
             "SELECT PSWD FROM Safe_Info WHERE ID=%d", id);

    int res = mysql_query(conn, sql_cmd);
    if (!res) {
        MYSQL_RES *result = mysql_store_result(conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row && row[0]) {
                strcpy(pswd, row[0]);   
                printf("GET_PSWD: id=%d, pswd=%s\n", id, pswd);
            } else {
                printf("GET_PSWD: no row for id=%d\n", id);
            }
            mysql_free_result(result);
        } else {
            fprintf(stderr, "ERROR(store): %s[%d]\n",
                    mysql_error(conn), mysql_errno(conn));
        }
    } else {
        fprintf(stderr, "ERROR(query): %s[%d]\n",
                mysql_error(conn), mysql_errno(conn));
    }
}



int get_syslock(int id){
	if (!conn){
		printf("not initialized dbmgr\n");
		return -1;
	}
	char sql_cmd[128];
    sprintf(sql_cmd, "SELECT SysLock FROM Safe_Info WHERE ID=%d;", id);

    if (mysql_query(conn, sql_cmd)) {
        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int syslock = -1;
    if (row && row[0]) {
        syslock = atoi(row[0]);  // 0 또는 1
    }

    mysql_free_result(res);
    return syslock;  // 0=unlock, 1=lock, -1=error

}
