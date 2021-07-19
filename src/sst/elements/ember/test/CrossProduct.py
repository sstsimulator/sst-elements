
class CrossProduct:
    def __init__(self,xxx):
        self.xxx = xxx
        self.cnts = []
        for i, x in enumerate( xxx ) :
            self.cnts.insert( i, len( x[1] ) )

    def __iter__(self):
        return self

    def __next__(self):
        if self.cnts[0] == 0:
            raise StopIteration
        else:
            tmp = ''
            for i, x in enumerate(self.xxx) :
                tmp += "{0}={1} ".format( x[0], x[1][ self.cnts[i] - 1 ] )

            for i in reversed(range( 0, len(self.xxx) )):
                self.cnts[i] -= 1
                if  i == 0 and self.cnts[i] == 0:
                    break

                if self.cnts[i] > 0 :
                    break
                else :
                    self.cnts[i] = len(self.xxx[i][1])

            return tmp

    def next(self):
        return self.__next__()

