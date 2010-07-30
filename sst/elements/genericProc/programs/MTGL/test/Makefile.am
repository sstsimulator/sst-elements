# -*- Makefile -*-

# traceview flags (see ../configure.in)
if WITH_TRACEVIEW
  TVFLAGS = -trace -trace_level 0 -profile
else
  TVFLAGS = 
endif

if WITH_DEBUG_ARRAYBOUNDS
 CCFLAG_ARRAYBOUNDS = -DCHECK_BOUNDS
else
 CCFLAG_ARRAYBOUNDS = 
endif

noinst_PROGRAMS = \
	test_bfs \
	test_cc \
	test_pagerank \
	test_pagerank_pim

if !RELEASABLE_CODE_ONLY
noinst_PROGRAMS += \
	test_static_graph1 \
	test_static_graph2 \
	test_static_graph3 \
	rmat2file \
	custom2file \
	test_snapshot \
	test_tri \
	test_indset \
	test_sssp_deltastepping \
	csg_test
#        test_vf2_isomorphism \
#        test_isomorphism_ullman 
#	test_dpmf \
#	cc_test \
#	sssp_test \
#	community_test \
#	assort_test \
#	dd_test \
#	ddc_test \
#	tri_test \
#	clco_test \
#	pd_test \
#	vb_test \
#	csg_test
endif

TESTS = $(noinst_PROGRAMS)
XFAIL_TESTS = $(noinst_PROGRAMS)

test_bfs_SOURCES = mtgl_test.hpp test_bfs.cpp
if ON_MTA
test_bfs_CFLAGS = -pl test_bfs.pl
test_bfs_CXXFLAGS = -pl test_bfs.pl
endif

test_cc_SOURCES = mtgl_test.hpp test_cc.cpp
if ON_MTA
test_cc_CFLAGS = -pl test_cc.pl
test_cc_CXXFLAGS = -pl test_cc.pl
endif

test_pagerank_pim_SOURCES = test_pagerank.cpp mtgl_test.hpp
test_pagerank_pim_CFLAGS = -DPIM_EXTENSIONS=1
test_pagerank_pim_CXXFLAGS = -DPIM_EXTENSIONS=1

test_pagerank_SOURCES = test_pagerank.cpp mtgl_test.hpp
if ON_MTA
test_pagerank_CFLAGS = -pl test_pagerank.pl
test_pagerank_CXXFLAGS = -pl test_pagerank.pl
endif

nodist_test_static_graph1_SOURCES = test_static_graph1.cpp
if ON_MTA
test_static_graph1_CFLAGS = -pl test_static_graph1.pl
test_static_graph1_CXXFLAGS = -pl test_static_graph1.pl
endif

nodist_test_static_graph2_SOURCES = test_static_graph2.cpp
if ON_MTA
test_static_graph2_CFLAGS = -pl test_static_graph2.pl
test_static_graph2_CXXFLAGS = -pl test_static_graph2.pl
endif

nodist_test_static_graph3_SOURCES = test_static_graph3.cpp
if ON_MTA
test_static_graph3_CFLAGS = -pl test_static_graph3.pl
test_static_graph3_CXXFLAGS = -pl test_static_graph3.pl
endif

nodist_rmat2file_SOURCES = rmat2file.cpp
if ON_MTA
rmat2file_CFLAGS = -pl rmat2file.pl
rmat2file_CXXFLAGS = -pl rmat2file.pl
endif

nodist_custom2file_SOURCES = custom2file.cpp
if ON_MTA
custom2file_CFLAGS = -pl custom2file.pl
custom2file_CXXFLAGS = -pl custom2file.pl
endif

nodist_test_sssp_deltastepping_SOURCES = test_sssp_deltastepping.cpp
if ON_MTA
test_sssp_deltastepping_CFLAGS = -pl test_sssp_deltastepping.pl
test_sssp_deltastepping_CXXFLAGS = -pl test_sssp_deltastepping.pl
endif

nodist_test_snapshot_SOURCES = test_snapshot.cpp
if ON_MTA
test_snapshot_CFLAGS = -pl test_snapshot.pl
test_snapshot_CXXFLAGS = -pl test_snapshot.pl
endif

nodist_test_tri_SOURCES =  test_tri.cpp
if ON_MTA
test_tri_CFLAGS = -pl test_tri.pl
test_tri_CXXFLAGS = -pl test_tri.pl
endif

nodist_test_indset_SOURCES = test_indset.cpp 
if ON_MTA
test_indset_CFLAGS = -pl test_indset.pl
test_indset_CXXFLAGS = -pl test_indset.pl
endif

#nodist_test_vf2_isomorphism_SOURCES = test_vf2_isomorphism.cpp
#if ON_MTA
#test_vf2_isomorphism_CFLAGS = -pl test_vf2_isomorphism.pl
#test_vf2_isomorphism_CXXFLAGS = -pl test_vf2_isomorphism.pl
#endif

#nodist_test_isomorphism_ullman_SOURCES = test_isomorphism_ullman.cpp
#if ON_MTA
#test_isomorphism_ullman_CFLAGS = -pl test_isomorphism_ullman.pl
#test_isomorphism_ullman_CXXFLAGS = -pl test_isomorphism_ullman.pl
#endif

# test_dpmf_SOURCES = test_dpmf.cpp 
# cc_test_SOURCES = test_connected_components.cpp
# sssp_test_SOURCES =  test_sssp_deltastepping.cpp
# # mta-pe 6.0.0 gives an internal compiler error if we attempt to
# # build community_test with separate compilation followed by linking.
# # command line building:
# #  c++ -par -I.. -pl comm.pl test_community.cpp ufl.cpp VolVolume.cpp
# # works.
# nodist_community_test_SOURCES = test_community.cpp ufl.cpp VolVolume.cpp
# nodist_assort_test_SOURCES = test_find_assortativity.cpp
# nodist_dd_test_SOURCES = test_find_degree_distribution.cpp
# nodist_ddc_test_SOURCES = test_find_degree_degree_correlation.cpp
# nodist_tri_test_SOURCES = test_find_triangles.cpp
# nodist_clco_test_SOURCES = test_find_cluster_coefficient.cpp
# nodist_pd_test_SOURCES = test_find_pseudo_diameter.cpp
# nodist_vb_test_SOURCES = test_find_vertex_betweenness.cpp
nodist_csg_test_SOURCES = test_connection_subgraphs.cpp
# nodist_st_test_SOURCES            =  test_st_search.cpp
# nodist_scc_test_SOURCES           =  test_strongly_connected_components.cpp
# nodist_test_spanning_tree_SOURCES =  test_spanning_tree.cpp

if ON_MTA
test_spanning_tree_CFLAGS = -pl test_spanning_tree.pl
test_spanning_tree_CXXFLAGS = -pl test_spanning_tree.pl

scc_test_CFLAGS = -pl scc_test.pl
scc_tets_CXXFLAGS = =pl scc_test.pl

csg_test_CFLAGS = -pl csg_test.pl
csg_test_CXXFLAGS = -pl csg_test.pl
endif

if ON_MTA
mostlyclean-local:
	/bin/rm -f *.ti
	/bin/rm -f *.ii
	/bin/rm -f *.pl
endif

