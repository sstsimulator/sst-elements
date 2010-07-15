#ifdef _WINDOWS
#include <config-win.h>
#endif /* _WINDOWS */

#include <mysql.h>

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "db.h"
#include "vector.h"
#include "str.h"
#include "hashtable.h"
#include "util.h"
#include "log.h"

#include "db_mysql.h"

// (WINVER < 0x0500)
#if (_MSC_VER < 1300 && WINVER < 0x0500)
/* pre VC7 */
//extern "C" long _ftol( double ); //defined by VC6 C libs
long _ftol2( double dblSource ) { return _ftol( dblSource ); }
int _aulldvrm(void *p) { return _aulldiv(p); }
void *_aligned_malloc(size_t size) { return malloc(size); }
void _aligned_free(void *p) { free(p); }
#endif


static cp_db_connection *cp_mysql_open_conn(cp_data_source *data_source);
static cp_result_set *cp_mysql_select(cp_db_connection *_conn, char *query);
static int cp_mysql_fetch_metadata(cp_result_set *result_set);
static int cp_mysql_fetch_next(cp_result_set *result_set);
static int cp_mysql_release_result_set(cp_result_set *result_set);
static int cp_mysql_update(cp_db_connection *_conn, char *query);
static int cp_mysql_close_conn(cp_db_connection *connection);
static char *cp_mysql_escape_string(cp_db_connection *conn, char *str, int len);
static char *cp_mysql_escape_binary(cp_db_connection *conn, char *str, 
									int len, int *res_len);
static cp_db_statement *
	cp_mysql_prepare_statement(cp_db_connection *conn, int prm_count, 
			                   cp_field_type *prm_type, char *query);
static int cp_mysql_execute_statement(cp_db_connection *conn, cp_db_statement *stmt, 
									  cp_result_set **result_set, int *lengths,
									  void **prm);
static void cp_mysql_release_statement(cp_db_statement *statement);
static void cp_mysql_set_autocommit(cp_db_connection *conn, int state);
static int cp_mysql_commit(cp_db_connection *conn);
static int cp_mysql_rollback(cp_db_connection *conn);

volatile static int initialized = 0;
static cp_db_actions *cp_mysql_db_actions = NULL;
static cp_hashtable *mysql_field_type_map = NULL;

#define DBNAME "mysql"

typedef struct _cp_mysql_bind_desc
{
	int len;
	MYSQL_BIND *bind;
	unsigned long *length;
	my_bool *is_null;
	char **buf;
} cp_mysql_bind_desc;

typedef struct _field_type_map
{
	int mysql_field_code;
	int cprops_field_code;
} field_type_map;

field_type_map mysql2cprops[] = 
{
	{ MYSQL_TYPE_DECIMAL,      CP_FIELD_TYPE_LONG }, 
	{ MYSQL_TYPE_TINY,         CP_FIELD_TYPE_SHORT }, 
	{ MYSQL_TYPE_SHORT,        CP_FIELD_TYPE_SHORT}, 
	{ MYSQL_TYPE_LONG,         CP_FIELD_TYPE_LONG }, 
	{ MYSQL_TYPE_FLOAT,        CP_FIELD_TYPE_FLOAT }, 
	{ MYSQL_TYPE_DOUBLE,       CP_FIELD_TYPE_DOUBLE }, 
//	{ MYSQL_TYPE_NULL,         CP_FIELD_TYPE_ }, 
	{ MYSQL_TYPE_TIMESTAMP,    CP_FIELD_TYPE_TIMESTAMP }, 
	{ MYSQL_TYPE_LONGLONG,     CP_FIELD_TYPE_LONG_LONG }, 
	{ MYSQL_TYPE_INT24,        CP_FIELD_TYPE_LONG }, 
	{ MYSQL_TYPE_DATE,         CP_FIELD_TYPE_DATE }, 
	{ MYSQL_TYPE_TIME,         CP_FIELD_TYPE_TIME }, 
	{ MYSQL_TYPE_DATETIME,     CP_FIELD_TYPE_DATE }, 
	{ MYSQL_TYPE_YEAR,         CP_FIELD_TYPE_SHORT }, 
	{ MYSQL_TYPE_NEWDATE,      CP_FIELD_TYPE_DATE }, 
	{ MYSQL_TYPE_VARCHAR,      CP_FIELD_TYPE_VARCHAR }, 
	{ MYSQL_TYPE_BIT,          CP_FIELD_TYPE_SHORT }, 
	{ MYSQL_TYPE_NEWDECIMAL,   CP_FIELD_TYPE_LONG }, 
	{ MYSQL_TYPE_ENUM,         CP_FIELD_TYPE_INT }, 
	{ MYSQL_TYPE_SET,          CP_FIELD_TYPE_INT }, 
	{ MYSQL_TYPE_TINY_BLOB,    CP_FIELD_TYPE_BLOB }, 
	{ MYSQL_TYPE_MEDIUM_BLOB,  CP_FIELD_TYPE_BLOB }, 
	{ MYSQL_TYPE_LONG_BLOB,    CP_FIELD_TYPE_BLOB }, 
	{ MYSQL_TYPE_BLOB,         CP_FIELD_TYPE_BLOB }, 
	{ MYSQL_TYPE_VAR_STRING,   CP_FIELD_TYPE_VARCHAR }, 
	{ MYSQL_TYPE_STRING,       CP_FIELD_TYPE_VARCHAR } 
//	{ MYSQL_TYPE_GEOMETRY      CP_FIELD_TYPE_ }
};

cp_mysql_bind_desc *create_mysql_bind_desc(int len, MYSQL_BIND *bind, 
										   cp_vector *types, long *lengths)
{
	int i;
	cp_mysql_bind_desc *desc = calloc(1, sizeof(cp_mysql_bind_desc));
	desc->len = len;
	desc->bind = bind;
	desc->length = lengths;
	desc->is_null = calloc(len, sizeof(my_bool));
	desc->buf = calloc(len, sizeof(char *));

	for (i = 0; i < len; i++)
	{
		cp_field_type *type = cp_vector_element_at(types, i);
		switch(*type)
		{
			case CP_FIELD_TYPE_BOOLEAN:
			case CP_FIELD_TYPE_SHORT:
			case CP_FIELD_TYPE_INT:
			case CP_FIELD_TYPE_LONG:
				desc->buf[i] = calloc(1, sizeof(long));
				break;
				
			case CP_FIELD_TYPE_LONG_LONG:
#ifdef HAS_LONG_LONG_INT
				desc->buf[i] = calloc(1, sizeof(long long int));
#else
#ifdef _WINDOWS
				desc->buf[i] = calloc(1, sizeof(ULARGE_INTEGER));
#else
				desc->buf[i] = calloc(1, sizeof(long));
#endif /* _WINDOWS */
#endif /* HAS_LONG_LONG_INT */
				break;
				
			case CP_FIELD_TYPE_FLOAT:
				desc->buf[i] = calloc(1, sizeof(float));
				break;
				
			case CP_FIELD_TYPE_DOUBLE:
				desc->buf[i] = calloc(1, sizeof(double));
				break;
				
			case CP_FIELD_TYPE_CHAR:
			case CP_FIELD_TYPE_VARCHAR:
				if (lengths[i] == 0) lengths[i] = 0x400;
				desc->buf[i] = calloc(1, lengths[i]);
				bind[i].buffer_length = lengths[i];
				break;
				
			case CP_FIELD_TYPE_BLOB:
				if (lengths[i] == 0) lengths[i] = 0x2000;
				desc->buf[i] = calloc(1, lengths[i]);
				bind[i].buffer_length = lengths[i];
				break;
				
			case CP_FIELD_TYPE_DATE:
			case CP_FIELD_TYPE_TIME:
			case CP_FIELD_TYPE_TIMESTAMP:
				desc->buf[i] = calloc(1, sizeof(MYSQL_TIME));
				break;

			default:
				DEBUGMSG("mystery field type: %d", *type);
				break;
		}

		bind[i].buffer = desc->buf[i];
		bind[i].length = &desc->length[i];
	}

	return desc;
}

void cp_mysql_bind_desc_destroy(cp_mysql_bind_desc *desc)
{
	if (desc)
	{
		if (desc->bind) free(desc->bind);
		if (desc->length) free(desc->length);
		if (desc->is_null) free(desc->is_null);
		if (desc->buf) 
		{
			int i;
			for (i = 0; i < desc->len; i++)
				if (desc->buf[i]) free(desc->buf[i]);
			free(desc->buf);
		}
		free(desc);
	}
}

/* ------------------------------------------------------------------------  
                                                                             
                            date conversion functions                        
                                                                             
   ------------------------------------------------------------------------ */

static int is_date_type(cp_field_type type)
{
	return type == CP_FIELD_TYPE_DATE || type == CP_FIELD_TYPE_TIME ||
		   type == CP_FIELD_TYPE_TIMESTAMP;
}

static int isdst()
{
	struct tm local;
	time_t t = time(NULL);
	localtime_r(&t, &local);
	return local.tm_isdst;
}

static cp_timestampz *mysqltime2cp_timestampz(MYSQL_TIME *mtime)
{
	struct tm ltime;
	time_t caltime;
	
	memset(&ltime, 0, sizeof(struct tm));
	ltime.tm_sec = mtime->second;
	ltime.tm_min = mtime->minute;
	ltime.tm_hour = mtime->hour;
	ltime.tm_mday = mtime->day ? mtime->day : 1;
	ltime.tm_mon = mtime->month ? mtime->month - 1 : 0;
	ltime.tm_year = mtime->year ? mtime->year - 1900 : 70;
	ltime.tm_isdst = isdst(); //~~ MYSQL_TIME doesn't disclose timezone/dst

	caltime = mktime(&ltime);
	return cp_timestampz_create_prm(caltime, 0, 0);
}

static MYSQL_TIME *cp_timestampz2mysqltime(cp_timestampz *tmz)
{
	MYSQL_TIME *res = calloc(1, sizeof(MYSQL_TIME));
	struct tm ltime;
	
	if (res == NULL) return NULL;
	
	localtime_r(&tmz->tm.tv_sec, &ltime);

	res->second = ltime.tm_sec;
	res->minute = ltime.tm_min;
	res->hour = ltime.tm_hour;
	res->day = ltime.tm_mday;
	res->month = ltime.tm_mon + 1;
	res->year = ltime.tm_year + 1900;

	return res;
}

cp_vector *cp_mysql_bind_desc_get_row(cp_mysql_bind_desc *desc)
{
	int i;
	cp_vector *v = cp_vector_create(desc->len); //~~ NOSYNC etc
	void *p;
	size_t len;
	
	for (i = 0; i < desc->len; i++)
	{
		switch (desc->bind[i].buffer_type)
		{
			case MYSQL_TYPE_TINY: len = sizeof(short); break;
			case MYSQL_TYPE_SHORT: len = sizeof(short); break;
			case MYSQL_TYPE_LONG: len = sizeof(long); break;
#ifdef HAS_LONG_LONG_INT
			case MYSQL_TYPE_LONGLONG: len = sizeof(long long int); break;
#else
#ifdef _WINDOWS
			case MYSQL_TYPE_LONGLONG: len = sizeof(ULARGE_INTEGER); break;
#else
			case MYSQL_TYPE_LONGLONG: len = sizeof(long); break;
#endif /* _WINDOWS */
#endif /* HAS_LONG_LONG_INT */
			case MYSQL_TYPE_FLOAT: len = sizeof(float); break;
			case MYSQL_TYPE_DOUBLE: len = sizeof(double); break;
			case MYSQL_TYPE_TIME: 
			case MYSQL_TYPE_DATE: 
			case MYSQL_TYPE_DATETIME: 
			case MYSQL_TYPE_TIMESTAMP:
				p = mysqltime2cp_timestampz((MYSQL_TIME *) desc->buf[i]);
				cp_vector_set_element(v, i, p);
				len = 0; 
				break;
			case MYSQL_TYPE_STRING:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_BLOB: 
				p = cp_string_create(desc->buf[i], desc->length[i]);
				cp_vector_set_element(v, i, p);
				len = 0;
				break;
			default: len = 0; break; //~~ err msg								  
		}

		if (len > 0)
		{
			p = malloc(len);
			memcpy(p, desc->buf[i], len);
			cp_vector_set_element(v, i, p);
		}
	}

	return v;
}


void cp_mysql_shutdown()
{
	if (!initialized) return;
	initialized = 0;

	if (cp_mysql_db_actions) 
	{
		cp_db_actions_destroy(cp_mysql_db_actions);
		cp_mysql_db_actions = NULL;
	}

	if (mysql_field_type_map) 
	{
		cp_hashtable_destroy(mysql_field_type_map);
		mysql_field_type_map = NULL;
	}
}

	
void cp_mysql_init()
{
	int id;
	int i, types;
	if (initialized) return;
	initialized = 1;

	id = cp_db_register_dbms(DBNAME, cp_mysql_shutdown);

	cp_mysql_db_actions = 
		cp_db_actions_create(id, DBNAME, cp_mysql_open_conn, cp_mysql_select, 
				cp_mysql_fetch_metadata, cp_mysql_fetch_next, 
				cp_mysql_release_result_set, cp_mysql_update, 
				cp_mysql_close_conn, cp_mysql_escape_string, 
				cp_mysql_escape_binary, NULL, cp_mysql_prepare_statement, 
				cp_mysql_execute_statement, cp_mysql_release_statement,
				cp_mysql_set_autocommit, cp_mysql_commit, cp_mysql_rollback);

	types = sizeof(mysql2cprops) / sizeof(field_type_map);
	mysql_field_type_map = 
		cp_hashtable_create_by_mode(COLLECTION_MODE_NOSYNC, types, 
									cp_hash_int, cp_hash_compare_int);
	for (i = 0; i < types; i++)
		cp_hashtable_put(mysql_field_type_map, 
						 &mysql2cprops[i].mysql_field_code,
						 &mysql2cprops[i].cprops_field_code);
}

static void 
	cp_mysql_connection_parameters_delete(cp_mysql_connection_parameters *prm)
{
	if (prm)
	{
		if (prm->host) free(prm->host);
		if (prm->login) free(prm->login);
		if (prm->password) free(prm->password);
		if (prm->db_name) free(prm->db_name);
		if (prm->unix_socket) free(prm->unix_socket);
		cp_free(prm);
	}
}

cp_data_source *
	cp_mysql_data_source(char *host, 
						 char *login, 
						 char *password, 
						 char *db_name, 
						 unsigned int port,
						 char *unix_socket, 
						 unsigned long client_flag)
{
	cp_data_source *data_source;
	cp_mysql_connection_parameters *impl;

	if (!initialized) cp_mysql_init();

	data_source = cp_calloc(1, sizeof(cp_data_source));
	if (data_source == NULL) return NULL;

	data_source->act = cp_mysql_db_actions;

	data_source->prm = cp_calloc(1, sizeof(cp_db_connection_parameters));
	if (data_source->prm == NULL)
	{
		cp_free(data_source);
		return NULL;
	}
	impl = cp_calloc(1, sizeof(cp_mysql_connection_parameters));
	if (impl == NULL)
	{
		cp_free(data_source->prm);
		cp_free(data_source);
		return NULL;
	}

	impl->host = strdup(host);
	impl->port = port;
	impl->login = strdup(login);
	impl->password = password ? strdup(password) : NULL;
	impl->db_name = db_name ? strdup(db_name) : NULL;
	impl->unix_socket = unix_socket ? strdup(unix_socket) : NULL;
	impl->client_flag = client_flag;
	data_source->prm->impl = impl;
	data_source->prm->impl_dtr = 
		(cp_destructor_fn) cp_mysql_connection_parameters_delete;

	return data_source;
}

cp_data_source *
	cp_dbms_mysql_get_data_source(char *host, 
			                      int port, 
								  char *login, 
								  char *password,
								  char *dbname)
{
	return cp_mysql_data_source(host, login, password, dbname, port, NULL, 0);
}

cp_data_source *
	cp_dbms_mysql_get_data_source_prm(char *host,
			                          int port, 
									  char *login, 
									  char *password, 
									  char *dbname, 
									  cp_hashtable *prm)
{
	int *client_flag = cp_hashtable_get(prm, "client_flag");
	return cp_mysql_data_source(host, login, password, dbname, port, 
								   cp_hashtable_get(prm, "unix_socket"),
								   client_flag ? (*client_flag) : 0);
}


static cp_db_connection *cp_mysql_open_conn(cp_data_source *data_source)
{
	MYSQL *conn = NULL;
	cp_mysql_connection_parameters *conf = data_source->prm->impl;
	cp_db_connection *res;

	conn = malloc(sizeof(MYSQL));
	if (conn == NULL)
	{
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, 
				"can\'t allocate MYSQL connection handle");
		return NULL;
	}

	if (!mysql_init(conn))
	{
		cp_error(CP_DBMS_CONNECTION_FAILURE,
				"can\'t initialize MYSQL connection");
		return NULL;
	}

	if (!mysql_real_connect(conn, conf->host, conf->login, conf->password, 
							conf->db_name, conf->port, conf->unix_socket, 
							conf->client_flag))
	{
		cp_error(CP_DBMS_CONNECTION_FAILURE, 
				"can\'t connect to mysql dbms:%s", mysql_error(conn));
		free(conn);
		return NULL;
	}

	res = cp_calloc(1, sizeof(cp_db_connection));
	res->data_source = data_source;
	res->connection = conn;
	res->autocommit = 1;
	return res;
}
	
static cp_field_type convert_field_type(int field_type)
{
	cp_field_type *type = cp_hashtable_get(mysql_field_type_map, &field_type);
	return type ? *type : field_type;
}

static cp_result_set *create_mysql_result_set(cp_db_connection *_conn, 
		                                      MYSQL_RES *src, 
											  int fetch_metadata)
{
	cp_result_set *res = cp_calloc(1, sizeof(cp_result_set));
	if (res == NULL) return NULL;

	res->connection = _conn;
	res->source = src;
	res->field_count = mysql_num_fields(src);

	res->results = cp_list_create();

	if (fetch_metadata) cp_mysql_fetch_metadata(res);

	return res;
}

static int fetch_rows(cp_result_set *result_set, MYSQL_RES *src, int count)
{
	int i, j;
	MYSQL_ROW row;
	long *lengths;
	cp_vector *curr;

	for (i = 0; i < count; i++)
	{
		if ((row = mysql_fetch_row(src)) == NULL) break;
		result_set->row_count++;
		lengths = mysql_fetch_lengths(src);
		curr = cp_vector_create_by_option(result_set->field_count, 
					COLLECTION_MODE_DEEP, 
					NULL, (cp_destructor_fn) cp_string_destroy);
		for (j = 0; j < result_set->field_count; j++)
		{
			if (row[j])
				cp_vector_set_element(curr, j, 
						cp_string_create(row[j], lengths[j]));
		}
		cp_list_append(result_set->results, curr);
	}

	return i;
}

static void set_fetch_complete(cp_result_set *rs)
{
	rs->fetch_complete = 1;
	if (rs->store) 
	{
		cp_mysql_bind_desc_destroy(rs->store);
		rs->store = NULL;
	}
}

void fetch_stmt_rows(cp_result_set *rs, int count)
{
	cp_mysql_bind_desc *desc = rs->store;
	cp_db_statement *stmt = rs->source;
	cp_vector *row;
	int i;
	int rc;

	for (i = 0; count == 0 || i < count; i++)
	{
		if ((rc = mysql_stmt_fetch(stmt->source))) 
		{
			set_fetch_complete(rs);
			break;
		}

		row = cp_mysql_bind_desc_get_row(desc);
		rs->row_count++;
		cp_list_append(rs->results, row);
	}
}

static cp_result_set *cp_mysql_select(cp_db_connection *_conn, char *query)
{
	int rc = -1;
	MYSQL *conn = _conn->connection;
	MYSQL_RES *src;
	cp_result_set *res;
	
	if ((rc = mysql_query(conn, query)) != 0)
	{
		cp_error(CP_DBMS_QUERY_FAILED, "%s\n%s: %s", query, 
				 _conn->data_source->act->dbms_lit, mysql_error(conn));
		return NULL;
	}

	/* handle read-at-once */
	if (_conn->read_result_set_at_once || _conn->fetch_size == 0)
	{
		src = mysql_store_result(conn);
		if (src == NULL)
		{
			cp_error(CP_DBMS_CLIENT_ERROR, "%s: can\'t retrieve result", 
					 _conn->data_source->act->dbms_lit);
			cp_error(CP_DBMS_CLIENT_ERROR, "query: %s", query);
			return NULL;
		}
		res = create_mysql_result_set(_conn, src, _conn->autofetch_query_metadata);
		fetch_rows(res, src, mysql_num_rows(src));
		set_fetch_complete(res);
		mysql_free_result(src);
	}
	else /* perform chunked read */
	{
		src = mysql_use_result(conn);
		if (src == NULL) 
		{
			cp_error(CP_DBMS_CLIENT_ERROR, "%s: can\'t retrieve result", 
					 _conn->data_source->act->dbms_lit);
			cp_error(CP_DBMS_CLIENT_ERROR, "query: %s", query);
			return NULL;
		}
		res = create_mysql_result_set(_conn, src, _conn->autofetch_query_metadata);
		fetch_rows(res, src, _conn->fetch_size);
	}
	
	return res;
}

static int cp_mysql_fetch_metadata(cp_result_set *res)
{
	int i;
	MYSQL_FIELD *field;
	MYSQL_RES *src = res->source;
	if (src == NULL ||
		res->field_headers != NULL ||
		res->field_types != NULL) return -1;
	
	res->field_headers = cp_vector_create(res->field_count);
	res->field_types = 
		cp_vector_create_by_option(res->field_count, 
				COLLECTION_MODE_COPY | COLLECTION_MODE_DEEP | 
				COLLECTION_MODE_MULTIPLE_VALUES, 
				(cp_copy_fn) intdup, (cp_destructor_fn) free);
	for (i = 0; i < res->field_count; i++)
	{
		int type; 
		field = mysql_fetch_field(src);
		type = convert_field_type(field->type); 
		cp_vector_set_element(res->field_headers, i, strdup(field->name));
		cp_vector_set_element(res->field_types, i, &type);
	}
	
	return 0;
}

static int cp_mysql_fetch_next(cp_result_set *result_set)
{
	MYSQL_RES *src = result_set->source;
	if (result_set->fetch_complete) return 0;

	if (result_set->position == result_set->row_count)
	{
		if (result_set->store)
			fetch_stmt_rows(result_set, result_set->connection->fetch_size);
		else
			fetch_rows(result_set, src, result_set->row_count);
	}

	return 0;
}

static int cp_mysql_release_result_set(cp_result_set *result_set)
{
	int rc = -1;

	if (result_set)
	{
		if (result_set->source)
		{
			MYSQL_RES *src = result_set->source;
			while ((mysql_fetch_row(src)) != NULL); /* flush result buffer */
			mysql_free_result(src);
			result_set->source = NULL;
			rc = 0;
		}

		if (result_set->store)
			cp_mysql_bind_desc_destroy(result_set->store);
	}
	
	return rc;
}

static int cp_mysql_update(cp_db_connection *_conn, char *query)
{
	int rc = -1;
	MYSQL *conn = _conn->connection;
	
	if (conn)
		rc = mysql_query(conn, query);

	return rc;
}

static int cp_mysql_close_conn(cp_db_connection *connection)
{
	MYSQL *conn = connection->connection;
	if (conn == NULL) return -1;
	
	mysql_close(conn);
	free(conn);
	connection->connection = NULL;
	return 0;
}

static char *cp_mysql_escape_string(cp_db_connection *conn, char *str, int len)
{
	MYSQL *mysql = conn->connection;
	char *res = malloc(2 * len + 1);

	if (res)
		mysql_real_escape_string(mysql, res, str, len);
	
	return res;
}

static char *cp_mysql_escape_binary(cp_db_connection *conn, char *str, 
									int len, int *res_len)
{
	MYSQL *mysql = conn->connection;
	char *res = malloc(2 * len + 1);

	if (res)
		*res_len = mysql_real_escape_string(mysql, res, str, len);
	
	return res;
}

int cprops2mysql(cp_field_type type)
{
	switch(type)
	{
		case CP_FIELD_TYPE_BOOLEAN: return MYSQL_TYPE_SHORT;
		case CP_FIELD_TYPE_CHAR: return MYSQL_TYPE_VARCHAR;
		case CP_FIELD_TYPE_SHORT: return MYSQL_TYPE_SHORT;
		case CP_FIELD_TYPE_INT: 
		case CP_FIELD_TYPE_LONG: return MYSQL_TYPE_LONG;
		case CP_FIELD_TYPE_LONG_LONG: return MYSQL_TYPE_LONGLONG;
		case CP_FIELD_TYPE_FLOAT: return MYSQL_TYPE_FLOAT;
		case CP_FIELD_TYPE_DOUBLE: return MYSQL_TYPE_DOUBLE;
		case CP_FIELD_TYPE_VARCHAR: return MYSQL_TYPE_VARCHAR;
		case CP_FIELD_TYPE_BLOB: return MYSQL_TYPE_BLOB;
		case CP_FIELD_TYPE_DATE: return MYSQL_TYPE_DATE;
		case CP_FIELD_TYPE_TIME: return MYSQL_TYPE_DATETIME;
		case CP_FIELD_TYPE_TIMESTAMP: return MYSQL_TYPE_DATETIME;
	}

	return MYSQL_TYPE_NULL;
}

static cp_db_statement *
	cp_mysql_prepare_statement(cp_db_connection *conn, int prm_count, 
			                   cp_field_type *prm_type, char *query)
{
	MYSQL_STMT *stmt; 
	cp_db_statement *res;
	stmt = mysql_stmt_init(conn->connection);
	if (!stmt)
	{
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, 
				"%s: can\'t allocate statement", conn->data_source->act->dbms_lit);
		return NULL;
	}
	
	if (mysql_stmt_prepare(stmt, query, strlen(query)))
	{
		cp_error(CP_DBMS_STATEMENT_ERROR, "%s: can\'t prepare statement: %s", 
				 conn->data_source->act->dbms_lit, mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return NULL;
	}

	res = cp_db_statement_instantiate(conn, prm_count, prm_type, stmt);
	if (res == NULL)
	{
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, "%s: can\'t allocate statement"
				 " wrapper", conn->data_source->act->dbms_lit);
		mysql_stmt_close(stmt);
		return NULL;
	}
	
	return res;
}


//~~ improve: allocate bind parameter structure beforehand and reuse
static int 
	cp_mysql_execute_statement(cp_db_connection *conn, cp_db_statement *stmt, 
							   cp_result_set **result_set, int *lengths,
							   void **prm)
{
	int i;
	int rc;
	MYSQL_BIND *bind = NULL;
	MYSQL_RES *meta = NULL;
	cp_result_set *rs = NULL;
	int field_count;
	my_bool *is_null = NULL;
	long *longlengths = calloc(stmt->prm_count, sizeof(long));
	long *reslengths;

	if (lengths)
		for (i = 0; i < stmt->prm_count; i++)
			longlengths[i] = lengths[i];

	if (stmt->prm_count)
	{
		bind = calloc(stmt->prm_count, sizeof(MYSQL_BIND));
		is_null = calloc(stmt->prm_count, sizeof(my_bool));
	}

	for (i = 0; i < stmt->prm_count; i++)
	{
		bind[i].buffer_type = cprops2mysql(stmt->types[i]);
		if (is_date_type(stmt->types[i]))
			bind[i].buffer = cp_timestampz2mysqltime(prm[i]);
		else
			bind[i].buffer = prm[i];
		bind[i].is_null = &is_null[i];
		if (lengths) bind[i].length = &longlengths[i];

		if(mysql_stmt_bind_param(stmt->source, bind))
		{
			cp_error(CP_DBMS_STATEMENT_ERROR, "%s: can\'t bind parameter: %s", 
				conn->data_source->act->dbms_lit, mysql_stmt_error(stmt->source));
			return -1;
		}
	}

	if (mysql_stmt_execute(stmt->source))
	{
		cp_error(CP_DBMS_QUERY_FAILED, "%s: prepared statement: %s", 
				 conn->data_source->act->dbms_lit, mysql_stmt_error(stmt->source));
		//~~ clean up
		return -1;
	}

	for (i = 0; i < stmt->prm_count; i++)
		if (is_date_type(stmt->types[i]))
			free(bind[i].buffer);
	if (bind) free(bind);

	rc = mysql_stmt_affected_rows(stmt->source);

	/* if stmt returns a result set, meta data should be available */
	meta = mysql_stmt_result_metadata(stmt->source);
	if (!meta) goto DONE;

	/* if it's a result set, fetch 'em */
	field_count = mysql_num_fields(meta);
	if (field_count) 
	{
		MYSQL_FIELD *field;
		bind = calloc(field_count, sizeof(MYSQL_BIND));
		reslengths = calloc(field_count, sizeof(long));
		rs = create_mysql_result_set(conn, meta, 0);
		rs->binary = 1; /* results for mysql prepared stmts come in binary format */
		rs->source = stmt;

		rs->field_headers = 
			cp_vector_create_by_option(rs->field_count, 
					COLLECTION_MODE_COPY | COLLECTION_MODE_DEEP | 
					COLLECTION_MODE_MULTIPLE_VALUES,
					(cp_copy_fn) strdup, (cp_destructor_fn) free);
		rs->field_types = 
			cp_vector_create_by_option(rs->field_count, 
					COLLECTION_MODE_COPY | COLLECTION_MODE_DEEP | 
					COLLECTION_MODE_MULTIPLE_VALUES, 
					(cp_copy_fn) intdup, (cp_destructor_fn) free);
		for (i = 0; i < rs->field_count; i++)
		{
			int type; 
			field = mysql_fetch_field(meta);
			type = convert_field_type(field->type); 
			cp_vector_set_element(rs->field_headers, i, field->name);
			cp_vector_set_element(rs->field_types, i, &type);
			bind[i].buffer_type = field->type;
			reslengths[i] = field->max_length;
			bind[i].length = &reslengths[i];
		}

		rs->store = 
			create_mysql_bind_desc(field_count, bind, rs->field_types, reslengths);

		if (mysql_stmt_bind_result(stmt->source, bind))
		{
			cp_error(CP_DBMS_CLIENT_ERROR, "%s: error binding prepared "
				     "statement results: %s", conn->data_source->act->dbms_lit,
					 mysql_stmt_error(stmt->source));
			cp_result_set_destroy(rs);
			rs = NULL;
			goto DONE;
		}
	}
	
	if (conn->read_result_set_at_once || conn->fetch_size == 0)
	{
		if (mysql_stmt_store_result(stmt->source))
		{
			cp_error(CP_DBMS_CLIENT_ERROR, "%s: error retrieving prepared "
				     "statement results: %s", conn->data_source->act->dbms_lit,
					 mysql_stmt_error(stmt->source));
			cp_result_set_destroy(rs);
			rs = NULL;
			goto DONE;
		}

		fetch_stmt_rows(rs, 0); 
		rs->fetch_complete = 1;
	}
	else
		fetch_stmt_rows(rs, conn->fetch_size);

DONE:
	free(longlengths);
	free(is_null);
	mysql_free_result(meta);
	if (result_set)
		*result_set = rs;
	return rc;
}

static void cp_mysql_release_statement(cp_db_statement *statement)
{
	if (mysql_stmt_close(statement->source))
	{
		cp_error(CP_DBMS_CLIENT_ERROR, "%s: error closing prepared statement: %s",
				 statement->connection->data_source->act->dbms_lit,
				 mysql_stmt_error(statement->source));
	}
	if (statement->store) free(statement->store);
}

static void cp_mysql_set_autocommit(cp_db_connection *conn, int state)
{
	mysql_autocommit(conn->connection, state);
}	

static int cp_mysql_commit(cp_db_connection *conn)
{
	return mysql_commit(conn->connection);
}

static int cp_mysql_rollback(cp_db_connection *conn)
{
	return mysql_rollback(conn->connection);
}

