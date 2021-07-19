def generate( args ):
    args = args.split(',')
    start = int(args[0])    
    numGroups = int(args[1])
    perGroup = int(args[2])
    interval = int(args[3])
    ret = '' 
    for i in range(numGroups):

        first = start + i * interval
        last = first + perGroup - 1 

        ret = ret + '{0}-{1}'.format(first,last) 
        if i + 1 < numGroups:
            ret = ret + ',' 

    return ret
