'''
Converts workload traces from the standard workload format
to SST scheduler format
usage: python convertTrace.py <input file name>
'''

import sys
import numpy as np

if __name__ == '__main__':
    #check input
    if len(sys.argv) != 3:
    	print "usage: python convertTrace.py <input file name> <time units>"
    	sys.exit()

    #check time units
    timeUnits = sys.argv[2]
    if timeUnits == 's':
        factor = 1
    elif timeUnits == 'ms':
        factor = 1000
    elif timeUnits == 'us':
        factor = 1000000
    elif timeUnits == 'ns':
        factor = 1000000000
    else:
        print "Valid time units: [s | ms | us | ns]"
        sys.exit()

    #load data
    print "Converting..."
    data = np.loadtxt(sys.argv[1], comments = ';')
    # The order: [Arrival time | Number of processors | Runtime | Requested Time]
    data = np.array([data[:,1], data[:,4], data[:,3], data[:,8]])
    data = np.int_(data.T)
    unitConvertedData = np.array([factor*data[:,0], data[:,1], factor*data[:,2], factor*data[:,3]])
    unitConvertedData = np.int_(unitConvertedData.T)
    for i in range(len(unitConvertedData)):
        if(unitConvertedData[i,3] < 0):
            unitConvertedData[i,3] = -1

    #write data
    np.savetxt(sys.argv[1] + ".sim", unitConvertedData, delimiter=' ', fmt='%.0f')
    print "Completed!"
    
    '''
    #Older version without unit conversion
    data = np.int_(data.T)
    #write data
    np.savetxt(sys.argv[1] + ".sim", data, delimiter=' ', fmt='%.0f')
    print "Completed!"
    '''