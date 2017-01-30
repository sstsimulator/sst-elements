import sys

tera=1000*1000*1000*1000
def getParams( type ):
	if type == 'test':
		flops = 5 * tera
		numCores = 1
		return numCores, flops/numCores
	if type == 'tiny':
		flops = 5 * tera
		numCores = 1
		return numCores, flops/numCores
	if type == 'big':
		flops = 150 * tera
		numCores = 1
		return numCores, flops/numCores
	if type == 'thin':
		flops = 10 * tera
		numCores = 1
		return numCores, flops/numCores
	elif type == 'medium':
		flops = 40 * tera
		numCores = 2 
		return numCores, flops/numCores
	elif type == 'fat':
		flops = 160 * tera 
		numCores =8 
		return numCores, flops/numCores
	else:
		sys.exit("ERROR node type " + type )

