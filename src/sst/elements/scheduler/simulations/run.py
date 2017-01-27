#!/usr/bin/env python
'''
Date        : 05/17/2016  
Created by  : Fulya Kaplan
Description : Run a batch of sst jobs with NetworkSim
'''

import os, sys

from optparse import OptionParser
import os.path, math
import numpy as np
import copy
import csv


# Function to run linux commands
def run(cmd):
    #print(cmd)
    os.system(cmd)

def submit_job(options):
    exp_name = "%s_%s_%s_%sKB_N%s_alpha%s_%s_%s_%s_iter%s" %(options.system_name, options.link_arrangement, options.routing, options.message_size, options.N, options.alpha, options.application, options.allocator, options.mapper, options.iteration)

    options.outdir = "%s/%s/%s" %(options.main_sim_path, options.exp_folder, exp_name)
    #os.environ['SIMOUTPUT'] = folder
    execcommand  = "hostname\n"
    execcommand += "date\n"
    execcommand += "source %s\n" %(options.env_script)
    execcommand += "export SIMOUTPUT=%s/\n" %(options.outdir)
    if options.application == "alltoall" or options.application == "alltoallnative" or options.application == "bisection":        
        execcommand += "python run_DetailedNetworkSim.py --emberOut ember.out --alpha %s --link_arrangement %s --rankmapper linear --routing %s --shuffle --schedPy ./%s_%s_%s_%s_N%s_%sKB.py\n" %(options.alpha, options.link_arrangement, options.routing, options.system_name, options.allocator, options.mapper, options.application, options.N, options.message_size)
    else:
        execcommand += "python run_DetailedNetworkSim.py --emberOut ember.out --alpha %s --link_arrangement %s --rankmapper custom --routing %s --schedPy ./%s_%s_%s_%s_N%s_%sKB.py\n" %(options.alpha, options.link_arrangement, options.routing, options.system_name, options.allocator, options.mapper, options.application, options.N, options.message_size)
    execcommand += "date\n"

    shfile = "%s/%s.sh" %(options.outdir, exp_name)
    outfile = "%s/%s.out" %(options.outdir, exp_name)

    #Check name only
    if options.check == True:
        print(options.outdir)
        print(execcommand)
        print(shfile)
        print(outfile)
        print
    #Launch the experiment
    else:
        if os.path.exists(options.outdir) == 1:
            if options.force == 1:
                print("Clobbering %s... used -f flag" %(exp_name))
                run("rm -rf " + options.outdir)
            else:
                print("Experiment %s exists... quitting. use -f to force" %(options.exp_name))
                sys.exit(1)
        run("mkdir -p " + options.outdir)

        shellfile = open(shfile, "w")
        shellfile.writelines(execcommand)
        shellfile.close()

        cmd = "chmod +x %s" %(shfile)
        run(cmd)
        cmd = ("qsub -q bme.q,budge.q,bungee.q,icsg.q -cwd -l mem_free=16G,s_vmem=16G -S /bin/bash -o %s -j y %s" % (outfile, shfile))
        run(cmd)
        #run("%s" %(shfile))

def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("-c",  action='store_true', dest="check", help="Check the experiment file names.")
    parser.add_option("-f",  action='store_true', dest="force", help="Force the experiment, will clobber old results.")
    parser.add_option("-e",  action='store', dest="exp_folder", help="Main experiment folder that holds all subfolders of the experiment.")

    (options, args) = parser.parse_args()

    if options.check == True:
        print("Action : Checking experiment file names")
    else:
        print("Action : Launching experiment")

    options.main_sim_path = "/mnt/nokrb/fkaplan3/SST/git/sst/sst-elements/src/sst/elements/scheduler/simulations"
    options.env_script = "/mnt/nokrb/fkaplan3/tools/addsimulator.sh"
    
    #system_name = "system_110nodes_220cores"
    #system_name = "system_72nodes_144cores"
    system_name = "system_272nodes_544cores"

    '''
    missing_results_file = "%s/missingResults.txt" %(options.main_sim_path)
    mf = open(missing_results_file, 'r')
    for l in mf:
        line = l.split('cores_')[1]
        line = line.split('_')
        alpha = line[4].split('alpha')[1]
        iteration = line[8].split('iter')[1]
        iteration = iteration.split('\n')[0]
        #print line

        options.system_name = system_name
        options.message_size = line[2]
        options.routing = line[1]
        options.link_arrangement = line[0]
        options.N = line[3]
        options.alpha = alpha
        options.application = line[5]
        options.allocator = line[6]
        options.mapper = line[7]
        options.iteration = iteration
        #print options
        submit_job(options)

    mf.close()    
    '''
    '''
    for N in [1]: # Each job uses 1/N of the machine
        for message_size in [1000]:
            applications = ['alltoallnative']
            allocators = ['simple']

            for application in applications:
                for allocator in allocators:
                    mappers = ['simple']
                    for mapper in mappers:
                        for alpha in [3]:
                            for routing in ['minimal']:
                                for link_arrangement in ['relative']:
                                    iteration = 3
                                    options.system_name = system_name
                                    options.message_size = message_size
                                    options.routing = routing
                                    options.link_arrangement = link_arrangement
                                    options.N = N
                                    options.alpha = alpha
                                    options.application = application
                                    options.allocator = allocator
                                    options.mapper = mapper
                                    options.iteration = iteration
                                    submit_job(options)

    '''
    for N in [1]: # Each job uses 1/N of the machine
        for message_size in [100]:
            if N == 1:
                #applications = ['alltoallnative', 'bisection', 'stencil']
                applications = ['alltoall']
                #applications = ['bisection']
                allocators = ['simple']
            else:
                applications = ['alltoallnative', 'stencil']
                allocators = ['simple', 'simplespread', 'random']

            for application in applications:
                for allocator in allocators:
                    if application == "stencil":
                        mappers = ['topo']
                    else:
                        mappers = ['simple']
                    for mapper in mappers:
                        for alpha in [0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4]:
                            for routing in ['minimal', 'valiant']:
                                for link_arrangement in ['absolute', 'relative', 'circulant']:
                                    if allocator == 'random' or application == 'alltoallnative' or application == 'alltoall' or application == 'bisection':
                                        num_iters = 15
                                    else:
                                        num_iters = 1
                                    for iteration in range(num_iters):
                                        options.system_name = system_name
                                        options.message_size = message_size
                                        options.routing = routing
                                        options.link_arrangement = link_arrangement
                                        options.N = N
                                        options.alpha = alpha
                                        options.application = application
                                        options.allocator = allocator
                                        options.mapper = mapper
                                        options.iteration = iteration
                                        submit_job(options)

    
    '''
    for N in [1]: # Each job uses 1/N of the machine
        for message_size in [1000]:
            if N == 1:
                applications = ['alltoall', 'bisection', 'stencil']
                allocators = ['simple']

            for application in applications:
                for allocator in allocators:
                    if application == "stencil":
                        mappers = ['topo']
                    else:
                        mappers = ['simple']
                    for mapper in mappers:
                        for alpha in [4]:
                            for routing in ['minimal']:
                                for link_arrangement in ['absolute']:
                                    if allocator == 'random':
                                        num_iters = 20
                                    else:
                                        num_iters = 1
                                    for iteration in range(num_iters):
                                        options.system_name = system_name
                                        options.message_size = message_size
                                        options.routing = routing
                                        options.link_arrangement = link_arrangement
                                        options.N = N
                                        options.alpha = alpha
                                        options.application = application
                                        options.allocator = allocator
                                        options.mapper = mapper
                                        options.iteration = iteration
                                        submit_job(options)
    '''

if __name__ == '__main__':
    main()
