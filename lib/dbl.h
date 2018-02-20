#ifndef LIBPAS_DBL_H
#define LIBPAS_DBL_H

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sqlite3.h>

#include "app.h"

#define DB_COLUMN_NAME azColName[i]
#define DB_COLUMN_VALUE argv[i]
#define DB_COLUMN_IS(V) strcmp(V, DB_COLUMN_NAME) == 0
#define DB_FOREACH_COLUMN for (int i = 0; i < argc; i++) 

extern int db_open(const char *path, sqlite3 **db);

extern int db_openR(const char *path, sqlite3 **db);

extern int db_exec(sqlite3 *db, char *q, int (*callback)(void*, int, char**, char**), void * data);

extern int db_getInt(int *item, sqlite3 *db, char *q);

extern int db_saveTableFieldInt(const char * table, const char *field, int id, int value, sqlite3 *dbl, const char* db_path);

extern int db_saveTableFieldFloat(const char * table, const char *field, int id, float value, sqlite3 *dbl, const char* db_path);

extern int db_saveTableFieldText(const char * table, const char *field, int id, const char *value, sqlite3 *dbl, const char* db_path);

#endif 

