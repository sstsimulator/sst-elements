import sys
import math

class Column:
    """ Utility class used by OutputParser. Keeps track of header and all rows
        held by one column. This is the kind of object returned by OutputParser.getColumn().
    """

    def __init__(self, header):
        self.header    = header
        self.index     = 0    # Utility field with arbitrary semantics. See OutputParser.assignSpikeIndex() for one possible use.
        self.values    = []
        self.value     = 0.0  # For most recent row
        self.startRow  = 0
        self.textWidth = 0
        self.minimum   =  math.inf
        self.maximum   = -math.inf
        self.range     = 0.0
        self.color     = ''
        self.scale     = ''

    def computeStats(self):
        for f in self.values:
            if math.isinf(f) or math.isnan(f): continue
            self.minimum = min(self.minimum, f)
            self.maximum = max(self.maximum, f)
        if numpy.isinf(self.maximum):  # There was no good data. If max is infinite, then so is min.
            # Set defensive values.
            self.range   = 0.0
            self.minimum = 0.0
            self.maximum = 0.0
        else:
            self.range = self.maximum - self.minimum

    def get(self, row = -1, defaultValue = 0.0):
        if row < 0: return self.value
        row -= self.startRow
        if row < 0 or row >= len(self.values): return defaultValue
        return self.values[row]

    def set(self, row, value):
        self.fill(row, 0.0)
        self.values[row - self.startRow] = value

    def fill(self, row, defaultValue = 0.0):
        if row < self.startRow:
            self.values = [defaultValue] * (self.startRow - row) + self.values
            self.startRow = row
            return True
        last = self.startRow + len(self.values) - 1
        if row > last:
            self.values += [defaultValue] * (row - last)
            return True
        return False

    def insert(self, row, defaultValue = 0.0):
        """ Creates a new row at the given index with the given value.
            If the new row comes before or after the current block of rows,
            then the block is simply extended.
        """
        if self.fill(row, defaultValue): return
        self.values.insert(row, defaultValue)

class OutputParser:
    """ Primary class for reading and accessing data in an augmented output file.
        There are two main ways to use this class. One is to read the entire file into
        memory. The other is to read through the file row-by-row, without remembering
        anything but the current row. The second method is designed specifically for
        output files which exceed your system's memory capacity.
        Both interfaces use the get() functions to retrieve buffered values.
        The all-at-once interface uses the parse() function.
        The row-by-row interface uses the open() and nextRow() functions.
        It's a good idea not to mix these usages. Note that the parse() function is
        actually built on top of the row-by-row functions.
    """

    def __init__(self):
        self.columns      = []
        self.inFile       = None
        self.raw          = False  # Indicates that all column names are empty, likely the result of output() in raw mode.
        self.delimiter    = ' '
        self.delimiterSet = False
        self.isXycePRN    = False
        self.time         = None
        self.timeFound    = False  # Indicates that time is a properly-labeled column, rather than a fallback.
        self.rows         = 0      # Total number of rows successfully read by nextRow()
        self.defaultValue = 0.0

    def open(self, fileName):
        """ Use this function in conjunction with nextRow() to read file line-by-line
            without filling memory with more than one row.
        """
        self.close()
        self.inFile       = open(fileName)
        self.raw          = True  # Will be negated if any non-empty column name is found.
        self.delimiter    = ' '
        self.delimiterSet = False
        self.isXycePRN    = False
        self.time         = None
        self.timeFound    = False
        self.rows         = 0

    def close(self):
        if self.inFile is not None:
            self.inFile.close()
        self.inFile = None
        self.columns = []

    def nextRow(self):
        """ Reads in the next row of data values. Also processes header rows, but continues
            reading until a data row comes in.
            Returns number of columns found in current row. If zero, then end-of-file
            has been reached or there is an error.
        """
        if self.inFile is None: return 0
        for line in self.inFile:
            if line == "\n": continue
            if line[:6] == "End of": return 0  # Don't mistake Xyce final output line as a column header.
            if line[-1] == "\n": line = line[0:-1]  # strip trailing newline

            # Determine delimiter
            if not self.delimiterSet:
                if   '\t' in line: self.delimiter = '\t'  # highest precedence
                elif ','  in line: self.delimiter = ','
                # space character is lowest precedence
                self.delimiterSet = self.delimiter != ' ' or line.strip()

            c = 0  # Column index
            start = 0  # Current position for column scan.
            l = line[0]
            isHeader = (l < "0"  or  l > "9")  and  l != "+"  and  l != "-"  # any character other than the start of a float
            if isHeader: self.raw = False
            while True:
                pos = line.find(self.delimiter, start)
                if pos == -1:
                    value = line[start:]
                    start = -1
                else:
                    value = line[start:pos]
                    start = pos + 1

                # Notice that c can never be greater than column count, because we always fill in columns as we go.
                if isHeader:
                    if c == len(self.columns):
                        self.columns.append(Column(value))
                else:
                    if c == len(self.columns): self.columns.append(Column(""))
                    column = self.columns[c]
                    if value == "":
                        column.value = self.defaultValue
                    else:
                        column.textWidth = max(column.textWidth, len(value))
                        column.value = float(value)

                c += 1
                if start == -1: break

            if isHeader:
                self.isXycePRN =  self.columns[0].header == "Index"
            else:
                self.rows += 1
                return c
        return 0

    def parse(self, fileName, defaultValue = 0.0):
        """ Use this function to read the file into memory all at once.
        """
        self.defaultValue = defaultValue
        self.open(fileName)
        while True:
            count = self.nextRow()
            if count == 0: break
            c = 0
            for column in self.columns:
                if len(column.values) == 0: column.startRow = self.rows - 1
                if c < count: column.values.append(column.value)
                else:         column.values.append(defaultValue)  # Because the structure is not sparse, we must fill out every row.
                c += 1
        if len(self.columns) == 0:
            self.close()
            return  # failed to read any input, not even a header row

        # If there is a separate columns file, open and parse it.
        columnFileName = fileName + ".columns"
        try:
            columnFile = open(columnFileName)
            line = columnFile.readline()
            if line[0:10] == "N2A.schema":
                c = None
                for line in columnFile:
                    if line[-1] == "\n": line = line[0:-1]
                    pos = line.find(":")
                    if pos < 0: pos = len(line)
                    key = line[0:pos]
                    value = line[pos+1:]
                    if key[0] == ' ':
                        if c is None: continue
                        setattr(c, key[1:], value)
                    else:
                        i = int(key)
                        if i < 0 or i >= len(self.columns):
                            c = None
                            continue
                        c = self.columns[i]
                        if column.header == "": column.header = value
            columnFile.close()
        except OSError: pass

        # Determine time column
        self.time = self.columns[0]  # fallback, in case we don't find it by name
        timeMatch = 0
        for column in self.columns:
            potentialMatch = 0
            if   column.header == "t"   : potentialMatch = 1
            elif column.header == "TIME": potentialMatch = 2
            elif column.header == "$t"  : potentialMatch = 3
            if potentialMatch > timeMatch:
                timeMatch = potentialMatch
                self.time = column
                self.timeFound = True

        # Get rid of Xyce "Index" column, as it is redundant with row number.
        if self.isXycePRN: self.columns = self.columns[1:]

    def assignSpikeIndex(self):
        """ Optional post-processing step to give columns their position in a spike raster.
        """
        if self.raw:
            i = 0
            for c in self.columns:
                if  not self.timeFound  or  not c is self.time:
                    c.index = i
                    i += 1
        else:
            nextColumn = -1
            for c in self.columns:
                try:
                    c.index = int(c.header)
                except ValueError:
                    c.index = nextColumn
                    nextColumn = nextColumn - 1   # Yes, that's actually subtraction. Mis-named or unnamed columns get negative indices, reserving the positive indices for explicit and proper names.

    def getColumn(self, columnNameOrNumber):
        if type(columnNameOrNumber) is int:
            if columnNameOrNumber >= len(self.columns): return None
            return self.columns[columnNameOrNumber]
        for column in self.columns:
            if column.header == columnNameOrNumber: return column
        return None

    def getRow(self, row = -1):
        result = []
        for column in self.columns:
            result.append(column.get(row, self.defaultValue))
        return result

    def get(self, columnNameOrNumber, row = -1):
        column = self.getColumn(columnNameOrNumber)
        if column is None: return self.defaultValue
        return column.get(row)

    def set(self, columnNameOrNumber, row, value):
        c = self.getColumn(columnNameOrNumber)
        if not c:
            if type(columnNameOrNumber) is int:
                while len(self.columns) <= columnNameOrNumber: self.columns.append(Column(str(len(self.columns))))
                c = self.columns[columnNameOrNumber]
            else:
                c = Column(columnNameOrNumber)
                self.columns.append(c)
        c.set(row, value)
        self.rows = max(self.rows, c.startRow + len(c.values))

    def insertRow(self, row):
        """ Open a new row across all columns at the given row index.
            All values will be filled with the default, including the time column if one exists.
        """
        for c in self.columns:
            c.insert(row, self.defaultValue)
            self.rows = max(self.rows, c.startRow + len(c.values))

    def hasData(self):
        for column in self.columns:
            if len(column.values) > 0: return True
        return False

    def hasHeaders(self):
        for column in self.columns:
            if column.header != "": return True
        return False

    def dump(self, out=sys.stdout, separator="\t"):
        """ Dumps parsed data in tabular form. This can be used directly by most software.
            Pass separator="," to create a CSV file.
        """
        if len(self.columns) == 0: return
        last = self.columns[-1]

        if self.hasHeaders():
            e = separator
            for column in self.columns:
                if column is last: e = "\n"
                print(column.header, end=e, file=out)

        if self.hasData():
            for r in range(self.rows):
                e = separator
                for column in self.columns:
                    if column is last: e = "\n"
                    print(column.get(r), end=e, file=out)

    def dumpMode(self, out=sys.stdout):
        """ Dumps column metadata (from output mode field).
        """
        if self.hasHeaders():
            for column in self.columns:
                print(column.header, file=out)
                print("color=" + column.color, file=out)
                print("scale=" + column.scale, file=out)

if __name__ == "__main__":
    # Example of the all-at-once interface.
    o = OutputParser()
    o.parse("C:/Users/frothga/n2a/jobs/2020-05-27-205826-0/out")
    print('done')
