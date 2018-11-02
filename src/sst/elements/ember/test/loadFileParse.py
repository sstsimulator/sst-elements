import pprint,sys


class ParseLoadFile:
    def __init__( self, filename ):
        self.fp = open(filename, 'r')
        self.lastLine = self.getline()

        self.stuff = [] 
        while True:
            key, value = self.getKeyValue();
            if key == None:
                break

            if key == '[JOB_ID]':
                self.stuff = self.stuff + [{ 'jobid': int(value) }]
                self.stuff[-1]['motifs'] = [] 
                self.stuff[-1]['motif_api'] = '' 
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
                        sys.exit('ERROR: invalid NID_LIST {}'.format(value))

            elif key == '[PARAM]':
                key,value = value.strip().split('=')
                self.stuff[-1]['params'][key] = value 
            elif key == '[MOTIF_API]':
                self.stuff[-1]['motif_api'] = value.strip()
            elif key == '[MOTIF]':
                self.stuff[-1]['motifs'] = self.stuff[-1]['motifs'] + [value] 
            else:
                sys.exit('ERROR: unknown key {}'.format(key))

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

    def next(self):

        if len(self.stuff) == 0:
            raise StopIteration
        else :
            jobid = self.stuff[0]['jobid'] 
            nidlist = self.stuff[0]['nid_list']
            params = self.stuff[0]['params']
            motif_api = self.stuff[0]['motif_api']
            motifs = self.stuff[0]['motifs']
            self.stuff.pop(0)
            return jobid, nidlist, params, motif_api, motifs 
            

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
            rem = rem.replace('\n', '')
        except ValueError:
            tag = self.lastLine

        tag = tag.replace('\n', '')
       
        value = value + rem
        while True:
            self.lastLine = self.getline()
            
            if self.lastLine == None:
                break
            if self.lastLine[0] == '[':
                break
            else:
                value = value + self.lastLine.replace('\n','') 

        return tag, value

    def getline(self):
        while True:
            line = self.fp.readline()

            if len(line) > 0:
                if not line.isspace() and line[0] != '#': 
                    return line
            else:
                return None 

