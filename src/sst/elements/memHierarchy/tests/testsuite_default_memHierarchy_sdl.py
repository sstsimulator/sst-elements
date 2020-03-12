# -*- coding: utf-8 -*-

import sst_unittest_support
from sst_unittest_support import *

################################################################################

def setUpModule():
    sst_unittest_support.setUpModule()
    # Put Module based setup code here. it is called before any testcases are run

def tearDownModule():
    # Put Module based teardown code here. it is called after all testcases are run
    sst_unittest_support.tearDownModule()

################################################################################

class testcase_memHierarchy_Component(SSTTestCase):

    @classmethod
    def setUpClass(cls):
        super(cls, cls).setUpClass()
        # Put class based setup code here. it is called once before tests are run

    @classmethod
    def tearDownClass(cls):
        # Put class based teardown code here. it is called once after tests are run
        super(cls, cls).tearDownClass()

#####

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_memHierarchy_sdl_1(self):
        #  sdl-1   Simple CPU + 1 level cache + Memory
        self.memHierarchy_Template("sdl_1", 500)

#    def test_memHierarchy_sdl_2(self):
#        #  sdl-2  Simple CPU + 1 level cache + DRAMSim Memory
#        self.memHierarchy_Template("sdl_2", 500)
#
#    def test_memHierarchy_sdl_3(self):
#        #  sdl-3  Simple CPU + 1 level cache + DRAMSim Memory (alternate block size)
#        memHierarchy_Template("sdl_3", 500)
#
#    def test_memHierarchy_sdl2_1(self):
#        #  sdl2-1  Simple CPU + 2 levels cache + Memory
#        memHierarchy_Template("sdl2_1", 500)
#
#    def test_memHierarchy_sdl3_1(self):
#        #  sdl3-1  2 Simple CPUs + 2 levels cache + Memory
#        memHierarchy_Template("sdl3_1", 500)
#
#    def test_memHierarchy_sdl3_2(self):
#        #  sdl3-2  2 Simple CPUs + 2 levels cache + DRAMSim Memory
#        memHierarchy_Template("sdl3_2", 500)
#
#    def test_memHierarchy_sdl3_3(self):
#        memHierarchy_Template("sdl3_3", 500)
#
#    def test_memHierarchy_sdl4_1(self):
#        memHierarchy_Template("sdl4_1", 500)
#
#    def test_memHierarchy_sdl4_2(self):
#        memHierarchy_Template("sdl4_2", 500)
#
#    def test_memHierarchy_sdl5_1(self):
#        memHierarchy_Template("sdl5_1", 500)
#
#    def test_memHierarchy_sdl8_1(self):
#        memHierarchy_Template("sdl8_1", 500)
#
#    def test_memHierarchy_sdl8_3(self):
#        memHierarchy_Template("sdl8_3", 500)
#
#    def test_memHierarchy_sdl8_4(self):
#        memHierarchy_Template("sdl8_4", 500)

# TODO: FIGURE OUT HOW TO RUN TEST TESTS BELOW
#    def test_memHierarchy_sdl9_1(self):
#        if [[ ${SST_MULTI_CORE:+isSet != isSet ]] ; then
#            self.memHierarchy_Template("sdl9_1", 500)
#        else                    # is multi core
#            self.memHierarchy_Template("sdl9_1_MC", 500)
#        fi
#
#    def test_memHierarchy_sdl9_2(self):
#        memHierarchy_Template("sdl9_2", 500)
#
#    def test_memHierarchy_sdl4_2_ramulator(self):
#        memHierarchy_Template("sdl4-2-ramulator", 500)
#
#
#
#    def test_memHierarchy_sdl5_1_ramulator(self):
#        pushd  $SST_REFERENCE_ELEMENTS/memHierarchy/tests
#        if [[ ${SST_MULTI_CORE:+isSet} != isSet ]] ; then
#            self.memHierarchy_Template("sdl5-1-ramulator", 500)
#        else
#            self.memHierarchy_Template("sdl5-1-ramulator_MC", 500)
#        fi
#        popd
#
#
#
#    def test_print_DramSim_summary(self):
#        if [ $DRAMSIM_EXACT_MATCH_FAIL_COUNT != 0 ] ; then
#            echo "DramSim exact match failed on:" > $$_tmp
#            echo $FAIL_LIST  >> $$_tmp
#        else
#            echo "        DramSim sdl tests all got exact match"  >> $$_tmp
#        fi
#        echo ' '
#        skip_this_test

#####

    def memHierarchy_Template(self, testcase, tolerance):
        # Set the various file paths
        testDataFileName="test_memHierarchy_{0}".format(testcase)
        sdlfile = "{0}/{1}.py".format(get_testsuite_dir(), testcase.replace("_", "-"))
        reffile = "{0}/refFiles/test_memHierarchy_{1}.out".format(get_testsuite_dir(), testcase)
        outfile = "{0}/{1}.out".format(get_test_output_run_dir(), testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(get_test_output_run_dir(), testDataFileName)

        self.run_sst(sdlfile, outfile, mpi_out_files=mpioutfiles)

        # Perform the test
        cmp_result = compare_sorted(testcase, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))




# THIS IS THE ORIGINAL memHierarchy_Template from the SQE Repository
#        memHierarchy_Template() {
#        memH_case=$1
#        Tol=$2    ##  curTick tolerance
#
#            startSeconds=`date +%s`
#            testDataFileBase="test_memHierarchy_$memH_case"
#            memH_test_dir=${SST_REFERENCE_ELEMENTS}/memHierarchy/tests
#
#            referenceFile="$memH_test_dir/refFiles/${testDataFileBase}.out"
#            outFile="${SST_TEST_OUTPUTS}/${testDataFileBase}.out"
#
#            testOutFiles="${SST_TEST_OUTPUTS}/${testDataFileBase}.testFiles"
#            tmpFile="${SST_TEST_OUTPUTS}/${testDataFileBase}.tmp"
#            errFile="${SST_TEST_OUTPUTS}/${testDataFileBase}.err"
#
#            RF_TDFB=`echo ${testDataFileBase} | sed s/-/_/g`
#            echo "TDFB $RF_TDFB"
#            referenceFile="$memH_test_dir/refFiles/${RF_TDFB}.out"
#            outFile="${SST_TEST_OUTPUTS}/${RF_TDFB}.out"
#
#            # Add basename to list for processing later
#            L_TESTFILE+=(${testDataFileBase})
#            pushd $SST_ROOT/sst-elements/src/sst/elements/memHierarchy/tests
#            rm -f dramsim*.log
#
#            sut="${SST_TEST_INSTALL_BIN}/sst"
#
#        echo ' '; echo Find pyFileName
#            pyFileName=`echo ${memH_case}.py | sed s/_/-/ | sed s/_MC// | sed s/-MC//`
#        echo ' '; echo $pyFileName
#        echo ' '
#            sutArgs=${SST_ROOT}/sst-elements/src/sst/elements/memHierarchy/tests/$pyFileName
#            echo $sutArgs
#            grep backend $sutArgs | grep dramsim > /dev/null
#            usingDramSim=$?
#
#            ls $sutArgs > /dev/null
#            if [ $? != 0 ]
#            then
#              ls $sutArgs
#              echo ' FAILED to find Python file.'
#              echo ' '
#              ls *.py
#              echo ' '
#              fail ' FAILED to find Python file.'
#              popd
#              return
#            fi
#
#            if [[ ${SST_MULTI_RANK_COUNT:+isSet} != isSet ]] || [ ${SST_MULTI_RANK_COUNT} -lt 2 ] ; then
#                 ${sut} ${sutArgs} > ${outFile}  2>${errFile}
#                 RetVal=$?
#                 notAlignedCt=`grep -c 'not aligned to the request size' $errFile`
#                 #          Append errFile to outFile   w/o  Not Aligned messages
#                 grep -v 'not aligned to the request size' $errFile >> $outFile
#            else
#                 #   This merges stderr with stdout
#                 mpirun -np ${SST_MULTI_RANK_COUNT} $NUMA_PARAM -output-filename $testOutFiles ${sut} ${sutArgs} 2>${errFile}
#                 RetVal=$?
#                 cat ${testOutFiles}* > $outFile
#                 notAlignedCt=`grep -c 'not aligned to the request size' $outFile`
#            fi
#
#            TIME_FLAG=$SSTTESTTEMPFILES/TimeFlag_$$_${__timerChild}
#            if [ -e $TIME_FLAG ] ; then
#                 echo " Time Limit detected at `cat $TIME_FLAG` seconds"
#                 fail " Time Limit detected at `cat $TIME_FLAG` seconds"
#                 rm $TIME_FLAG
#                 return
#            fi
#            if [ $RetVal != 0 ] ; then
#                 echo ' '; echo WARNING: sst did not finish normally ; echo ' '
#                 ls -l ${sut}
#                 fail "WARNING: sst did not finish normally, RetVal=$RetVal"
#                 #   The following is not complete for all cases but is benign
#                 if [ -s ${errFile} ] ; then
#                     echo ' ' ; echo "* * * *  $notAlignedCt Not Aligned messages from $memH_case   * * * *" ; echo ' '
#                     echo "         stderr File  $memH_case"
#                     cat $errFile | grep -v 'not aligned to the request size'
#                     echo "          ----------"
#                 fi
#                 popd            #   Why popd here but not on Time Limit?
#                 return
#            fi
#        #                   --- It completed normally ---
#
#            if [ $notAlignedCt != 0 ] ; then
#                echo ' '
#                if [ $usingDramSim == 0 ] ; then    ## usingDramSim is TRUE
#                     echo "          This case uses DramSim"
#                fi
#                echo "* * * *   $notAlignedCt Not Aligned messages from $memH_case   * * * *" ; echo ' '
#            fi
#
#            if [ -s ${errFile} ] ; then
#                echo "         STDERR (omiting \"not aligned\" messages"
#                cat $errFile | grep -v 'not aligned to the request size'
#                echo "         ----- end stderr"
#            fi
#
#            grep -v ^cpu.*: $outFile > $tmpFile
#            grep -v 'not aligned to the request size' $tmpFile > $outFile
#            RemoveComponentWarning
#            pushd ${SSTTESTTEMPFILES}
#        #          Append errFile to outFile   w/o  Not Aligned messages
#            diff -b $referenceFile $outFile > ${SSTTESTTEMPFILES}/_raw_diff
#            if [ $? == 0 ] ; then
#                fileSize=`wc -l $outFile | awk '{print $1}'`
#                echo "            Exact Match of reduced Output  -- $fileSize lines"
#                rm ${SSTTESTTEMPFILES}/_raw_diff
#            else
#                wc $referenceFile $outFile
#                wc ${SSTTESTTEMPFILES}/_raw_diff
#                rm diff_sorted
#                compare_sorted $referenceFile $outFile
#                if [ $? == 0 ] ; then
#                   echo " Sorted match with Reference File"
#                   rm ${SSTTESTTEMPFILES}/_raw_diff
#                else
#        cat ${SSTTESTTEMPFILES}/diff_sorted
#                   echo "`diff $referenceFile $outFile | wc` $memH_case" >> ${SST_TEST_INPUTS_TEMP}/$$_diffSummary
#           #                             --- Special case with-DramSim ---
#                   if [ $usingDramSim == 0 ] ; then    ## usingDramSim is TRUE
#                      echo "            wc the diff"
#                      diff ${outFile} ${referenceFile} | wc; echo ' '
#                      echo " Remove the histogram"
#                      ref=`grep -v '\[.*\]' ${referenceFile} | wc | awk '{print $1, $2}'`;
#                      new=`grep -v '\[.*\]' ${outFile}       | wc | awk '{print $1, $2}'`;
#                      if [ "$ref" == "$new" ];
#                      then
#                          echo " Word / Line count of matches (using DRAMSim)"
#
#                          FAIL_LIST="$FAIL_LIST $memH_case"
#
#                          DRAMSIM_EXACT_MATCH_FAIL_COUNT=$(($DRAMSIM_EXACT_MATCH_FAIL_COUNT+1 ))
#                          echo " COUNT $DRAMSIM_EXACT_MATCH_FAIL_COUNT"
#                          echo " [${FAIL_LIST}]"
#
#                      else
#                          echo " FAILURE:    Line / Word count do not agree "
#                          FAIL_LIST="$FAIL_LIST $memH_case"
#                          DRAMSIM_EXACT_MATCH_FAIL_COUNT=$(($DRAMSIM_EXACT_MATCH_FAIL_COUNT+1 ))
#                          if [ -s $errFile ]; then
#                              echo  "STDERR is:    ------------------"
#                              cat $errFile | grep -v 'not aligned to the request size'
#                              echo  "END of STDERR --------------------"
#                          fi
#                          fail " FAILURE:    Line / Word count do not agree (using DRAMSIM)"
#                          echo "Examine up to twenty lines of sorted difference"
#                          cat diff_sorted | sed 20q
#                      fi
#                   else
#                      fail "Output does not match Reference (w/o DRAMSim)"
#                   fi              ##    --- end of code for Dramsim case ----
#
#                fi
#            fi
#            popd
#            grep "Simulation is complete, simulated time:" $tmpFile
#            if [ $? != 0 ] ; then
#                echo "Completion test message not found"
#                fail "Completion test message not found"
#                echo ' '; grep -i complet $tmpFile ; echo ' '
#            fi
#
#            endSeconds=`date +%s`
#            elapsedSeconds=$(($endSeconds -$startSeconds))
#            echo "${memH_case}: Wall Clock Time  $elapsedSeconds seconds"
#            echo " "
#            popd
#
#        }
#


