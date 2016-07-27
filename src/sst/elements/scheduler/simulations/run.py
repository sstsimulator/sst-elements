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


# Function to run linux commands
def run(cmd):
    #print(cmd)
    os.system(cmd)

def submit_job(options):
    exp_name = "N%s_alpha%s_%s_%s_%s_iter%s" %(options.N, options.alpha, options.application, options.allocator, options.mapper, options.iteration)

    options.outdir = "%s/%s/%s" %(options.main_sim_path, options.exp_folder, exp_name)
    #os.environ['SIMOUTPUT'] = folder
    execcommand  = "hostname\n"
    execcommand += "date\n"
    execcommand += "source %s\n" %(options.env_script)
    execcommand += "export SIMOUTPUT=%s/\n" %(options.outdir)
    execcommand += "python run_DetailedNetworkSim.py --emberOut ember.out --alpha %s --schedPy ./%s_%s_%s_N%s.py\n" %(options.alpha, options.allocator, options.mapper, options.application, options.N)
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
        #cmd = ("qsub -q bungee.q,budge.q -cwd -l mem_free=4G,s_vmem=4G -S /bin/bash -o %s -j y %s" % (outfile, shfile))
        cmd = ("qsub -q me.q -cwd -S /bin/bash -o %s -j y %s" % (outfile, shfile))
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
    
    '''    
    for N in [1, 2, 4, 8]: # Each job uses 1/N of the machine
        for alpha in [0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4]: 
            for application in ['alltoall', 'bisection', 'mesh']:
                for allocator in ['simple', 'spread', 'random']:
                    for mapper in ['libtopomap']:
    '''
    for N in [1]: # Each job uses 1/N of the machine
        #for alpha in [0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4]:
        for alpha in [4]:
            if N == 1:
                #applications = ['alltoall', 'bisection', 'mesh']
                applications = ['alltoall']
            else:
                applications = ['alltoall', 'mesh']

            for application in applications:
                if application == 'bisection':
                    allocators = ['simple']
                    mappers = ['simple']
                else:
                    #allocators = ['simple', 'spread', 'random']
                    allocators = ['simple']
                    mappers = ['libtopomap']

                for allocator in allocators:
                    for mapper in mappers:
                        if allocator == 'random':
                            num_iters = 1
                        else:
                            num_iters = 1
                        for iteration in range(num_iters):
                            options.N = N
                            options.alpha = alpha
                            options.application = application
                            options.allocator = allocator
                            options.mapper = mapper
                            options.iteration = iteration
                            submit_job(options)
    '''
    for N in [1]: # Each job uses 1/N of the machine
        #for alpha in [0.125, 0.25, 0.5, 0.625, 0.75, 0.875, 1]:
        for alpha in [4]:
            for application in ['alltoall']:
                for allocator in ['simple']:
                    for mapper in ['libtopomap']:
                        if allocator == 'random':
                            num_iters = 1
                        else:
                            num_iters = 1
                        for iteration in range(num_iters):
                            options.N = N
                            options.alpha = alpha
                            options.application = application
                            options.allocator = allocator
                            options.mapper = mapper
                            options.iteration = iteration
                            submit_job(options)
    '''
    '''
    #Network steady state analysis
    for N in [1]: # Each job uses 1/N of the machine
        for alpha in [4]: 
            for application in ['alltoall']:
                for allocator in ['simple']:
                    for mapper in ['libtopomap']:
                        if allocator == 'random':
                            num_iters = 100
                        else:
                            num_iters = 1
                        for iteration in range(num_iters):
                            for motif_iteration in range(10,110,10):
                                options.N = N
                                options.alpha = alpha
                                options.application = application
                                options.allocator = allocator
                                options.mapper = mapper
                                options.iteration = iteration
                                options.motif_iteration = motif_iteration
                                submit_job(options)
    '''


if __name__ == '__main__':
    main()
