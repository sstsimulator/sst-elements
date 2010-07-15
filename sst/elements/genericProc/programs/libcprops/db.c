#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "config.h"
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#include "util.h"
#endif /* HAVE_DLFCN_H */

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

#include "db.h"
/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                           general framework setup                       */
/*                                                                         */
/* ----------------------------------------------------------------------- */

volatile static int initialized = 0;
static cp_list *shutdown_hook = NULL;
static cp_hashtable *drivers = NULL;

int cp_db_init()
{
	if (initialized) return 1;
	initialized = 1;

	shutdown_hook = cp_list_create();
	return 0;
}

int cp_db_register_dbms(char *name, void (*shutdown_call)())
{
	cp_list_append(shutdown_hook, shutdown_call);

	return cp_list_item_count(shutdown_hook);
}

static void invoke_shutdown_call(void *fn)
{
	void (*call)() = fn;
	(*call)();
}

void cp_dbms_driver_descriptor_destroy(void *descriptor)
{
	cp_dbms_driver_descriptor *desc = descriptor;
#if defined(HAVE_DLFCN_H) || defined (_WINDOWS)
	if (desc->lib)
		dlclose(desc->lib);
#endif /* HAVE_DLFCN_H */
	free(desc);
}
	
int cp_db_shutdown()
{
	if (!initialized) return 1;
	initialized = 0;

	cp_list_destroy_custom(shutdown_hook, invoke_shutdown_call);
	if (drivers) cp_hashtable_destroy(drivers);

	return 0;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                          cp_timestampz functions                        */
/*                                                                         */
/* ----------------------------------------------------------------------- */

cp_timestampz *cp_timestampz_create(struct timeval *tm, int minuteswest)
{
	cp_timestampz *tmz = calloc(1, sizeof(cp_timestampz));
	if (tmz == NULL) return NULL;

	tmz->tm.tv_sec = tm->tv_sec;
	tmz->tm.tv_usec = tm->tv_usec;
	tmz->tz_minuteswest = minuteswest;

	return tmz;
}

cp_timestampz *
	cp_timestampz_create_prm(int sec_since_epoch, int microsec, int minwest)
{
	cp_timestampz *tmz = calloc(1, sizeof(cp_timestampz));
	if (tmz == NULL) return NULL;

	tmz->tm.tv_sec = sec_since_epoch;
	tmz->tm.tv_usec = microsec;
	tmz->tz_minuteswest = minwest;

	return tmz;
}

struct tm *cp_timestampz_localtime(cp_timestampz *tm, struct tm *res)
{
#ifdef HAVE_LOCALTIME_R
	return localtime_r((time_t *) &tm->tm.tv_sec, res);
#else
	res = localtime((time_t *) &tm->tm.tv_sec);
	return res;
#endif /* HAVE_LOCALTIME_R */
}

void cp_timestampz_destroy(cp_timestampz *tm)
{
	free(tm);
}


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                      connection parameter functions                     */
/*                                                                         */
/* ----------------------------------------------------------------------- */

void cp_db_connection_parameters_destroy(cp_db_connection_parameters *prm)
{
	if (prm->impl_dtr)
		(*prm->impl_dtr)(prm->impl);
	else
		free(prm->impl);

	free(prm);
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                           connection functions                          */
/*                                                                         */
/* ----------------------------------------------------------------------- */

cp_result_set *
	cp_db_connection_select(cp_db_connection *connection, char *query)
{
	return (*connection->data_source->act->select)(connection, query);
}

int cp_db_connection_update(cp_db_connection *connection, char *query)
{
	return (*connection->data_source->act->update)(connection, query);
}

int cp_db_connection_close(cp_db_connection *connection)
{
	return (*connection->data_source->act->close)(connection);
}

void cp_db_connection_destroy(cp_db_connection *connection)
{
	free(connection);
}

void 
	cp_db_connection_set_fetch_metadata(cp_db_connection *connection, int mode)
{
	connection->autofetch_query_metadata = mode;
}

void cp_db_connection_set_read_result_set_at_once(cp_db_connection *connection, 
												  int mode)
{
	if (mode == 0 && connection->data_source->act->fetch_next == NULL)
	{
		cp_error(CP_METHOD_NOT_IMPLEMENTED, 
				 "%s: can\'t unset read at once mode - method fetch_next not "
				 "implemented", connection->data_source->act->dbms_lit);
		return;
	}
	connection->read_result_set_at_once = mode;
}

void cp_db_connection_set_fetch_size(cp_db_connection *connection, int fetch_size)
{
	if (connection->data_source->act->fetch_next == NULL)
	{
		cp_error(CP_METHOD_NOT_IMPLEMENTED, 
				 "%s: can\'t set fetch size - method fetch_next not implemented",
				 connection->data_source->act->dbms_lit);
		return;
	}
	connection->fetch_size = fetch_size;
}

char *cp_db_connection_escape_string(cp_db_connection *connection, 
		                             char *src, 
								     int len)
{
	if (connection->data_source->act->escape_string)
		return (*connection->data_source->act->escape_string)(connection, src, len);

	cp_error(CP_METHOD_NOT_IMPLEMENTED, 
			"%s driver does not implement escape_string", 
			connection->data_source->act->dbms_lit);

	return strdup(src);
}

char *cp_db_connection_escape_binary(cp_db_connection *connection, 
		                             char *src, 
								     int src_len, 
								     int *res_len)
{
	char *res;
	if (connection->data_source->act->escape_binary)
		return (*connection->data_source->act->escape_binary)(connection, src, 
				                                              src_len, res_len);

	cp_error(CP_METHOD_NOT_IMPLEMENTED, 
			"%s driver does not implement escape_binary", 
			connection->data_source->act->dbms_lit);

	*res_len = src_len;
	res = malloc(src_len);
	memcpy(res, src, src_len);
	return res;
}

cp_string *cp_db_connection_unescape_binary(cp_db_connection *connection,
											char *src)
{
	if (connection->data_source->act->unescape_binary)
		return (*connection->data_source->act->unescape_binary)(connection, src);

	cp_error(CP_METHOD_NOT_IMPLEMENTED, 
			"%s driver does not implement unescape_binary", 
			connection->data_source->act->dbms_lit);


	return cp_string_create(src, strlen(src));
}

cp_db_statement *
	cp_db_connection_prepare_statement(cp_db_connection *connection,
									   int prm_count,
									   cp_field_type *prm_types,
									   char *query)
{
	if (connection->data_source->act->prepare_statement)
		return (*connection->data_source->act->prepare_statement)(connection, 
				                                                  prm_count,
																  prm_types, 
																  query);

	cp_error(CP_METHOD_NOT_IMPLEMENTED, 
			"%s driver does not implement prepare_statement", 
			connection->data_source->act->dbms_lit);

	return NULL;
}

/** execute a prepared statement */
int cp_db_connection_execute_statement(cp_db_connection *connection,
		                               cp_db_statement *statement, 
									   cp_vector *prm,
									   cp_result_set **results)
{
	if (connection->data_source->act->execute_statement)
	{
		int rc;
		int *lengths = NULL;
		void **sprm = NULL;
		char *cstr;
		if (statement->prm_count)
		{
			int i;
//			cp_field_type *type;
			cp_string *str;
			lengths = calloc(statement->prm_count, sizeof(int));
			sprm = calloc(statement->prm_count, sizeof(void *));
			for (i = 0; i < statement->prm_count; i++)
			{
				sprm[i] = cp_vector_element_at(prm, i);
				switch (statement->types[i])
				{
					case CP_FIELD_TYPE_BOOLEAN: 
					case CP_FIELD_TYPE_SHORT:     
						lengths[i] = sizeof(short); break;
					case CP_FIELD_TYPE_INT:       
						lengths[i] = sizeof(int); break;
					case CP_FIELD_TYPE_LONG:      
						lengths[i] = sizeof(long); break;
					case CP_FIELD_TYPE_LONG_LONG: 
#ifdef HAVE_LONG_LONG_INT
						lengths[i] = sizeof(long long int); break;
#else
						lengths[i] = sizeof(__int64); break;
#endif /* HAVE_LONG_LONG_INT */
					case CP_FIELD_TYPE_FLOAT:     
						lengths[i] = sizeof(float); break;
					case CP_FIELD_TYPE_DOUBLE:    
						lengths[i] = sizeof(double); break;
					case CP_FIELD_TYPE_CHAR:
					case CP_FIELD_TYPE_VARCHAR:
						cstr = cp_vector_element_at(prm, i);
						lengths[i] = strlen(cstr);
						break;
					case CP_FIELD_TYPE_BLOB:      
						str = cp_vector_element_at(prm, i);
						lengths[i] = str->len;
						sprm[i] = str->data;
						break;
					case CP_FIELD_TYPE_DATE:      
					case CP_FIELD_TYPE_TIME:      
					case CP_FIELD_TYPE_TIMESTAMP:
					default:
						;
				}
			}
		}
		rc =  (*connection->data_source->act->execute_statement)(connection, 
																 statement, 
																 results, 
																 lengths, 
																 sprm);
		if (statement->prm_count)
		{
			free(lengths);
			free(sprm);
		}

		return rc;
	}

	cp_error(CP_METHOD_NOT_IMPLEMENTED, 
			"%s driver does not implement execute_statement", 
			connection->data_source->act->dbms_lit);

	return -1;
}

int cp_db_connection_execute_statement_args(cp_db_connection *connection,
											cp_db_statement *statement,
											cp_result_set **results, ...)
{
	if (connection->data_source->act->execute_statement)
	{
		int i;
		int rc;
		va_list argp;
		int p_int;
		long p_long;
#ifdef HAVE_LONG_LONG_INT
		long long int p_longlong;
#else
		__int64 p_longlong;
#endif /* HAVE_LONG_LONG_INT */
		double p_double;
		cp_timestampz p_timestamp;
		cp_string p_str;
		cp_vector *tmp = NULL;
		void *ptr;
		int *lengths = NULL;
		
		void **prm = NULL;
		
		if (statement->prm_count)
		{
			prm = calloc(statement->prm_count, sizeof(void *));
			lengths = calloc(statement->prm_count, sizeof(int));
		}

		va_start(argp, results);
		for (i = 0; i < statement->prm_count; i++)
		{
			tmp = cp_vector_create(statement->prm_count);
			switch (statement->types[i])
			{
				case CP_FIELD_TYPE_CHAR:
				case CP_FIELD_TYPE_VARCHAR:
					prm[i] = va_arg(argp, char *);
					lengths[i] = strlen(prm[i]);
					break;

				case CP_FIELD_TYPE_BLOB:
					ptr = malloc(sizeof(int));
					p_str = va_arg(argp, cp_string);
					memcpy(ptr, &p_str, sizeof(cp_string));
					prm[i] = ptr;
					lengths[i] = p_str.len;
					break;
					
				case CP_FIELD_TYPE_BOOLEAN:
				case CP_FIELD_TYPE_SHORT:
				case CP_FIELD_TYPE_INT:
					ptr = malloc(sizeof(int));
					p_int = va_arg(argp, int);
					memcpy(ptr, &p_int, sizeof(int));
					cp_vector_add_element(tmp, ptr);
					prm[i] = ptr;
					lengths[i] = sizeof(int);
					break;

				case CP_FIELD_TYPE_LONG:
					ptr = malloc(sizeof(long));
					p_long = va_arg(argp, long);
					memcpy(ptr, &p_long, sizeof(long));
					cp_vector_add_element(tmp, ptr);
					prm[i] = ptr;
					lengths[i] = sizeof(long);
					break;

				case CP_FIELD_TYPE_LONG_LONG:
#ifdef HAVE_LONG_LONG_INT
					ptr = malloc(sizeof(long long int));
					p_longlong = va_arg(argp, long long int);
					memcpy(ptr, &p_longlong, sizeof(long long int));
					lengths[i] = sizeof(long long int);
#else
					ptr = malloc(sizeof(__int64));
					p_longlong = va_arg(argp, __int64);
					memcpy(ptr, &p_longlong, sizeof(__int64));
					lengths[i] = sizeof(__int64);
#endif /* HAVE_LONG_LONG_INT */
					cp_vector_add_element(tmp, ptr);
					prm[i] = ptr;
					break;

				case CP_FIELD_TYPE_FLOAT:
				case CP_FIELD_TYPE_DOUBLE:
					ptr = malloc(sizeof(double));
					p_double = va_arg(argp, double);
					memcpy(ptr, &p_double, sizeof(double));
					cp_vector_add_element(tmp, ptr);
					prm[i] = ptr;
					lengths[i] = sizeof(double);
					break;

				case CP_FIELD_TYPE_DATE:
				case CP_FIELD_TYPE_TIME:
				case CP_FIELD_TYPE_TIMESTAMP:
					ptr = malloc(sizeof(cp_timestampz));
					p_timestamp = va_arg(argp, cp_timestampz);
					memcpy(ptr, &p_timestamp, sizeof(cp_timestampz));
					cp_vector_add_element(tmp, ptr);
					prm[i] = ptr;
					break;
			}
		}

		rc = (*connection->data_source->act->execute_statement)(connection, 
																statement, 
																results, 
																lengths, 
																prm);
		va_end(argp);
		if (statement->prm_count)
		{
			free(prm);
			free(lengths);
		}
		if (prm) free(prm);
		if (tmp)
			cp_vector_destroy_custom(tmp, (cp_destructor_fn) free);
		return rc;
	}

	cp_error(CP_METHOD_NOT_IMPLEMENTED, 
			"%s driver does not implement execute_statement", 
			connection->data_source->act->dbms_lit);

	return -1;
}
		
void cp_db_connection_close_statement(cp_db_connection *connection, 
									  cp_db_statement *statement)
{
	if (connection->data_source->act->release_statement)
		(*connection->data_source->act->release_statement)(statement);
	else
	{
		cp_error(CP_METHOD_NOT_IMPLEMENTED,
				"%s driver does not implement release_statement",
				connection->data_source->act->dbms_lit);
	}

	cp_db_statement_destroy(statement);
}

void cp_db_connection_set_autocommit(cp_db_connection *connection, int state)
{
	connection->autocommit = state;
	if (connection->data_source->act->set_autocommit)
		(*connection->data_source->act->set_autocommit)(connection, state);
	else
	{
		cp_error(CP_METHOD_NOT_IMPLEMENTED,
				"%s driver does not implement set_autocommit",
				connection->data_source->act->dbms_lit);
	}
}

int cp_db_connection_commit(cp_db_connection *connection)
{
	if (connection->data_source->act->commit)
		return (*connection->data_source->act->commit)(connection);

	cp_error(CP_METHOD_NOT_IMPLEMENTED,
			"%s driver does not implement commit",
			connection->data_source->act->dbms_lit);

	return -1;
}

int cp_db_connection_rollback(cp_db_connection *connection)
{
	if (connection->data_source->act->rollback)
		return (*connection->data_source->act->rollback)(connection);

	cp_error(CP_METHOD_NOT_IMPLEMENTED,
			"%s driver does not implement rollback",
			connection->data_source->act->dbms_lit);

	return -1;
}


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                       prepared statement functions                      */
/*                                                                         */
/* ----------------------------------------------------------------------- */

cp_db_statement *cp_db_statement_instantiate(cp_db_connection *connection,
										     int prm_count, 
		                                     cp_field_type *types, 
										     void *source)
{
	cp_db_statement *stmt = calloc(1, sizeof(cp_db_statement));
	if (stmt == NULL) goto DONE;

	stmt->connection = connection;
	stmt->prm_count = prm_count;
	stmt->types = malloc(prm_count * sizeof(cp_field_type));
	if (stmt->types == NULL) goto DONE;
	memcpy(stmt->types, types, prm_count * sizeof(cp_field_type));
	stmt->source = source;

DONE:
	if (stmt && stmt->types == NULL)
	{
		free(stmt);
		stmt = NULL;
	}

	return stmt;
}

void cp_db_statement_set_binary(cp_db_statement *stmt, int binary)
{
	stmt->binary = binary;
}

void cp_db_statement_destroy(cp_db_statement *statement)
{
	if (statement)
	{
		if (statement->types) free(statement->types);
		free(statement);
	}
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                           result set functions                          */
/*                                                                         */
/* ----------------------------------------------------------------------- */

cp_vector *cp_result_set_next(cp_result_set *result_set)
{
	cp_vector *row = cp_list_remove_head(result_set->results);
	if (row == NULL && !result_set->fetch_complete)
	{
		if (result_set->connection)
		{
			if (result_set->connection->data_source->act->fetch_next)
				(*result_set->connection->data_source->act->fetch_next)(result_set);
			else
				cp_error(CP_METHOD_NOT_IMPLEMENTED, "%s: fetch_next",
						result_set->connection->data_source->act->dbms_lit);
			
			row = cp_list_remove_head(result_set->results);
		}
	}
		
	if (row) 
	{
		result_set->position++;
		if (result_set->dispose)
			cp_list_append(result_set->dispose_list, row);
	}
	else //~~
		result_set->fetch_complete = 1;

	return row;
}

void cp_result_set_destroy(cp_result_set *result_set)
{
	cp_vector *row;

	while ((row = cp_list_remove_head(result_set->results)))
		cp_result_set_release_row(result_set, row);

	if (result_set->dispose)
		while ((row = cp_list_remove_head(result_set->dispose_list)))
			cp_result_set_release_row(result_set, row);

	if (result_set->dispose_list)
		cp_list_destroy(result_set->dispose_list);

	if (result_set->fetch_complete == 0 &&
	    result_set->connection->data_source->act->release_result_set)	
		(*result_set->connection->data_source->act->release_result_set)(result_set);

	cp_list_destroy(result_set->results);

	if (result_set->field_headers)
		cp_vector_destroy(result_set->field_headers);

	if (result_set->field_types)
		cp_vector_destroy(result_set->field_types);

	free(result_set);
}

cp_vector *cp_result_set_get_headers(cp_result_set *result_set)
{
	if (result_set->field_headers == NULL && 
		result_set->connection != NULL)
	{
		if (result_set->connection->data_source->act->fetch_metadata != NULL)
			(*result_set->connection->data_source->act->fetch_metadata)(result_set);
		else
			cp_error(CP_METHOD_NOT_IMPLEMENTED, "%s: fetch_metadata", 
					result_set->connection->data_source->act->dbms_lit);
	}
	
	return result_set->field_headers;
}

cp_vector *cp_result_set_get_field_types(cp_result_set *result_set)
{
	if (result_set->field_types == NULL && 
		result_set->connection != NULL)
	{
		if (result_set->connection->data_source->act->fetch_metadata != NULL)
			(*result_set->connection->data_source->act->fetch_metadata)(result_set);
		else
			cp_error(CP_METHOD_NOT_IMPLEMENTED, "%s: fetch_metadata", 
					result_set->connection->data_source->act->dbms_lit);
	}
	
	return result_set->field_types;
}

int cp_result_set_field_count(cp_result_set *result_set)
{
	return result_set->field_count;
}

int cp_result_set_row_count(cp_result_set *result_set)
{
	return result_set->row_count;
}

char *cp_result_set_get_header(cp_result_set *rs, int column)
{
	cp_vector *field_headers;
	field_headers = cp_result_set_get_headers(rs);
	if (rs->field_headers == NULL) return NULL;

	return cp_vector_element_at(field_headers, column);
}

cp_field_type cp_result_set_get_field_type(cp_result_set *rs, int column)
{
	cp_vector *field_types; 
	cp_field_type *type;
	field_types = cp_result_set_get_field_types(rs);
	if (rs->field_types == NULL) return -1;

	type = cp_vector_element_at(rs->field_types, column);
	if (type == NULL) return -1;

	return *type;
}

int cp_result_set_is_binary(cp_result_set *result_set)
{
	return result_set->binary;
}

void cp_result_set_autodispose(cp_result_set *rs, int mode)
{
	rs->dispose = mode;
	if (mode && rs->dispose_list == NULL)
		rs->dispose_list =  cp_list_create();
}

void cp_result_set_release_row(cp_result_set *result_set, cp_vector *row)
{
	if (result_set->binary)
	{
		int i;
		int n = cp_vector_size(row);
		cp_field_type *type;
		void *item;
		
		for (i = 0; i < n; i++)
		{
			if ((item = cp_vector_element_at(row, i)) == NULL) continue;
			type = cp_vector_element_at(result_set->field_types, i);
			if (*type == CP_FIELD_TYPE_VARCHAR ||
				*type == CP_FIELD_TYPE_CHAR ||
				*type == CP_FIELD_TYPE_BLOB)
			{
				cp_string_destroy(item);
				cp_vector_set_element(row, i, NULL);
			}
			else
				free(item);
		}

		cp_vector_destroy(row);
	}
	else
		cp_vector_destroy_custom(row, (cp_destructor_fn) cp_string_destroy);
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                            db_actions functions                         */
/*                                                                         */
/* ----------------------------------------------------------------------- */

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
						 cp_db_rollback_fn rollback)
{
	cp_db_actions *actions = calloc(1, sizeof(cp_db_actions));
	if (actions == NULL) return NULL;

	actions->dbms = dbms;
	actions->dbms_lit = strdup(dbms_lit);
	if (actions->dbms_lit == NULL)
	{
		free(actions);
		return NULL;
	}
	
	actions->open = open;
	actions->select = select;
	actions->fetch_metadata = fetch_metadata;
	actions->fetch_next = fetch_next;
	actions->release_result_set = release_result_set;
	actions->update = update;
	actions->close = close;
	actions->escape_string = escape_string;
	actions->escape_binary = escape_binary;
	actions->unescape_binary = unescape_binary;
	actions->prepare_statement = prepare_statement;
	actions->execute_statement = execute_statement;
	actions->release_statement = release_statement;
	actions->set_autocommit = set_autocommit;
	actions->commit = commit;
	actions->rollback = rollback;

	return actions;
}

void cp_db_actions_destroy(cp_db_actions *actions)
{
	if (actions)
	{
		if (actions->dbms_lit) free(actions->dbms_lit);
		free(actions);
	}
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                           data source functions                         */
/*                                                                         */
/* ----------------------------------------------------------------------- */

#if defined(HAVE_DLFCN_H) || defined(_WINDOWS)
#ifndef SO_EXT
#if defined(unix) || defined(__unix__) || defined(__MACH__)
#define SO_EXT "so"
#endif /* unix */
#ifdef _WINDOWS
#define SO_EXT "dll"
#endif /* _WINDOWS */
#endif /* SO_EXT */

int cp_dbms_load_driver(char *name)
{
	cp_dbms_driver_descriptor *desc;
	
	if (drivers == NULL)
		drivers = 
			cp_hashtable_create_by_option(COLLECTION_MODE_DEEP | 
										  COLLECTION_MODE_COPY,
										  1, 
										  cp_hash_string, 
										  cp_hash_compare_string,
										  (cp_copy_fn) strdup, 
										  free, NULL, 
										  cp_dbms_driver_descriptor_destroy);
	
	desc = cp_hashtable_get(drivers, name);
	if (desc == NULL)
	{
		char buf[0x100];

		void *lib;
		void *ds_fn;
		void (*init_fn)();

#ifdef HAVE_SNPRINTF
		snprintf(buf, 0x100, "libcp_dbms_%s.%s", name, SO_EXT);
#else
		sprintf(buf, "libcp_dbms_%s.%s", name, SO_EXT);
#endif /* HAVE_SNPRINTF */
		lib = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
		if (lib == NULL) 
		{
			cp_error(CP_LOADLIB_FAILED, "can\'t load %s dbms driver from %s",
					 name, buf);
			return -1;
		}
		
#ifdef HAVE_SNPRINTF
		snprintf(buf, 0x100, "cp_dbms_%s_get_data_source", name);
#else
		sprintf(buf, "cp_dbms_%s_get_data_source", name);
#endif /* HAVE_SNPRINTF */
		ds_fn = dlsym(lib, buf);
		if (ds_fn == NULL)
		{
			cp_error(CP_LOADFN_FAILED, 
					 "can\'t find data source factory function %s", buf);
			dlclose(lib);
			return -1;
		}
		
		desc = calloc(1, sizeof(cp_dbms_driver_descriptor));
		if (desc == NULL)
		{
			cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate "
					 "descriptor function for %s", name);
			dlclose(lib);
			return -1;
		}
		desc->lib = lib;
		desc->get_data_source = ds_fn;
		
#ifdef HAVE_SNPRINTF
		snprintf(buf, 0x100, "cp_dbms_%s_get_data_source_prm", name);
#else
		sprintf(buf, "cp_dbms_%s_get_data_source_prm", name);
#endif /* HAVE_SNPRINTF */
		desc->get_data_source_prm = dlsym(lib, buf);
		
#ifdef HAVE_SNPRINTF
		snprintf(buf, 0x100, "cp_dbms_%s_init", name);
#else
		sprintf(buf, "cp_dbms_%s_init", name);
#endif /* HAVE_SNPRINTF */
		init_fn = dlsym(lib, buf);
		if (init_fn) (*init_fn)();

		cp_hashtable_put(drivers, name, desc);
	}

	/* return 0 on success, non-zero otherwise */
	return desc == NULL; 
}
#endif /* HAVE_DLFCN_H */

static cp_dbms_driver_descriptor *fetch_driver(char *driver)
{
	cp_dbms_driver_descriptor *desc;
	
	if (drivers == NULL)
	{
		cp_error(CP_DBMS_NO_DRIVER, "no drivers loaded");
		return NULL;
	}

	desc = cp_hashtable_get(drivers, driver);

#if defined(HAVE_DLFCN_H) || defined(_WINDOWS)
	if (desc == NULL)
	{
		if (cp_dbms_load_driver(driver) == 0)
			desc = cp_hashtable_get(drivers, driver);
	}
#endif /* HAVE_DLFCN_H */

	return desc;
}

cp_data_source *
	cp_dbms_get_data_source(char *driver, char *host, int port, char *user,
			char *passwd, char *dbname)
{
	cp_dbms_driver_descriptor *desc = fetch_driver(driver);
	
	if (desc == NULL)
	{
		cp_error(CP_DBMS_NO_DRIVER, "can\'t find driver for %s", driver);
		return NULL;
	}
	
	return (desc->get_data_source)(host, port, user, passwd, dbname);
}

cp_data_source *
	cp_dbms_get_data_source_prm(char *driver, char *host, int port, 
			char *user, char *passwd, char *dbname, cp_hashtable *prm)
{
	cp_dbms_driver_descriptor *desc = fetch_driver(driver);
	
	if (desc == NULL)
	{
		cp_error(CP_DBMS_NO_DRIVER, "can\'t find driver for %s", driver);
		return NULL;
	}

	if (desc->get_data_source_prm == NULL)
	{
		cp_error(CP_METHOD_NOT_IMPLEMENTED, "%s driver does not implement "
				 "get_data_source_prm()", driver);
		return NULL;
	}
	
	return (desc->get_data_source_prm)(host, port, user, passwd, dbname, prm);
}

void cp_data_source_destroy(cp_data_source *data_source)
{
	if (data_source)
	{
		if (data_source->prm) 
			cp_db_connection_parameters_destroy(data_source->prm);	
		//~~ refcount?
//		if (data_source->act)
//			cp_db_actions_destroy(data_source->act);
		free(data_source);
	}
}

cp_db_connection *
	cp_data_source_get_connection(cp_data_source *data_source)
{
	return (*data_source->act->open)(data_source);
}


/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                         connection pool functions                       */
/*                                                                         */
/* ----------------------------------------------------------------------- */

cp_db_connection_pool *
	cp_db_connection_pool_create(cp_data_source *data_source, 
								 int min_size, 
								 int max_size, 
								 int initial_size)
{
	int rc;
	cp_db_connection_pool *pool = calloc(1, sizeof(cp_db_connection_pool));
	if (pool == NULL) return NULL;

	pool->min_size = min_size;
	pool->max_size = max_size;
	
	pool->data_source = data_source;
	
	pool->lock = malloc(sizeof(cp_mutex));
	if (pool->lock == NULL)
	{
		cp_db_connection_pool_destroy(pool);
		return NULL;
	}

	if ((rc = cp_mutex_init(pool->lock, NULL)))
	{
		free(pool->lock);
		pool->lock = NULL;
		cp_db_connection_pool_destroy(pool);
		return NULL;
	}

	pool->cond = malloc(sizeof(cp_cond));
	if (pool->cond == NULL)
	{
		cp_db_connection_pool_destroy(pool);
		return NULL;
	}
	cp_cond_init(pool->cond, NULL);

	pool->free_list = 
		cp_list_create_list(COLLECTION_MODE_NOSYNC | 
							COLLECTION_MODE_MULTIPLE_VALUES, 
							NULL, NULL, NULL);
	if (pool->free_list == NULL)
	{
		cp_db_connection_pool_destroy(pool);
		return NULL;
	}
	
	pool->running = 1;

	for (pool->size = 0; pool->size < initial_size; pool->size++)
	{
		cp_db_connection *conn = 
			(*pool->data_source->act->open)(pool->data_source);
		if (conn == NULL)
		{
			if (pool->size > 0) //~~ better than nothing
			{
				cp_error(CP_DBMS_CONNECTION_FAILURE, 
						"creating connection pool: managed %d connections", 
						pool->size);
				break;
			}
			else
			{
				cp_error(CP_DBMS_CONNECTION_FAILURE, 
						"can\'t open connection pool");
				cp_db_connection_pool_destroy(pool);
				pool = NULL;
				break;
			}
		}
		cp_list_append(pool->free_list, conn);
	}

	return pool;
}

int cp_db_connection_pool_shutdown(cp_db_connection_pool *pool)
{
	cp_db_connection *conn;

	/* signal shutdown */
	cp_mutex_lock(pool->lock);
	pool->running = 0;
	cp_cond_broadcast(pool->cond);
	cp_mutex_unlock(pool->lock);

	while (pool->size > 0)
	{
		cp_mutex_lock(pool->lock);
		while ((conn = cp_list_remove_head(pool->free_list)) != NULL)
		{
			(*pool->data_source->act->close)(conn);
			pool->size--;
		}
		cp_mutex_unlock(pool->lock);

		if (pool->size > 0) //~~ set some kind of limit - eg wait up to 30 sec
		{
#ifdef __TRACE__
			DEBUGMSG("waiting on %d db connections", pool->size);
#endif
			cp_mutex_lock(pool->lock);
			while (cp_list_item_count(pool->free_list) == 0)
				cp_cond_wait(pool->cond, pool->lock);
			cp_mutex_unlock(pool->lock);
		}
	}

	return 0;
}

void cp_db_connection_pool_destroy(cp_db_connection_pool *pool)
{
	if (pool)
	{
		if (pool->free_list) cp_list_destroy(pool->free_list);
		if (pool->lock) 
		{
			cp_mutex_destroy(pool->lock);
			free(pool->lock);
		}
		if (pool->cond)
		{
			cp_cond_destroy(pool->cond);
			free(pool->cond);
		}
		free(pool);
	}
}

void cp_db_connection_pool_set_blocking(cp_db_connection_pool *pool, int block)
{
	pool->block = block;
}

cp_db_connection *
	cp_db_connection_pool_get_connection(cp_db_connection_pool *pool)
{
	cp_db_connection *conn;

	cp_mutex_lock(pool->lock);
	conn = cp_list_remove_head(pool->free_list);
	if (conn == NULL)
	{
		if (pool->size < pool->max_size)
			conn = (*pool->data_source->act->open)(pool->data_source);
		else if (pool->block)
		{
			do
			{
				cp_cond_wait(pool->cond, pool->lock);
			} while (pool->running && ((conn = cp_list_remove_head(pool->free_list)) == NULL));
		}
	}
	cp_mutex_unlock(pool->lock);

	return conn;
}

void cp_db_connection_pool_release_connection(cp_db_connection_pool *pool, 
											  cp_db_connection *connection)
{
	cp_mutex_lock(pool->lock);
	cp_list_insert(pool->free_list, connection);
	cp_cond_signal(pool->cond);
	cp_mutex_unlock(pool->lock);
}

