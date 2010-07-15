#ifndef _CP_PGSQL_H
#define _CP_PGSQL_H

/**
 * @addtogroup cp_dbms
 */
/** @{ */
/**
 * @file
 * definitions for postgres driver
 */

#include "common.h"
#include "db.h"

__BEGIN_DECLS

CPROPS_DLL
cp_data_source *
	cp_postgres_data_source(char *host, 
						    int port, 
						    char *login, 
						    char *password, 
						    char *db_name, 
						    char *options, 
						    char *sslmode,
						    char *krbsrvname,
						    char *service);

CPROPS_DLL
cp_data_source *
	cp_dbms_postgres_get_data_source(char *host, 
			                         int port, 
									 char *login, 
									 char *password,
									 char *dbname);

CPROPS_DLL
cp_data_source *
	cp_dbms_postgres_get_data_source_prm(char *host,
			                             int port, 
										 char *login, 
										 char *password, 
										 char *dbname, 
										 cp_hashtable *prm);

typedef CPROPS_DLL struct _cp_pgsql_connection_parameters
{
	char *host;
	int port;
	char *login;
	char *password;
	char *db_name;
	char *options;
	char *sslmode;
	char *krbsrvname;
	char *service;
} cp_pgsql_connection_parameters;

__END_DECLS

/** @} */

#endif
