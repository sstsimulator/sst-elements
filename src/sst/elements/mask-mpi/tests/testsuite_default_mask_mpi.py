# -*- coding: utf-8 -*-
import os
import subprocess
import unittest

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_mask_mpi(SSTTestCase):

    def setUp(self):
        super(testcase_mask_mpi, self).setUp()
        global module_init
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(testcase_mask_mpi, self).tearDown()

#####

    def test_sendrecv(self):
        self.mask_mpi_template("test_sendrecv")

    def test_reduce(self):
        self.mask_mpi_template("test_reduce")

    def test_allreduce(self):
        self.mask_mpi_template("test_allreduce")

    def test_ring_allreduce(self):
        self.mask_mpi_template("test_ring_allreduce")

    def test_hierarchical_allreduce(self):
        self.mask_mpi_template(
            "test_hierarchical_allreduce",
            model_args="--hierarchy hierarchical --intra iris.recdouble "
                       "--inter iris.ring --bcast iris.btree")

    def test_hierarchical_allreduce_auto_smp(self):
        self.mask_mpi_template(
            "test_hierarchical_allreduce_auto_smp",
            sdl="test_hierarchical_allreduce",
            ref="test_hierarchical_allreduce_auto_smp")

    def test_hierarchical_allreduce_flat_smp(self):
        self.mask_mpi_template(
            "test_hierarchical_allreduce_flat_smp",
            sdl="test_hierarchical_allreduce",
            ref="test_hierarchical_allreduce_flat_smp",
            model_args="--hierarchy flat --allreduce iris.ring")

    def test_hierarchical_allreduce_one_node_default(self):
        self.mask_mpi_template(
            "test_hierarchical_allreduce_one_node_default",
            sdl="test_hierarchical_allreduce",
            ref="test_hierarchical_allreduce_one_node",
            model_args="--nodes 1 --hierarchy hierarchical")

    def test_hierarchical_allreduce_no_smp_warning(self):
        self.mask_mpi_template(
            "test_hierarchical_allreduce_no_smp_warning",
            sdl="test_hierarchical_allreduce",
            ref="test_hierarchical_allreduce_no_smp",
            model_args="--nodes 8 --cores 1 --hierarchy hierarchical",
            expected_stderr="no SMP communicator for hierarchical "
                            "all-reduce; using flat")

    @unittest.skipIf(testing_check_get_num_ranks() > 1,
                     "sparse communicators are unsupported with multi-rank SST")
    def test_hierarchical_allreduce_sparse_comm_warning(self):
        self.mask_mpi_template(
            "test_hierarchical_allreduce_sparse_comm_warning",
            sdl="test_hierarchical_allreduce",
            model_args="--hierarchy hierarchical --allreduce iris.ring "
                       "--sparse-comm",
            expected_stderr="no SMP communicator for hierarchical "
                            "all-reduce; using flat")

    def test_bad_allreduce_stage_alg(self):
        self.mask_mpi_error_template(
            "test_bad_allreduce_stage_alg", "--intra iris.missing",
            "unknown allreduce algorithm 'iris.missing'")

    def test_bad_allreduce_hierarchy(self):
        self.mask_mpi_error_template(
            "test_bad_allreduce_hierarchy", "--hierarchy diagonal",
            "unknown collective.allreduce.hierarchy 'diagonal'")

    def test_flat_allreduce_stages(self):
        self.mask_mpi_error_template(
            "test_flat_allreduce_stages",
            "--hierarchy flat --intra iris.recdouble",
            "collective.allreduce.hierarchy=flat cannot be combined")

    def test_bad_collective_allreduce_field(self):
        self.mask_mpi_param_error_template(
            "test_bad_collective_allreduce_field", "collective.allreduce.bad",
            "unknown collective parameter 'collective.allreduce.bad'")

    def test_legacy_collective_params(self):
        migrations = (
            ("allgather", "collective.allgather"),
            ("alltoall", "collective.alltoall"),
        )
        for legacy, replacement in migrations:
            with self.subTest(parameter=legacy):
                self.mask_mpi_param_error_template(
                    "test_legacy_" + legacy, legacy,
                    'parameter "{}" has been replaced by "{}"'.format(
                        legacy, replacement))

    def test_unknown_param_outside_collective_namespace(self):
        self.mask_mpi_template(
            "test_unknown_param_outside_collective_namespace",
            sdl="test_allreduce", ref="test_allreduce",
            env={"ALG": "ring", "ALG_PARAM": "unrelated_param"})

    def mask_mpi_param_error_template(self, testcase, param, expected):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        outfile = os.path.join(outdir, testcase + ".out")
        errfile = os.path.join(outdir, testcase + ".err")
        self.run_sst(os.path.join(test_path, "test_allreduce.py"),
                     outfile, errfile, set_cwd=test_path, expected_rc=1,
                     env={"ALG": "ring", "ALG_PARAM": param})
        with open(errfile) as error_file:
            self.assertIn(expected, error_file.read())

    def test_alltoall(self):
        self.mask_mpi_template("test_alltoall", expected_rank_passes=8)

    def test_direct_alltoall(self):
        self.mask_mpi_template(
            "test_direct_alltoall", sdl="test_alltoall",
            ref="test_direct_alltoall", model_args="--alg iris.direct",
            expected_rank_passes=8)

    def test_allgather(self):
        self.mask_mpi_template("test_allgather")

    def test_ring_allgather(self):
        self.mask_mpi_template(
            "test_ring_allgather", sdl="test_allgather",
            ref="test_ring_allgather", model_args="--alg iris.ring")

    def test_halo3d26(self):
        self.mask_mpi_template("test_halo3d26")

#####

    def mask_mpi_error_template(self, testcase, model_args, expected):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        sdlfile = os.path.join(test_path, "test_hierarchical_allreduce.py")
        outfile = os.path.join(outdir, testcase + ".out")
        errfile = os.path.join(outdir, testcase + ".err")
        other_args = '--model-options="{}"'.format(model_args)
        self.run_sst(sdlfile, outfile, errfile, set_cwd=test_path,
                     other_args=other_args, expected_rc=1)
        with open(errfile) as error_file:
            self.assertIn(expected, error_file.read())

    def mask_mpi_template(self, testcase, striptotail=0, sdl=None, ref=None,
                          model_args=None, expected_stderr=None,
                          expected_rank_passes=None, env=None):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, sdl or testDataFileName)
        reffile = "{0}/refFiles/{1}.out".format(test_path,
                                                ref or testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        tmpfile = "{0}/{1}.tmp".format(tmpdir, testDataFileName)
        cmpfile = "{0}/{1}.cmp".format(tmpdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        run_args = {}
        if model_args:
            run_args["other_args"] = '--model-options="{}"'.format(model_args)
        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles,
                     set_cwd=test_path, env=env, **run_args)

        testing_remove_component_warning_from_file(outfile)

        if expected_rank_passes is not None:
            with open(outfile) as output_file:
                output = output_file.read()
            for rank in range(expected_rank_passes):
                self.assertIn("Rank {} PASS".format(rank), output)
            self.assertNotIn("FAIL", output)

        # Copy the outfile to the cmpfile
        os.system("cp {0} {1}".format(outfile, cmpfile))

        if striptotail == 1:
            # Post processing of the output data to scrub it into a format to compare
            os.system("grep Random {0} > {1}".format(outfile, tmpfile))
            os.system("tail -5 {0} > {1}".format(tmpfile, cmpfile))

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("hg test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))
        if expected_stderr is not None:
            with open(errfile) as error_file:
                self.assertIn(expected_stderr, error_file.read())

        cmp_result = testing_compare_sorted_diff(testcase, cmpfile, reffile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testcase)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(cmpfile, reffile))
