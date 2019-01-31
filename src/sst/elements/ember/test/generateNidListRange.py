def generate( args ):
    args = args.split(',')
    start = int(args[0])    
    length = int(args[1])
    #print 'generate', start, length 
    return str(start) + '-' + str( start + length - 1 )
