#include <string.h>

#include <cprops/db.h>
#include <cprops/db_mysql.h>
#include <cprops/str.h>
#include <cprops/log.h>

#include <cprops/config.h>

void do_select(cp_db_connection *conn, char *query)
{
	cp_result_set *rs;
	rs = cp_db_connection_select(conn, query);
	if (rs)
	{
		int i;
		cp_vector *r;

		while ((r = cp_result_set_next(rs)) != NULL)
		{
			for (i = 0; i < rs->field_count; i++)
				printf(" | %s", (char *) cp_string_tocstr(cp_vector_element_at(r, i)));
			printf(" |\n");
            cp_result_set_release_row(rs, r);
		}

		cp_result_set_destroy(rs);
	}
}


int main(int argc, char *argv[])
{
	cp_data_source *ds = NULL;
	cp_db_connection *conn = NULL;

	char *host = "localhost";
	char *login = "test";
	char *password = NULL;
	char *db_name = "test";
	int port = 0; /* use default */
	
	cp_log_init("test_mysql.log", LOG_LEVEL_DEBUG);
	cp_db_init();
	
#ifdef CP_DBMS_STATIC
	/* to use cp_mysql_data_source, link with -lcp_dbms_mysql */
	ds = cp_mysql_data_source(host, login, password, db_name, 0, NULL, 0);
#else
    if (cp_dbms_load_driver("mysql"))
    {
        cp_error(CP_DBMS_NO_DRIVER, "can\'t load driver");
        goto DONE;
    }

	ds = cp_dbms_get_data_source("mysql", host, port, login, password, db_name);
#endif

	if (ds == NULL)
	{
		cp_error(CP_DBMS_CONNECTION_FAILURE, "can\'t connect");
		goto DONE;
	}

	conn = cp_data_source_get_connection(ds);
	if (conn == NULL)
	{
		cp_error(CP_DBMS_CONNECTION_FAILURE, "can\'t connect");
		goto DONE;
	}

	if (argc > 1)
	{
		char *query = argv[1];
		if (strstr(query, "select"))
			do_select(conn, query);
		else
			cp_db_connection_update(conn, query);
	}
	else 
		do_select(conn, "select * from translation");

	cp_db_connection_close(conn);

DONE:
	if (conn) 
		cp_db_connection_destroy(conn);

	if (ds)
		cp_data_source_destroy(ds);

	cp_db_shutdown();
	cp_log_close();

	return 0;
}

