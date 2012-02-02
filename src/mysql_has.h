
#ifndef MYSQL_HAS_H
#define MYSQL_HAS_H

#include <mysql.h>

extern int mysql_has(MYSQL * mysql, const char * database, const char * table, const char * field);

#endif
