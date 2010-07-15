#ifndef _CP_MYSQL_H
#define _CP_MYSQL_H

/**
 * @addtogroup cp_dbms
 */
/** @{ */
/**
 * @file
 * definitions for mysql driver
 */

#include "common.h"
#include "db.h"

__BEGIN_DECLS

CPROPS_DLL
cp_data_source *
	cp_mysql_data_source(char *host, 
						 char *login, 
						 char *password, 
						 char *db_name, 
						 unsigned int port,
						 char *unix_socket, 
						 unsigned long client_flag);

CPROPS_DLL
cp_data_source *
	cp_dbms_mysql_get_data_source(char *host, 
			                      int port, 
								  char *login, 
								  char *password,
								  char *dbname);

CPROPS_DLL
cp_data_source *
	cp_dbms_mysql_get_data_source_prm(char *host,
			                          int port, 
									  char *login, 
									  char *password, 
									  char *dbname, 
									  cp_hashtable *prm);

typedef CPROPS_DLL struct _cp_mysql_connection_parameters
{
	char *host;
	char *login;
	char *password;
	char *db_name;
	unsigned int port;
	char *unix_socket;
	unsigned long client_flag;
} cp_mysql_connection_parameters;

__END_DECLS

/** @} */

#endif
