import sys

def getLinkParams( type ):
	if type == '5':
		linkBW='5GB/s'
		flitSize='32B'
		return linkBW, flitSize
	if type == '12.5':
		linkBW='12.5GB/s'
		flitSize='32B'
		return linkBW, flitSize
	if type == '25':
		linkBW='25GB/s'
		flitSize='32B'
		return linkBW, flitSize
	elif type == '50':
		linkBW='50GB/s'
		flitSize='48B'
		return linkBW, flitSize
	elif type == '125':
		linkBW='125GB/s'
		flitSize='64B'
		return linkBW, flitSize
	else:
		sys.exit()

