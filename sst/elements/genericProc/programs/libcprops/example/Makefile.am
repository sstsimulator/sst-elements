TEST_PROGS = test_avl test_avl_multi test_client test_hashlist \
		test_hashlist_iterator test_hashtable test_hashtable2 \
		test_httpasync test_httpclient test_list \
		test_list_iterator test_priority_list test_proxy \
		test_str test_trie test_trie2 test_vector \
		test_rb test_splay wf test_heap  \
		test_mempool test_sorted_hash test_narytree \
		test_multimap test_reindex test_reindex_unique
#TEST_PROGS += test_btree
#noinst_PROGRAMS = $(TEST_PROGS)

EXTRA_DIST = Makefile.vc

# uncomment these two lines to treat the examples as unit tests
TESTS = $(TEST_PROGS)
check_PROGRAMS = $(TEST_PROGS)

CPROPSLIB = $(top_builddir)/libcprops.la
INCLUDES = -I$(top_srcdir)

test_avl_SOURCES = test_avl.c
test_avl_LDADD = $(CPROPSLIB)

test_avl_multi_SOURCES = test_avl_multi.c
test_avl_multi_LDADD = $(CPROPSLIB)

test_client_SOURCES = test_client.c
test_client_LDADD = $(CPROPSLIB)

test_hashlist_SOURCES = test_hashlist.c
test_hashlist_LDADD = $(CPROPSLIB)

test_hashlist_iterator_SOURCES = test_hashlist_iterator.c
test_hashlist_iterator_LDADD = $(CPROPSLIB)

test_hashtable_SOURCES = test_hashtable.c
test_hashtable_LDADD = $(CPROPSLIB)

test_hashtable2_SOURCES = test_hashtable2.c
test_hashtable2_LDADD = $(CPROPSLIB)

test_httpasync_SOURCES = test_httpasync.c
test_httpasync_LDADD = $(CPROPSLIB)

test_httpclient_SOURCES = test_httpclient.c
test_httpclient_LDADD = $(CPROPSLIB)

test_list_SOURCES = test_list.c
test_list_LDADD = $(CPROPSLIB)

test_list_iterator_SOURCES = test_list_iterator.c
test_list_iterator_LDADD = $(CPROPSLIB)

test_priority_list_SOURCES = test_priority_list.c
test_priority_list_LDADD = $(CPROPSLIB)

test_proxy_SOURCES = test_proxy.c
test_proxy_LDADD = $(CPROPSLIB)

test_str_SOURCES = test_str.c
test_str_LDADD = $(CPROPSLIB)

test_trie_SOURCES = test_trie.c
test_trie_LDADD = $(CPROPSLIB)

test_trie2_SOURCES = test_trie2.c
test_trie2_LDADD = $(CPROPSLIB)

test_vector_SOURCES = test_vector.c
test_vector_LDADD = $(CPROPSLIB)

test_rb_SOURCES = test_rb.c
test_rb_LDADD = $(CPROPSLIB)

test_splay_SOURCES = test_splay.c
test_splay_LDADD = $(CPROPSLIB)

wf_SOURCES = wf.c
wf_LDADD = $(CPROPSLIB)

test_heap_SOURCES = test_heap.c
test_heap_LDADD = $(CPROPSLIB)

test_mempool_SOURCES = test_mempool.c
test_mempool_LDADD = $(CPROPSLIB)

test_sorted_hash_SOURCES = test_sorted_hash.c
test_sorted_hash_LDADD = $(CPROPSLIB)

test_narytree_SOURCES = test_narytree.c
test_narytree_LDADD = $(CPROPSLIB)

test_multimap_SOURCES = test_multimap.c
test_multimap_LDADD = $(CPROPSLIB)

test_reindex_SOURCES = test_reindex.c
test_reindex_LDADD = $(CPROPSLIB)

test_reindex_unique_SOURCES = test_reindex_unique.c
test_reindex_unique_LDADD = $(CPROPSLIB)

#test_btree_SOURCES = test_btree.c
#test_btree_LDADD = $(CPROPSLIB)

test_pq_SOURCES = test_pq.c
test_pq_LDADD = $(CPROPSLIB) @POSTGRESQL_LDFLAGS@
test_pq_CFLAGS = @POSTGRESQL_CFLAGS@

test_mysql_SOURCES = test_mysql.c
test_mysql_LDADD = $(CPROPSLIB) @MYSQL_LDFLAGS@
test_mysql_CFLAGS = @MYSQL_CFLAGS@

test_ssl_client_SOURCES = test_ssl_client.c
test_ssl_client_LDADD = $(CPROPSLIB)

if COMPILE_CP_DBMS_POSTGRES_STATIC
TEST_PROGS += test_pq
endif
if COMPILE_CP_DBMS_MYSQL_STATIC
TEST_PROGS += test_mysql
endif
if COMPILE_WITH_SSL
TEST_PROGS += test_ssl_client
endif
