def generate( args ):
    args = args.split(',')
    start = int(args[0])    
    length = int(args[1])
    interval = int(args[2])
    ret = '' 
    for i in range(length):
        node = i * interval + start
        ret = ret + str(node)
        if i + 1 < length:
            ret = ret + ',' 
    #print( 'generate( {}, {}, {} ) = \'{}\'').format( start, length, interval, ret )

    return ret
