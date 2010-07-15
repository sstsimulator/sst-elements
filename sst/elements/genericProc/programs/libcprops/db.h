#ifndef _CP_PERSISTENCE_H
#define _CP_PERSISTENCE_H

/**
 * @addtogroup cp_dbms
 */
/** @{ */
/**
 * @file
 * definitions for dbms abstraction api
 */


#include "common.h"

#include <stdarg.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "collection.h"
#include "hashtable.h"
#include "linked_list.h"
#include "vector.h"
#include "str.h"

__BEGIN_DECLS

/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                       general framework declarations                    */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef CPROPS_DLL enum
{
	CP_FIELD_TYPE_BOOLEAN,   //  0
	CP_FIELD_TYPE_CHAR,      //  1
	CP_FIELD_TYPE_SHORT,     //  2
	CP_FIELD_TYPE_INT,       //  3
	CP_FIELD_TYPE_LONG,      //  4
	CP_FIELD_TYPE_LONG_LONG, //  5
	CP_FIELD_TYPE_FLOAT,     //  6
	CP_FIELD_TYPE_DOUBLE,    //  7
	CP_FIELD_TYPE_VARCHAR,   //  8
	CP_FIELD_TYPE_BLOB,      //  9
	CP_FIELD_TYPE_DATE,      // 10
	CP_FIELD_TYPE_TIME,      // 11
	CP_FIELD_TYPE_TIMESTAMP  // 12
} cp_field_type;

/** initialize persistence framework */
CPROPS_DLL
int cp_db_init();
/** register a dbms driver with the persistence framework */
CPROPS_DLL
int cp_db_register_dbms(char *dbms_name, void (*shutdown_call)());
/** shutdown dbms framework */
CPROPS_DLL
int cp_db_shutdown();


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                        cp_timestampz declarations                       */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef CPROPS_DLL struct _cp_timestampz
{
	struct timeval tm;
	int tz_minuteswest;
} cp_timestampz;

CPROPS_DLL
cp_timestampz *cp_timestampz_create(struct timeval *tm, int minuteswest);
CPROPS_DLL
cp_timestampz *
	cp_timestampz_create_prm(int sec_since_epoch, int microsec, int minwest);
CPROPS_DLL
void cp_timestampz_destroy(cp_timestampz *tm);
CPROPS_DLL
struct tm *cp_timestampz_localtime(cp_timestampz *tm, struct tm *res);


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                    connection parameter declarations                    */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef CPROPS_DLL struct _cp_db_connection_parameters 
{
	void *impl;
	cp_destructor_fn impl_dtr;
} cp_db_connection_parameters; /**< wrapper for dbms specific connection parameter 
                                    descriptor */

/** destroy connection parameter descriptor */
CPROPS_DLL
void cp_db_connection_parameters_destroy(cp_db_connection_parameters *prm);


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                          connection declarations                        */
/*                                                                         */
/* ----------------------------------------------------------------------- */

CPROPS_DLL struct _cp_data_source;

typedef CPROPS_DLL struct _cp_db_connection
{
	struct _cp_data_source *data_source;    /**< data source                     */
	void *connection;						/**< impl. connection type  */
	void *store; 							/**< impl. specific storage */
	short autofetch_query_metadata;
	short read_result_set_at_once;
	int fetch_size;
	int autocommit;
} cp_db_connection; /**< wrapper for dbms specific connection structure */


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                     prepared statement declarations                     */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef CPROPS_DLL struct _cp_db_statement
{
	cp_db_connection *connection;
	int prm_count;
	cp_field_type *types;
	void *source;
	void *store;
	short binary; /**< set to 1 to request results in binary format */
} cp_db_statement;

/** 
 * convenience function for driver implementations - do not call directly. to
 * create a prepared statement use cp_db_connection_prepare_statement.
 */
CPROPS_DLL
cp_db_statement *cp_db_statement_instantiate(cp_db_connection *connection,
										     int prm_count, 
		                                     cp_field_type *types, 
										     void *source);

/** set binary = 1 to request results in binary format */
CPROPS_DLL
void cp_db_statement_set_binary(cp_db_statement *stmt, int binary);

/** 
 * internal function - do not call directly. to release a prepared statement 
 * use cp_db_connection_close_statement.
 */
CPROPS_DLL
void cp_db_statement_destroy(cp_db_statement *statement);

/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                         result set declarations                         */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef CPROPS_DLL struct _cp_result_set
{
	short fetch_complete;
	cp_db_connection *connection;
	void *source;					/**< implementation result type      */
	void *store; 					/**< implementation specific storage */
	cp_vector *field_headers;
	cp_vector *field_types;
	int position; 
	int row_count;
	int field_count;
	cp_list *results;
	short dispose;
	cp_list *dispose_list;
	short binary;
} cp_result_set; /**< holder for SELECT query results */

/** get a vector with field headers */
CPROPS_DLL
cp_vector *cp_result_set_get_headers(cp_result_set *result_set);
/** get a vector with field type codes - see cp_field_type */
CPROPS_DLL
cp_vector *cp_result_set_get_field_types(cp_result_set *result_set);
/** get field count */
CPROPS_DLL
int cp_result_set_field_count(cp_result_set *result_set);
/** get number of rows */
CPROPS_DLL
int cp_result_set_row_count(cp_result_set *result_set);
/** get a vector with field headers */
CPROPS_DLL
char *cp_result_set_get_header(cp_result_set *result_set, int column);
/** get the type of a column */
CPROPS_DLL
cp_field_type cp_result_set_get_field_type(cp_result_set *rs, int column);
/** set auto disposal */
CPROPS_DLL
void cp_result_set_autodispose(cp_result_set *rs, int mode);
/** get next row */
CPROPS_DLL
cp_vector *cp_result_set_next(cp_result_set *result_set);
/** close result set */
CPROPS_DLL
void cp_result_set_destroy(cp_result_set *result_set);
/** free a row from a result set */
CPROPS_DLL
void cp_result_set_release_row(cp_result_set *result_set, cp_vector *row);
/** check whether rows are in binary format */
CPROPS_DLL
int cp_result_set_is_binary(cp_result_set *result_set);

/** perform a SELECT query */
CPROPS_DLL
cp_result_set *
	cp_db_connection_select(cp_db_connection *connection, char *query);
/** perform an INSERT, UPDATE or DELETE query */
CPROPS_DLL
int cp_db_connection_update(cp_db_connection *connection, char *query);
/** close connection */
CPROPS_DLL
int cp_db_connection_close(cp_db_connection *connection);
/** deallocate connection wrapper */
CPROPS_DLL
void cp_db_connection_destroy(cp_db_connection *connection);

/** auto fetch metadata */
CPROPS_DLL
void 
	cp_db_connection_set_fetch_metadata(cp_db_connection *connection, int mode);
/** fetch complete result set immediately */
CPROPS_DLL
void cp_db_connection_set_read_result_set_at_once(cp_db_connection *connection, 
												  int mode);
/** number of rows to fetch at a time */
CPROPS_DLL
void cp_db_connection_set_fetch_size(cp_db_connection *connection, int fetch_size);
/** escape a string for use in an SQL query on this connection */
CPROPS_DLL
char *cp_db_connection_escape_string(cp_db_connection *connection, 
		                             char *src,
								     int len);
/** escape binary data for use in an SQL query on this connection */
CPROPS_DLL
char *cp_db_connection_escape_binary(cp_db_connection *connection, 
		                             char *src, 
								     int src_len, 
								     int *res_len);
/** unescape binary data returned by an SQL query on this connection */
CPROPS_DLL
cp_string *cp_db_connection_unescape_binary(cp_db_connection *connection,
											char *src);
/** create a prepared statement for use with a connection */
CPROPS_DLL
cp_db_statement *
	cp_db_connection_prepare_statement(cp_db_connection *connection,
									   int prm_count,
									   cp_field_type *prm_types, 
									   char *query);
/** execute a prepared statement */
CPROPS_DLL
int cp_db_connection_execute_statement(cp_db_connection *connection,
		                               cp_db_statement *statement, 
									   cp_vector *prm,
									   cp_result_set **results);
/** execute a prepared statement */
CPROPS_DLL
int cp_db_connection_execute_statement_args(cp_db_connection *connection,
		                                    cp_db_statement *statement, 
									        cp_result_set **results, ...);
/** release a statement */
CPROPS_DLL
void cp_db_connection_close_statement(cp_db_connection *connection, 
									  cp_db_statement *statement);
/** set autocommit state */
CPROPS_DLL
void cp_db_connection_set_autocommit(cp_db_connection *conn, int state);
/** perform a commit */
CPROPS_DLL
int cp_db_connection_commit(cp_db_connection *conn);
/** perform a rollback */
CPROPS_DLL
int cp_db_connection_rollback(cp_db_connection *conn);


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                          db_actions declarations                        */
/*                                                                         */
/* ----------------------------------------------------------------------- */

/** 
 * implementations must define a function to open connections 
 */
typedef cp_db_connection *(*cp_db_open_fn)(struct _cp_data_source *);


/** 
 * implementations must define a function to perform SELECT queries 
 */
typedef cp_result_set *(*cp_db_select_fn)(cp_db_connection *conn, char *query);

/** 
 * implementations may define a function to retrieve metadata for a query 
 * result 
 */
typedef int (*cp_db_fetch_metadata_fn)(cp_result_set *rs);

/** 
 * implementations may define a function to retrieve result rows by chunk 
 */
typedef int (*cp_db_fetch_next_fn)(cp_result_set *rs);

/** 
 * implementations may define a function to close an open result set 
 */
typedef int (*cp_db_release_result_set_fn)(cp_result_set *rs);

/** 
 * implemetations must define a function to perform INSERT, UPDATE or DELETE 
 * queries 
 */
typedef int (*cp_db_update_fn)(cp_db_connection *conn, char *query);

/** 
 * implemetations must define a function to close database connections 
 */
typedef int (*cp_db_close_fn)(cp_db_connection *conn);

/** 
 * implementations may define a function to escape strings for use in SQL queries 
 */
typedef char *(*cp_db_escape_string_fn)(cp_db_connection *conn, char *src, int len);

/** 
 * implementations may define a function to escape binary data for use in SQL 
 * queries 
 */
typedef char *(*cp_db_escape_binary_fn)(cp_db_connection *conn, char *src, 
		                                int src_len, int *res_len);
/** 
 * implementation may define a function to unescape binary data returned by
 * an SQL query
 */
typedef cp_string *(*cp_db_unescape_binary_fn)(cp_db_connection *conn, 
											   char *src);
/** 
 * implementations may define a function to create a prepared statement 
 */
typedef cp_db_statement *
	(*cp_db_prepare_statement_fn)(cp_db_connection *conn, 
								  int prm_count,
								  cp_field_type *prm_types, 
								  char *query);

/** 
 * implementations may define a function to execute a prepared statement 
 */
typedef int (*cp_db_execute_statement_fn)(cp_db_connection *conn, 
										  cp_db_statement *stmt, 
										  cp_result_set **results, 
										  int *lengths,
										  void **prm);
/**
 * implementations may define a function to release implementation specific
 * allocations made to instantiate a prepared statement
 */
typedef void (*cp_db_release_statement_fn)(cp_db_statement *stmt);

/** 
 * implementations may define a function to set autocommit mode 
 */
typedef void (*cp_db_set_autocommit_fn)(cp_db_connection *conn, int state);

/** 
 * implementations may define a function to perform a commit 
 */
typedef int (*cp_db_commit_fn)(cp_db_connection *conn);

/** 
 * implementations may define a function to perform a rollback 
 */
typedef int (*cp_db_rollback_fn)(cp_db_connection *conn);

typedef CPROPS_DLL struct _cp_db_actions
{
	int              		      dbms;
	char            		     *dbms_lit;
	cp_db_open_fn	 		      open;
	cp_db_select_fn	  		      select;
	cp_db_fetch_next_fn		      fetch_next;
	cp_db_fetch_metadata_fn       fetch_metadata;
	cp_db_release_result_set_fn   release_result_set;
	cp_db_update_fn	 		      update;
	cp_db_close_fn	 		      close;
	cp_db_escape_string_fn        escape_string;
	cp_db_escape_binary_fn        escape_binary;
	cp_db_unescape_binary_fn      unescape_binary;
	cp_db_prepare_statement_fn    prepare_statement;
	cp_db_execute_statement_fn    execute_statement;
	cp_db_release_statement_fn    release_statement;
	cp_db_set_autocommit_fn       set_autocommit;
	cp_db_commit_fn               commit;
	cp_db_rollback_fn             rollback;
} cp_db_actions; /**< holder for implementation specific dbms access functions */

/** convenience method to create a new implementation method holder */
CPROPS_DLL
cp_db_actions *
	cp_db_actions_create(int dbms,
			             char *dbms_lit,
			          	 cp_db_open_fn open,
					  	 cp_db_select_fn select,
						 cp_db_fetch_metadata_fn fetch_metadata,
						 cp_db_fetch_next_fn fetch_next,
						 cp_db_release_result_set_fn release_result_set,
					  	 cp_db_update_fn update,
					  	 cp_db_close_fn close,
						 cp_db_escape_string_fn escape_string,
						 cp_db_escape_binary_fn escape_binary,
						 cp_db_unescape_binary_fn unescape_binary,
						 cp_db_prepare_statement_fn prepare_statement,
						 cp_db_execute_statement_fn execute_statement,
						 cp_db_release_statement_fn release_statement,
						 cp_db_set_autocommit_fn set_autocommit,
						 cp_db_commit_fn commit,
						 cp_db_rollback_fn rollback);
/** deallocate an implementation method holder */
CPROPS_DLL
void cp_db_actions_destroy(cp_db_actions *actions);


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                      driver descriptor declarations                     */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef struct _cp_data_source *
	(*cp_db_get_data_source_fn)(char *host, int port, char *user, 
								char *passwd, char *dbname);

typedef struct _cp_data_source *
	(*cp_db_get_data_source_prm_fn)(char *host, int port, char *user, 
									char *passwd, char *dbname, 
									cp_hashtable *prm);

typedef CPROPS_DLL struct _cp_dbms_driver_descriptor
{
	void *lib;
	cp_db_get_data_source_fn get_data_source;
	cp_db_get_data_source_prm_fn get_data_source_prm;
} cp_dbms_driver_descriptor;


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                         data source declarations                        */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef CPROPS_DLL struct _cp_data_source
{
	cp_db_connection_parameters *prm;
	cp_db_actions               *act;
} cp_data_source; /**< dbms driver abstraction */

#if defined(HAVE_DLFCN_H) || defined(_WINDOWS)
CPROPS_DLL
int cp_dbms_load_driver(char *name);
#endif /* HAVE_DLFCN_H */

CPROPS_DLL
cp_data_source *
	cp_dbms_get_data_source(char *driver, char *host, int port, char *user,
			char *passwd, char *dbname);

CPROPS_DLL
cp_data_source *
	cp_dbms_get_data_source_prm(char *driver, char *host, int port, 
			char *user, char *passwd, char *dbname, cp_hashtable *prm);

/** deallocate a cp_data_source structure */
CPROPS_DLL
void cp_data_source_destroy(cp_data_source *data_source);
/** open a connection with the set connection parameters */
CPROPS_DLL
cp_db_connection *
	cp_data_source_get_connection(cp_data_source *data_source); 


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                       connection pool declarations                      */
/*                                                                         */
/* ----------------------------------------------------------------------- */

typedef CPROPS_DLL struct _cp_db_connection_pool
{
	int running;
	int block;

	int min_size;
	int max_size;
	int size;
	cp_data_source *data_source;
	cp_list *free_list;

	cp_mutex *lock;
	cp_cond  *cond;
} cp_db_connection_pool; /**< connection pool structure */

/** create a new connection pool */
CPROPS_DLL
cp_db_connection_pool *
	cp_db_connection_pool_create(cp_data_source *data_source, 
								 int min_size, 
								 int max_size, 
								 int initial_size);

/** shut down a connection pool */
CPROPS_DLL
int cp_db_connection_pool_shutdown(cp_db_connection_pool *pool);
/** deallocate a connection pool object */
CPROPS_DLL
void cp_db_connection_pool_destroy(cp_db_connection_pool *pool);

/** 
 * set connection pool to block until a connection is available when 
 * retrieving connections if no connection is available
 */
CPROPS_DLL
void cp_db_connection_pool_set_blocking(cp_db_connection_pool *pool, int block);

/** retrieve a connection from a connection pool */
CPROPS_DLL
cp_db_connection *
	cp_db_connection_pool_get_connection(cp_db_connection_pool *pool);

/** return a connection to the pool */
CPROPS_DLL
void cp_db_connection_pool_release_connection(cp_db_connection_pool *pool, 
											  cp_db_connection *connection);

__END_DECLS

/** @} */

#endif
