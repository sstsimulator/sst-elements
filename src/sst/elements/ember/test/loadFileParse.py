import pprint
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
            elif key == '[NID_LIST]':
                self.stuff[-1]['nid_list'] = value.strip()
            elif key == '[MOTIF]':
                self.stuff[-1]['motifs'] = self.stuff[-1]['motifs'] + [value] 
            else:
                sys.exit('foobar')

        self.fp.close() 
        #pprint.pprint(self.stuff)

    def __iter__(self):
            return self

    def next(self):

        if len(self.stuff) == 0:
            raise StopIteration
        else :
            jobid = self.stuff[0]['jobid'] 
            nidlist = self.stuff[0]['nid_list']
            motifs = self.stuff[0]['motifs']
            self.stuff.pop(0)
            return jobid, nidlist, motifs 
            

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

