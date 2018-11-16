import sys

Truncate=40

def truncate(value):
	if Truncate and len(value) > Truncate:
		return value[0:Truncate] + '...'
	else:
		return value

def updateDict( name, params, key, value ):
    if key in params:
        if str(value) != str(params[key]):
            print "override {} {}={} with {}".format( name, key, params[key], truncate(value) )
            params[ key ] = value 
    else:
        print "set {} {}={}".format( name, key, truncate(value) )
        params[ key ] = value 

def updateParams( params, merlinParams, nicParams, emberParams ):
    for key, value in params.items(): 
        prefix, suffix = key.split(':')

        if prefix == 'nic':
            updateDict( 'nicParams', nicParams, suffix, value ) 
        elif prefix == 'ember':
            updateDict( 'emberParams', emberParams, suffix, value ) 
        elif prefix == 'merlin':
            updateDict( 'merlinParams', merlinParams, suffix, value ) 
        else:
            sys.exit('ERROR: unknown dictionary {}'.format(prefix))

