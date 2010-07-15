#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef _WINDOWS
#include "socket.h"
#endif /* _WINDOWS */
#include <time.h>
#include <stdio.h>

#include "config.h"
#include "common.h"
#include "db.h"
#include "str.h"
#include "util.h"
#include "hashtable.h"
#include "vector.h"
#include "log.h"

#include "db_postgres.h"

static cp_db_connection *pgsql_open_conn(cp_data_source *data_source);
static cp_result_set *pgsql_select(cp_db_connection *_conn, char *query);
static int pgsql_fetch_metadata(cp_result_set *result_set);
static int pgsql_fetch_next(cp_result_set *result_set);
static int pgsql_release_result_set(cp_result_set *result_set);
static int pgsql_update(cp_db_connection *_conn, char *query);
static int pgsql_close_conn(cp_db_connection *connection);
static char *pgsql_escape_string(cp_db_connection *conn, char *str, int len);
static char *pgsql_escape_binary(cp_db_connection *conn, char *str, 
		                         int len, int *res_len);
static cp_string *pgsql_unescape_binary(cp_db_connection *conn, char *str);
static cp_db_statement *
	pgsql_prepare_statement(cp_db_connection *conn, int prm_count, 
		                    cp_field_type *prm_type, char *query);
static int pgsql_execute_statement(cp_db_connection *conn, cp_db_statement *stmt, 
								   cp_result_set **result_set, int *lengths, 
								   void **prm);
static void pgsql_release_statement(cp_db_statement *statement);
static void pgsql_set_autocommit(cp_db_connection *conn, int state);
static int pgsql_commit(cp_db_connection *conn);
static int pgsql_rollback(cp_db_connection *conn);

volatile static int initialized = 0;
static cp_db_actions *pgsql_db_actions = NULL;
static cp_hashtable *pgsql_field_type_map = NULL;

#define DBNAME "postgres"

typedef struct _field_type_map
{
	int pgsql_field_code;
	int cprops_field_code;
} field_type_map;

field_type_map pgsql2cprops[] = 
{
	{    16, CP_FIELD_TYPE_BOOLEAN },   // bool
    {    17, CP_FIELD_TYPE_BLOB },   	// bytea
    {    18, CP_FIELD_TYPE_CHAR },   	// char
    {    19, CP_FIELD_TYPE_VARCHAR },   // name
    {    20, CP_FIELD_TYPE_LONG_LONG }, // int8
    {    21, CP_FIELD_TYPE_SHORT },     // int2
//  {    22, CP_FIELD_TYPE_ },          // int2vector
    {    23, CP_FIELD_TYPE_INT },       // int4
    {    25, CP_FIELD_TYPE_VARCHAR },   // text
    {    26, CP_FIELD_TYPE_INT },       // oid
    {   602, CP_FIELD_TYPE_VARCHAR },   // path
    {   700, CP_FIELD_TYPE_FLOAT },     // float4
  	{   701, CP_FIELD_TYPE_DOUBLE },    // float8
  	{   702, CP_FIELD_TYPE_DATE }, // abstime
  	{   703, CP_FIELD_TYPE_TIME }, // reltime
  	{   704, CP_FIELD_TYPE_TIME }, // tinterval
//  	{   790, CP_FIELD_TYPE_ }, // money
//  	{   791, CP_FIELD_TYPE_ }, // _money
  	{   829, CP_FIELD_TYPE_VARCHAR }, // macaddr
  	{   869, CP_FIELD_TYPE_VARCHAR }, // inet
//  	{   650, CP_FIELD_TYPE_ }, // cidr
 	{  1000, CP_FIELD_TYPE_BOOLEAN }, // _bool
 	{  1001, CP_FIELD_TYPE_VARCHAR }, // _bytea
 	{  1002, CP_FIELD_TYPE_VARCHAR }, // _char
 	{  1003, CP_FIELD_TYPE_VARCHAR }, // _name
	{  1005, CP_FIELD_TYPE_SHORT }, // _int2
//	{  1006, CP_FIELD_TYPE_ }, // _int2vector
	{  1007, CP_FIELD_TYPE_INT }, // _int4
	{  1009, CP_FIELD_TYPE_VARCHAR }, // _text
	{  1014, CP_FIELD_TYPE_VARCHAR }, // _bpchar
	{  1015, CP_FIELD_TYPE_VARCHAR }, // _varchar
	{  1016, CP_FIELD_TYPE_LONG_LONG }, // _int8
	{  1021, CP_FIELD_TYPE_FLOAT }, // _float4
	{  1022, CP_FIELD_TYPE_DOUBLE }, // _float8
	{  1023, CP_FIELD_TYPE_DATE }, // _abstime
	{  1024, CP_FIELD_TYPE_TIME }, // _reltime
	{  1025, CP_FIELD_TYPE_TIME }, // _tinterval
	{  1033, CP_FIELD_TYPE_VARCHAR }, // aclitem
	{  1034, CP_FIELD_TYPE_VARCHAR }, // _aclitem
	{  1040, CP_FIELD_TYPE_VARCHAR }, // _macaddr
	{  1041, CP_FIELD_TYPE_VARCHAR }, // _inet
//	{   651, CP_FIELD_TYPE_ }, // _cidr
	{  1042, CP_FIELD_TYPE_VARCHAR }, // bpchar
	{  1043, CP_FIELD_TYPE_VARCHAR }, // varchar
	{  1082, CP_FIELD_TYPE_DATE }, // date
	{  1083, CP_FIELD_TYPE_TIME }, // time
	{  1114, CP_FIELD_TYPE_TIMESTAMP }, // timestamp
	{  1115, CP_FIELD_TYPE_TIMESTAMP }, // _timestamp
	{  1182, CP_FIELD_TYPE_DATE }, // _date
	{  1183, CP_FIELD_TYPE_TIME }, // _time
	{  1184, CP_FIELD_TYPE_TIMESTAMP }, // timestamptz
	{  1185, CP_FIELD_TYPE_TIMESTAMP }, // _timestamptz
	{  1186, CP_FIELD_TYPE_TIME }, // interval
	{  1187, CP_FIELD_TYPE_TIME }, // _interval
	{  1231, CP_FIELD_TYPE_INT }, // _numeric
	{  1266, CP_FIELD_TYPE_TIME }, // timetz
	{  1270, CP_FIELD_TYPE_TIME }, // _timetz
	{  1560, CP_FIELD_TYPE_BOOLEAN }, // bit
	{  1561, CP_FIELD_TYPE_BOOLEAN }, // _bit
	{  1562, CP_FIELD_TYPE_LONG }, // varbit
	{  1563, CP_FIELD_TYPE_LONG }, // _varbit
	{  1700, CP_FIELD_TYPE_LONG }, // numeric
	{  2275, CP_FIELD_TYPE_VARCHAR }, // cstring
	{  2276, CP_FIELD_TYPE_VARCHAR }, // any
	{ 10635, CP_FIELD_TYPE_LONG }, // cardinal_number
	{ 10637, CP_FIELD_TYPE_VARCHAR } // character_data
};

void cp_pgsql_shutdown()
{
	if (!initialized) return;
	initialized = 0;

	if (pgsql_db_actions)
	{
		cp_db_actions_destroy(pgsql_db_actions);
		pgsql_db_actions = NULL;
	}

	if (pgsql_field_type_map)
	{
		cp_hashtable_destroy(pgsql_field_type_map);
		pgsql_field_type_map = NULL;
	}
}

void cp_dbms_postgres_init()
{
	int id;
	int i, types;
	if (initialized) return;
	initialized = 1;

#ifdef _WINDOWS
	/* must initialize winsock or else */
	cp_socket_init();
#endif /* _WINDOWS */

	id = cp_db_register_dbms(DBNAME, cp_pgsql_shutdown);

	pgsql_db_actions = 
		cp_db_actions_create(id, DBNAME, pgsql_open_conn, pgsql_select, 
				pgsql_fetch_metadata, pgsql_fetch_next, 
				pgsql_release_result_set, pgsql_update, pgsql_close_conn,
				pgsql_escape_string, pgsql_escape_binary, 
				pgsql_unescape_binary, pgsql_prepare_statement, 
				pgsql_execute_statement, pgsql_release_statement,
				pgsql_set_autocommit, pgsql_commit, pgsql_rollback);

	types = sizeof(pgsql2cprops) / sizeof(field_type_map);
	pgsql_field_type_map = 
		cp_hashtable_create_by_mode(COLLECTION_MODE_NOSYNC, types,
									cp_hash_int, cp_hash_compare_int);
	for (i = 0; i < types; i++)
		cp_hashtable_put(pgsql_field_type_map,
						 &pgsql2cprops[i].pgsql_field_code,
						 &pgsql2cprops[i].cprops_field_code);
}

static void 
	cp_pgsql_connection_parameters_delete(cp_pgsql_connection_parameters *prm)
{
	if (prm)
	{
		if (prm->host) free(prm->host);
		if (prm->login) free(prm->login);
		if (prm->password) free(prm->password);
		if (prm->db_name) free(prm->db_name);
		if (prm->options) free(prm->options);
		if (prm->sslmode) free(prm->sslmode);
		if (prm->krbsrvname) free(prm->krbsrvname);
		if (prm->service) free(prm->service);
	}
}

cp_data_source *
	cp_postgres_data_source(char *host, 
						    int port, 
						    char *login, 
						    char *password, 
						    char *db_name,
						    char *options,
						    char *sslmode,
						    char *krbsrvname, 
						    char *service)
{
	cp_data_source *data_source;
	cp_pgsql_connection_parameters *impl;
	
	if (!initialized) cp_dbms_postgres_init();

	data_source = cp_calloc(1, sizeof(cp_data_source));
	if (data_source == NULL) return NULL;

	data_source->act = pgsql_db_actions;

	data_source->prm = cp_calloc(1, sizeof(cp_db_connection_parameters));
	if (data_source->prm == NULL)
	{
		cp_free(data_source);
		return NULL;
	}
	impl = calloc(1, sizeof(cp_pgsql_connection_parameters));
	if (impl == NULL)
	{
		cp_free(data_source->prm);
		cp_free(data_source);
		return NULL;
	}
	//~~ check for alloc failures
	impl->host = host ? strdup(host) : NULL;
	impl->port = port;
	impl->login = login ? strdup(login) : NULL;
	impl->password = password ? strdup(password) : NULL;
	impl->db_name = db_name ? strdup(db_name) : NULL;
	impl->options = options ? strdup(options) : NULL;
	impl->sslmode = sslmode ? strdup(sslmode) : NULL;
	impl->krbsrvname = krbsrvname ? strdup(krbsrvname) : NULL;
	impl->service = service ? strdup(service) : NULL;

	data_source->prm->impl = impl;
	data_source->prm->impl_dtr = 
		(cp_destructor_fn) cp_pgsql_connection_parameters_delete;

	return data_source;
}

cp_data_source *
	cp_dbms_postgres_get_data_source(char *host, 
			                         int port, 
									 char *login, 
									 char *password,
									 char *dbname)
{
	return cp_postgres_data_source(host, port, login, password, dbname, 
								   NULL, NULL, NULL, NULL);
}

cp_data_source *
	cp_dbms_postgres_get_data_source_prm(char *host,
			                             int port, 
										 char *login, 
										 char *password, 
										 char *dbname, 
										 cp_hashtable *prm)
{
	return cp_postgres_data_source(host, port, login, password, dbname, 
								   cp_hashtable_get(prm, "options"),
								   cp_hashtable_get(prm, "sslmode"),
								   cp_hashtable_get(prm, "krbsrvname"),
								   cp_hashtable_get(prm, "service"));
}

static int pgsql_close_conn(cp_db_connection *connection)
{
	void *conn = connection->connection;
    if (conn == NULL) return -1;
	PQfinish(conn); 
    return 0;
}

void cs_append(cp_string *cs, char *title, char *value)
{
	cp_string_cstrcat(cs, " ");
	cp_string_cstrcat(cs, title);
	cp_string_cstrcat(cs, "=");
	cp_string_cstrcat(cs, value);
}

static cp_db_connection *pgsql_open_conn(cp_data_source *data_source)
{
    PGconn *conn = NULL;
	cp_string *cs = cp_string_create("", 0);
	cp_pgsql_connection_parameters *conf = data_source->prm->impl;
	cp_db_connection *res;

	if (conf->host) cs_append(cs, "host", conf->host);
	if (conf->port) 
	{
		char buf[32];
#ifdef HAVE_SNPRINTF
		snprintf(buf, 32, " port=%d", conf->port);
#else
		sprintf(buf, " port=%d", conf->port);
#endif /* HAVE_SNPRINTF */
		cp_string_cstrcat(cs, buf);
	}
	if (conf->login) cs_append(cs, "user", conf->login);
	if (conf->password) cs_append(cs, "password", conf->password);
	if (conf->db_name) cs_append(cs, "dbname", conf->db_name);
	if (conf->options) cs_append(cs, "options", conf->options);
	if (conf->sslmode) cs_append(cs, "sslmode", conf->sslmode);
	if (conf->krbsrvname) cs_append(cs, "krbsrvname", conf->krbsrvname);
	if (conf->service) cs_append(cs, "service", conf->service);

    conn = PQconnectdb(cp_string_tocstr(cs));

	cp_string_delete(cs);

	if (conn == NULL) return NULL;

    if (PQstatus(conn) == CONNECTION_BAD) 
	{
        cp_error(CP_DBMS_CONNECTION_FAILURE, 
				 "PGSQL: connection to database %s failed", conf->db_name); 
		cp_error(CP_DBMS_CONNECTION_FAILURE, 
				 "PGSQL: %s", PQerrorMessage(conn));
        goto failed;
    }

#ifdef __TRACE__
    cp_info("PGSQL: Connected to server at %s", conf->host);
#endif

	/* good. now set up connection wrapper */
	res = cp_calloc(1, sizeof(cp_db_connection));
	res->data_source = data_source;
	res->connection = conn;
	res->read_result_set_at_once = 1;
	res->autocommit = 1;
    return res;

failed:
	PQfinish(conn); 
    return NULL;
}

static int pgsql_result_error(PGresult *res)
{
 	switch(PQresultStatus(res)) 
	{
    	case PGRES_EMPTY_QUERY:
   		case PGRES_BAD_RESPONSE:
	    case PGRES_NONFATAL_ERROR:
		case PGRES_FATAL_ERROR:
       		cp_error(1, "PGSQL: %s", PQresultErrorMessage(res));
			PQclear(res);
    		return 1;
	
		default: 
   			break;
    }

	return 0;
}

static cp_result_set *
	create_result_set(cp_db_connection *_conn, PGresult *res)
{
	cp_result_set *result_set = cp_calloc(1, sizeof(cp_result_set));
	result_set->field_count = PQnfields(res);
	result_set->row_count = PQntuples(res);
	result_set->results = cp_list_create();
	result_set->connection = _conn;
	result_set->source = res;

//  always fetch meta data - column types needed to decide whether to unescape 
//  BYTEA columns
	pgsql_fetch_metadata(result_set);

	return result_set;
}

cp_string *pgsql_unescape_binary(cp_db_connection *conn, char *str)
{
	int newlen;
	char *unescaped = PQunescapeBytea(str, &newlen);
	cp_string *res = cp_string_create(unescaped, newlen);
	PQfreemem(unescaped);
	return res;
}
	
static int fetch_rows(cp_result_set *result_set, PGresult *res)
{
	int i, j;
	cp_list *list = result_set->results;
	cp_vector *rec;
	cp_field_type *type; 

	for (i = 0; i < result_set->row_count; i++)
	{
		/* check for chunked reading */
		if (!result_set->connection->read_result_set_at_once 
				&& result_set->connection->fetch_size != 0 
				&& i == result_set->connection->fetch_size) break;

		if (result_set->binary)
			rec = cp_vector_create(result_set->field_count);
		else
			rec = cp_vector_create_by_option(result_set->field_count, 
				COLLECTION_MODE_DEEP, 
				NULL, (cp_destructor_fn) cp_string_destroy);

		for (j = 0; j < result_set->field_count; j++)
		{
			if (!PQgetisnull(res, i, j))
			{
				type = cp_vector_element_at(result_set->field_types, j);
				if (result_set->binary)
				{
					switch (*type)
					{
						case CP_FIELD_TYPE_BOOLEAN:
						{
							char *b = calloc(1, sizeof(short));
							short *p = (short *) b;
							memcpy(b, PQgetvalue(res, i, j), 1);
							*p = (short) *b;
							cp_vector_set_element(rec, j, p);
						}
						break;

						case CP_FIELD_TYPE_SHORT:
						{
							short *p = malloc(sizeof(short));
							memcpy(p, PQgetvalue(res, i, j), sizeof(short));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
							*p = ntohs(*p);
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							cp_vector_set_element(rec, j, p);
						}
						break;

						case CP_FIELD_TYPE_INT:
						{
							int *p = malloc(sizeof(int));
							memcpy(p, PQgetvalue(res, i, j), sizeof(int));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
							*p = ntohl(*p);
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							cp_vector_set_element(rec, j, p);
						}
						break;

						case CP_FIELD_TYPE_LONG:
						{
							long *p = malloc(sizeof(long));
							memcpy(p, PQgetvalue(res, i, j), sizeof(long));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
							if (sizeof(long) == sizeof(int))
								*p = ntohl(*p);
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							cp_vector_set_element(rec, j, p);
						}
						break;

						case CP_FIELD_TYPE_FLOAT:
						{
							float *p = calloc(1, sizeof(float));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
							int *q = (int *) p;
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							memcpy(p, PQgetvalue(res, i, j), sizeof(float));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
							*q = ntohl(*q);
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							cp_vector_set_element(rec, j, p);
						}
						break;

						case CP_FIELD_TYPE_DOUBLE:
						{
							double *p = malloc(sizeof(double));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
							int *q = (int *) p;
							int w;
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							memcpy(p, PQgetvalue(res, i, j), sizeof(double));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN 
							/* believe it or not */
							q[0] = ntohl(q[0]);
							q[1] = ntohl(q[1]);
							w = q[0];
							q[0] = q[1];
							q[1] = w;
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							cp_vector_set_element(rec, j, p);
						}
						break;

						case CP_FIELD_TYPE_TIME:
						case CP_FIELD_TYPE_DATE:
						case CP_FIELD_TYPE_TIMESTAMP:
						{
							union 
							{
								double p;
								int q[2];
							} up;
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN 
							int w;
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							cp_timestampz *t = calloc(1, sizeof(cp_timestampz));
							memcpy(&up.p, PQgetvalue(res, i, j), sizeof(double));
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN 
							up.q[0] = ntohl(up.q[0]);
							up.q[1] = ntohl(up.q[1]);
							w = up.q[0];
							up.q[0] = up.q[1];
							up.q[1] = w;
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
							up.p += (30 * 365 + 7) * 24 * 3600;
							t->tm.tv_sec = up.p;
							t->tm.tv_usec = (up.p - t->tm.tv_sec) * 1000000;
							cp_vector_set_element(rec, j, t);
						}
						break;
						
						default:
							cp_vector_set_element(rec, j, 
								cp_string_create(PQgetvalue(res, i, j), 
												 PQgetlength(res, i, j)));
							break;
					}
				}
				else if (*type == CP_FIELD_TYPE_BLOB)
					cp_vector_set_element(rec, j, 
						pgsql_unescape_binary(NULL, PQgetvalue(res, i, j)));
				else
					cp_vector_set_element(rec, j, 
							cp_string_create(PQgetvalue(res, i, j), 
											 PQgetlength(res, i, j)));
			}
		}

		cp_list_append(list, rec);
	}

	if (i == result_set->row_count) 
	{
		result_set->fetch_complete = 1;
		PQclear(res);
		result_set->source = NULL;
	}
			
	return i;
}

static cp_result_set *pgsql_select(cp_db_connection *_conn, char *query)
{
	int rc;
	PGresult *res = NULL;
	PGconn *conn = (PGconn *) _conn->connection;
	cp_result_set *result_set = NULL;

	rc = PQsendQuery(conn, query);
	while (PQflush(conn) == 1);
	if (rc == 0) 
	{
		cp_error(CP_DBMS_QUERY_FAILED, "%s", query);
		return NULL;
	}

	if ((res = PQgetResult(conn)) != NULL)
	{
		result_set = create_result_set(_conn, res);
		do 
		{
			if (pgsql_result_error(res)) return NULL;
			fetch_rows(result_set, res);
			/* if doing chunked reading, it's time to break */
			if (!_conn->read_result_set_at_once) break;
		}
		while ((res = PQgetResult(conn)) != NULL);
	}
	else while (PQflush(conn) == 1);

	return result_set;
}

static int convert_field_type(int field_type)
{
	int *type = cp_hashtable_get(pgsql_field_type_map, &field_type);
	return type != NULL ? *type : field_type;
}

static int pgsql_fetch_metadata(cp_result_set *result_set)
{
	int rc = -1;
	int j;

	if (result_set->field_headers == NULL)
	{
		PGresult *res = result_set->source;
		result_set->field_headers = 
			cp_vector_create_by_option(result_set->field_count, 
									   COLLECTION_MODE_NOSYNC | 
									   COLLECTION_MODE_COPY | 
									   COLLECTION_MODE_DEEP |
									   COLLECTION_MODE_MULTIPLE_VALUES, 
									   (cp_copy_fn) strdup, 
									   (cp_destructor_fn) free);
		result_set->field_types = 
			cp_vector_create_by_option(result_set->field_count, 
									   COLLECTION_MODE_NOSYNC | 
									   COLLECTION_MODE_COPY | 
									   COLLECTION_MODE_DEEP |
									   COLLECTION_MODE_MULTIPLE_VALUES,
									   (cp_copy_fn) intdup, 
									   (cp_destructor_fn) free);
		for (j = 0; j < result_set->field_count; j++)
		{
			int type = convert_field_type(PQftype(result_set->source, j));
			cp_vector_set_element(result_set->field_headers, j, 
					              PQfname(res, j));
			cp_vector_set_element(result_set->field_types, j, &type);
		}
		rc = 0;
	}

	return rc;
}

static int pgsql_fetch_next(cp_result_set *result_set)
{
	int i, j;
	PGresult *res = result_set->source;
	PGconn *conn = (PGconn *) result_set->connection->connection;
	cp_vector *rec;

	while (res)
	{
		if (pgsql_result_error(res)) return -1;

		for (i = result_set->position; i < result_set->row_count; i++)
		{
			if (((i + 1) % result_set->connection->fetch_size) == 0) break;

			rec = cp_vector_create_by_option(result_set->field_count, 
					COLLECTION_MODE_DEEP, 
					NULL, (cp_destructor_fn) cp_string_destroy);

			for (j = 0; j < result_set->field_count; j++)
			{
				if (!PQgetisnull(res, i, j))
					cp_vector_set_element(rec, j, 
						cp_string_create(PQgetvalue(res, i, j), 
							             PQgetlength(res, i, j)));
			}

			cp_list_append(result_set->results, rec);
		}

		if (i == result_set->row_count) 
		{
			result_set->fetch_complete = 1;
			PQclear(res);
			result_set->source = NULL;
		}
		else /* read chunk, return */
			break;

		res = PQgetResult(conn);
	}

	return 0;
}

static int pgsql_release_result_set(cp_result_set *result_set)
{
	if (result_set->source)
	{
		PGresult *res = result_set->source;
		PGconn *conn = (PGconn *) result_set->connection->connection;
		do
		{
			PQclear(res);
		} while ((res = PQgetResult(conn)) != NULL);
	}

	return 0;
}

static int pgsql_update(cp_db_connection *_conn, char *query)
{
	int rows = 0;
    PGresult *res = NULL;
    PGconn *conn = (PGconn*) _conn->connection;
	int rc;

	rc = PQsendQuery(conn, query);
	while (PQflush(conn) == 1);
	if (rc == 0) 
	{
		cp_error(CP_DBMS_QUERY_FAILED, "%s", query);
		return -1;
	}

	while ((res = PQgetResult(conn)) != NULL)
	{
	    switch (PQresultStatus(res)) 
		{
	    	case PGRES_BAD_RESPONSE:
	    	case PGRES_NONFATAL_ERROR:
		    case PGRES_FATAL_ERROR:
				cp_error(CP_DBMS_QUERY_FAILED, "postgres: %s", query);
				cp_error(CP_DBMS_QUERY_FAILED, 
						 "postgres: %s", PQresultErrorMessage(res));
				PQclear(res);
    	    	return -1;

    		default:
    			rows += atoi(PQcmdTuples(res));
	        	break;
	    }
    	PQclear(res);
	}

    return rows;
}

static char *pgsql_escape_string(cp_db_connection *conn, char *src, int len)
{
	char *res = malloc(len * 2 + 1); /* postgres spec */
	PQescapeString(res, src, len);
	return res;
}

static char *pgsql_escape_binary(cp_db_connection *conn, char *src, 
		                         int len, int *res_len)
{
	return PQescapeBytea(src, len, res_len);
}

//~~ magic numbers are postgres OIDs - maybe include 
//~~ postgresql/server/catalog/pg_type.h instead
static Oid cprops2pgsql(cp_field_type type)
{
	switch(type)
	{
		case CP_FIELD_TYPE_BOOLEAN: return 16;
		case CP_FIELD_TYPE_CHAR: return 1043; // 18
		case CP_FIELD_TYPE_SHORT:return 21;
		case CP_FIELD_TYPE_INT: return 23;
		case CP_FIELD_TYPE_LONG: return sizeof(long) == sizeof(int) ? 23 : 20;
		case CP_FIELD_TYPE_LONG_LONG: return 20;
		case CP_FIELD_TYPE_FLOAT: return 700;
		case CP_FIELD_TYPE_DOUBLE: return 701;
		case CP_FIELD_TYPE_VARCHAR: return 1043;
		case CP_FIELD_TYPE_BLOB: return 17;
		case CP_FIELD_TYPE_DATE: return 1114; // 1082
		case CP_FIELD_TYPE_TIME: return 1083;
		case CP_FIELD_TYPE_TIMESTAMP: return 1114;
		default: return 2276;
	}

	return 2276;
}

char *format_prm(char *query, int prm_count)
{
	char *subst = query;
	char marker[5]; /* no more than 999 parameters */
	int count = 1;
	
	if (prm_count)
	{
		cp_string *buf = NULL;
		char *curr = query;
		char *prev = NULL;
		while  ((curr = strchr(curr, '?')) != NULL)
		{
			if (buf == NULL) 
			{
				buf = cp_string_create("", 0);
				prev = query;
			}

			cp_string_cat_bin(buf, prev, curr - prev);
#ifdef HAVE_SNPRINTF
			snprintf(marker, 5, "$%d", count);
#else
			sprintf(marker, "$%d", count);
#endif /* HAVE_SNPRINTF */
			count++;
			cp_string_cstrcat(buf, marker);
			prev = ++curr;
		}
		if (prev) cp_string_cstrcat(buf, prev);
		if (buf)
		{
			subst = buf->data;
			free(buf);
		}
	}

	return subst;
}

static cp_db_statement *pgsql_prepare_statement(cp_db_connection *conn, 
												int prm_count, 
												cp_field_type *prm_type, 
												char *query)
{
	cp_db_statement *statement = NULL;
	int i;
	PGresult *res;
	Oid *paramTypes = malloc(prm_count * sizeof(Oid));
	char *stmt = malloc(21);
	char *subst;
	ExecStatusType rc;

#ifdef HAVE_STRLCPY
	strlcpy(stmt, "STMT", 21);
#else
	strcpy(stmt, "STMT");
#endif /* HAVE_STRLCPY */
	gen_id_str(&stmt[4]);

	for (i = 0; i < prm_count; i++)
		paramTypes[i] = cprops2pgsql(prm_type[i]);
	
	/* parameters may be given as ?, postgres expects $1, $2 etc */
	subst = format_prm(query, prm_count);
	res = PQprepare(conn->connection, stmt, subst, prm_count, paramTypes);
	free(paramTypes);
	if (subst != query) free(subst);

	if (res == NULL)
	{
		cp_error(CP_DBMS_CLIENT_ERROR, "%s - prepare statement: %s", 
				 conn->data_source->act->dbms_lit, 
				 PQerrorMessage(conn->connection));
		free(stmt);
	}
	else
	{
		rc = PQresultStatus(res);
		switch (rc)
		{
			case PGRES_EMPTY_QUERY:
    		case PGRES_BAD_RESPONSE:
	    	case PGRES_FATAL_ERROR:
				free(stmt);
				stmt = NULL;
				cp_error(CP_DBMS_CLIENT_ERROR, "%s - prepare statement: %s", 
						 conn->data_source->act->dbms_lit, 
						 PQresultErrorMessage(res));
				break;

    		case PGRES_NONFATAL_ERROR:
				cp_warn("%s - prepare statement: %s", 
						conn->data_source->act->dbms_lit, 
						PQresultErrorMessage(res));
			default:
				statement = cp_db_statement_instantiate(conn, prm_count, 
														prm_type, stmt);
				if (statement == NULL)
				{
					//~~ report error
					free(stmt);
				}
				break;
			
		}
		PQclear(res);
	}

	if (statement)
		statement->store = memdup(prm_type, prm_count * sizeof(cp_field_type));

	return statement;
}

#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN

void hton8byte(void *ptr)
{
	int w;
	union
	{
		double d;
		int i[2];
	} *u;

	u = ptr;
	w = ntohl(u->i[0]);
	u->i[0] = ntohl(u->i[1]);
	u->i[1] = w;
}
	
	
static void **to_network_order(cp_field_type *types, void **prm, int count)
{
	int i;
	void **res = calloc(count, sizeof(void *));
	short *pshort;
	int *pint;
	long *plong;
	float *pfloat;
	double *pdouble;
	
	for (i = 0; i < count; i++)
	{
		switch (types[i])
		{
			case CP_FIELD_TYPE_SHORT:
				pshort = malloc(sizeof(short));
				memcpy(pshort, prm[i], sizeof(short));
				*pshort = htons(*pshort);
				res[i] = pshort;
				break;

			case CP_FIELD_TYPE_INT:
				pint = malloc(sizeof(int));
				memcpy(pint, prm[i], sizeof(int));
				*pint = htonl(*pint);
				res[i] = pint;
				break;

			case CP_FIELD_TYPE_LONG:
				if (sizeof(long) == sizeof(int))
				{
					plong = malloc(sizeof(long));
					memcpy(plong, prm[i], sizeof(long));
					*plong = htonl(*plong);
					res[i] = plong;
					break;
				}

			case CP_FIELD_TYPE_FLOAT:
				pfloat = malloc(sizeof(float));
				memcpy(pfloat, prm[i], sizeof(float));
				plong = (long *) pfloat;
				*plong = htonl(*plong);
				res[i] = pfloat;
				break;

			case CP_FIELD_TYPE_DOUBLE:
				pdouble = malloc(sizeof(double));
				memcpy(pdouble, prm[i], sizeof(double));
				hton8byte(pdouble);
				res[i] = pdouble;
				break;

			case CP_FIELD_TYPE_TIME:
			case CP_FIELD_TYPE_DATE:
			case CP_FIELD_TYPE_TIMESTAMP:
				res[i] = prm[i];
				hton8byte(res[i]);
				break;

			default:
				res[i] = prm[i];
		}
	}

	return res;
}

void delete_prm(void **in_order_prm, cp_field_type *types, int count)
{
	int i;

	for (i = 0; i < count; i++)
	{
		switch (types[i])
		{
			case CP_FIELD_TYPE_SHORT:
			case CP_FIELD_TYPE_INT:
			case CP_FIELD_TYPE_FLOAT:
			case CP_FIELD_TYPE_DOUBLE:
				free(in_order_prm[i]);
				break;

			case CP_FIELD_TYPE_LONG:
				if (sizeof(long) == sizeof(int))
				{
					free(in_order_prm[i]);
					break;
				}

			default:
				break;
		}
	}

	free(in_order_prm);
}
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */

static void *to_pgtime(cp_timestampz *tmz)
{
	double *d = calloc(1, sizeof(double));
	*d = tmz->tm.tv_sec + ((double) tmz->tm.tv_usec) / 1e6
	     + tmz->tz_minuteswest * 60 - ((30 * 365 + 7) * 86400);
	return d;
}

static void subst_cp2pgsql(int count, cp_field_type *types, void **prm, int *lengths)
{
	int i;
	short *s;
	char *ch;
	
	for (i = 0; i < count; i++)
	{
		switch (types[i])
		{
			case CP_FIELD_TYPE_BOOLEAN:
				s = prm[i];
				ch = prm[i];
				*ch = (prm[i] == 0 ? 'f' : 't');
				lengths[i] = 1;
				break;

			case CP_FIELD_TYPE_TIME:
			case CP_FIELD_TYPE_DATE:
			case CP_FIELD_TYPE_TIMESTAMP:
				prm[i] = to_pgtime(prm[i]);
				lengths[i] = sizeof(double);
				break;

			default:
				break;
		}
	}
}

static void free_cp2pgsql(int count, cp_field_type *types, void **prm)
{
	int i;
	
	for (i = 0; i < count; i++)
	{
		if (types[i] == CP_FIELD_TYPE_TIME ||
			types[i] == CP_FIELD_TYPE_DATE ||
			types[i] == CP_FIELD_TYPE_TIMESTAMP)
		{
			free(prm[i]);
		}
	}
}

static int pgsql_execute_statement(cp_db_connection *conn, 
								   cp_db_statement *stmt, 
								   cp_result_set **result_set, 
								   int *lengths, 
								   void **prm)
{
	int *fmt = NULL;
	cp_result_set *res = NULL;
	PGresult *pgres = NULL;
	ExecStatusType ret;
	int i;
	int rc = 0;
	cp_field_type *types = NULL;
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
	void **in_order_prm = prm;
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */

	subst_cp2pgsql(stmt->prm_count, stmt->store, prm, lengths);

	if (lengths)
	{
		types = stmt->store;
		fmt = calloc(stmt->prm_count, sizeof(int));
		for (i = 0; i < stmt->prm_count; i++) fmt[i] = 1;
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
		in_order_prm = to_network_order(types, prm, stmt->prm_count);
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
	}

	pgres = PQexecPrepared(conn->connection, (char *) stmt->source,
						   stmt->prm_count,
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
						   (const char * const *) in_order_prm,
#else
						   (const char * const *) prm,
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */						   
						   lengths, fmt, stmt->binary);
	free_cp2pgsql(stmt->prm_count, stmt->store, prm);
	if (fmt) free(fmt);
#ifdef CP_BYTE_ORDER_LITTLE_ENDIAN
	if (in_order_prm != prm) delete_prm(in_order_prm, types, stmt->prm_count);
#endif /* CP_BYTE_ORDER_LITTLE_ENDIAN */
	
	ret = PQresultStatus(pgres);
	switch (ret)
	{
		case PGRES_TUPLES_OK:
			res = create_result_set(conn, pgres);
			res->binary = stmt->binary;
			fetch_rows(res, pgres);
			break;

		case PGRES_COMMAND_OK:
			rc = atoi(PQcmdTuples(pgres));
			PQclear(pgres);
			break;

		case PGRES_EMPTY_QUERY:
   		case PGRES_BAD_RESPONSE:
    	case PGRES_FATAL_ERROR:
   		case PGRES_NONFATAL_ERROR:
		default:
			rc = -1;
			PQclear(pgres);
			cp_error(CP_DBMS_QUERY_FAILED, "%s - prepared statement: %s", 
					 conn->data_source->act->dbms_lit, 
					 PQresultErrorMessage(pgres));
			break;
	}

	if (result_set)
		*result_set = res;

	return rc;
}

static void pgsql_release_statement(cp_db_statement *statement)
{
	if (statement->source) free(statement->source);
	if (statement->store) free(statement->store);
}

static void pgsql_set_autocommit(cp_db_connection *conn, int state)
{
	if (state == 0)
		pgsql_update(conn, "begin;");
}

static int pgsql_commit(cp_db_connection *conn)
{
	int rc = pgsql_update(conn, "commit;");
	if (conn->autocommit == 0)
		pgsql_update(conn, "begin;");
	return rc;
}

static int pgsql_rollback(cp_db_connection *conn)
{
	int rc = pgsql_update(conn, "rollback;");
	if (conn->autocommit == 0)
		pgsql_update(conn, "begin;");
	return rc;
}

#ifdef WINDOWS
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
#endif /* _WINDOWS */
