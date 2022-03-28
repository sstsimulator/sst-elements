#!/bin/bash

TEMP_FILE=""

for myFile in *.dot
do
    TEMP_FILE=${myFile}
    TEMP_FILE=`echo ${TEMP_FILE} | sed 's/[.].*$//'`
    echo ${myFile}
#     echo ${TEMP_FILE}
    dot -Tpdf ${myFile} -o ${TEMP_FILE}.pdf &> /dev/null
done

