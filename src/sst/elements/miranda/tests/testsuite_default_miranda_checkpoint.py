#!/usr/bin/env python3

import os

from sst_unittest import *
from sst_unittest_support import *


class testcase_miranda_checkpoint(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()

    def tearDown(self):
        super(type(self), self).tearDown()

    @unittest.skipIf(testing_check_get_num_ranks() > 1, "miranda checkpoint: streambench restart skipped if ranks > 1")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "miranda checkpoint: streambench restart skipped if threads > 1")
    def test_miranda_streambench_checkpoint_restart(self):
        self.miranda_checkpoint_restart_test_template("streambench", checkpoint_period="50us")

    def miranda_checkpoint_restart_test_template(self, testcase, checkpoint_period, testtimeout=240):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()

        testDataFileName = "test_miranda_{0}_checkpoint_restart".format(testcase)
        checkpoint_prefix = "{0}_checkpoint".format(testDataFileName)
        checkpoint_time = checkpoint_period.replace("us", "000000")

        sdlfile = "{0}/{1}.py".format(test_path, testcase)
        generate_outfile = "{0}/{1}_generate.out".format(outdir, testDataFileName)
        generate_errfile = "{0}/{1}_generate.err".format(outdir, testDataFileName)
        restart_outfile = "{0}/{1}_restart.out".format(outdir, testDataFileName)
        restart_errfile = "{0}/{1}_restart.err".format(outdir, testDataFileName)

        checkpoint_args = "--checkpoint-sim-period={0} --checkpoint-prefix={1}".format(
            checkpoint_period, checkpoint_prefix
        )
        self.run_sst(
            sdlfile,
            generate_outfile,
            generate_errfile,
            other_args=checkpoint_args,
            set_cwd=outdir,
            timeout_sec=testtimeout,
        )

        checkpoint_manifest = "{0}/{1}/{1}_1_{2}/{1}_1_{2}.sstcpt".format(
            outdir, checkpoint_prefix, checkpoint_time
        )

        self.assertTrue(
            os.path.isfile(checkpoint_manifest),
            "miranda checkpoint manifest was not generated: {0}".format(checkpoint_manifest),
        )

        self.run_sst(
            checkpoint_manifest,
            restart_outfile,
            restart_errfile,
            set_cwd=outdir,
            timeout_sec=testtimeout,
        )

        if os_test_file(generate_errfile, "-s"):
            log_testing_note(
                "miranda checkpoint generation test {0} has a Non-Empty Error File {1}".format(
                    testDataFileName, generate_errfile
                )
            )

        if os_test_file(restart_errfile, "-s"):
            log_testing_note(
                "miranda checkpoint restart test {0} has a Non-Empty Error File {1}".format(
                    testDataFileName, restart_errfile
                )
            )

        self.assertFalse(
            os_test_file(generate_errfile, "-s"),
            "miranda checkpoint generation produced a non-empty error file: {0}".format(generate_errfile),
        )
        self.assertFalse(
            os_test_file(restart_errfile, "-s"),
            "miranda checkpoint restart produced a non-empty error file: {0}".format(restart_errfile),
        )

        with open(restart_outfile, "r") as restart_file:
            restart_output = restart_file.read()

        self.assertIn(
            "Simulation is complete",
            restart_output,
            "miranda checkpoint restart did not complete successfully: {0}".format(restart_outfile),
        )
