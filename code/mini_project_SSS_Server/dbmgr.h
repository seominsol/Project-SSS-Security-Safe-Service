#ifndef __DBMGR_H__
#define __DBMGR_H__ 
#include <mysql/mysql.h>

void createSafe(int id);
void dbmgr_init(MYSQL*);
void save_log(int id, const char* log);
void set_pswd(int id, const char* pswd);
void get_pswd(int id, char* pswd);
int try_login(int id, const char* pswd);
int login(int id, const char* pswd);
void change_pswd(int id, const char* new_pswd);
int set_syslock(int id, int value);
#endif
