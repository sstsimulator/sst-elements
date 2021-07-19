import sys

class Buffer:
    def __init__(self):
        self.buffer = ''
        self.offset = 0
    def write( self, data ):
        self.buffer += data
    def readline( self ):
        end = self.offset
        while end < len(self.buffer) and self.buffer[end] != '\n':
            end += 1
        start = self.offset
        self.offset = end + 1
        return self.buffer[start:self.offset]

class ParseLoadFile:

    def __init__( self, filename, fileVars ):
        self.fp = open(filename, 'r')
        self.buffer = Buffer()
        self.preprocess( fileVars )
        self.lastLine = self.getline()

        self.stuff = [] 
        while True:
            key, value = self.getKeyValue();
            if key == None:
                break

            if key == '[JOB_ID]':
                self.stuff = self.stuff + [{ 'jobid': int(value) }]
                self.stuff[-1]['motifs'] = [] 
                self.stuff[-1]['params'] = {} 
            elif key == '[NID_LIST]':
                value = ''.join(value.split())
                if value[0].isdigit():
                    self.stuff[-1]['nid_list'] = value.strip()
                else:
                    left,right = value.split('=') 
                    if left == 'generateNidList':
                        self.stuff[-1]['nid_list'] = self.generateNidList( right ) 
                    else:
                        sys.exit('ERROR: invalid NID_LIST {0}'.format(value))

            elif key == '[NUM_CORES]':
                self.stuff[-1]['num_cores'] = value.strip()
            elif key == '[PARAM]':
                key,value = value.strip().split('=')
                if key == 'ember:famAddrMapper.nidList' and value[0:len('generateNidList')] == 'generateNidList':
                    self.stuff[-1]['params'][key] = self.generateNidList( value )
                else:
                    self.stuff[-1]['params'][key] = value
            elif key == '[MOTIF]':
                self.stuff[-1]['motifs'] = self.stuff[-1]['motifs'] + [value] 
            else:
                print('Warning: unknown key {0}'.format(key))

        self.fp.close() 

    def __iter__(self):
            return self

    def generateNidList( self, generator ):
        name,args = generator.split('(',1)
        try:
            module = __import__( name, fromlist=[''] )
        except:
            sys.exit('Failed: could not import nidlist generator `{0}`'.format(name) )

        return module.generate( args.split(')',1)[0] ) 

    def __next__(self):

        if len(self.stuff) == 0:
            raise StopIteration
        else :
            jobid = self.stuff[0]['jobid'] 
            nidlist = self.stuff[0]['nid_list']
            numCores = 1
            if 'num_cores' in self.stuff[0]:
                numCores = self.stuff[0]['num_cores']
            params = self.stuff[0]['params']
            motifs = self.stuff[0]['motifs']
            self.stuff.pop(0)
            return jobid, nidlist, numCores, params, motifs 

    next = __next__

    def substitute( self, line, variables ):
        retval = ''
        line = line.replace('}','{')
        line = line.split('{')
        for x in line:
            if x in variables:
                retval += variables[x]
            else:
                retval += x

        return retval

    def preprocess( self, vars ):
        while True:
            line = self.fp.readline()
            if len(line) > 0:
                if line[0] != '#' and not line.isspace():
                    if line[0:len('[VAR]')] == '[VAR]':
                        tag, rem = line.split(' ',1);
                        var,value = rem.split('=');
                        vars[var] = value.rstrip()
                    else:
                        self.buffer.write( self.substitute(line,vars) )
            else:
                return


    def getKeyValue( self ):

        if self.lastLine == None:
            return None,None
        value = ''
        if self.lastLine[0] != '[':
            sys.exit('badly formed file ');

        tag = None
        rem = '' 
        try: 
            tag, rem = self.lastLine.split(' ',1);
            rem = rem.replace('\n', ' ')
        except ValueError:
            tag = self.lastLine

        tag = tag.replace('\n', ' ')
       
        value = value + rem
        while True:
            self.lastLine = self.getline()
            
            if self.lastLine == None:
                break
            if self.lastLine[0] == '[':
                break
            else:
                value = value + self.lastLine.replace('\n',' ') 

        return tag, value

    def getline(self):
        while True:
            line = self.buffer.readline()
            if len(line) > 0:
                if not line.isspace() and line[0] != '#': 
                    return line
            else:
                return None 

