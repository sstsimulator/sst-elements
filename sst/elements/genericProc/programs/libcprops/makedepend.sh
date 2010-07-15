#!/bin/sh

file=Makefile

if [ ! -e $file ] ; then
  file=makefile
  if [ ! -e $file ] ; then
	echo makedepend.sh: error:  [mM]akefile is not present
	exit 1
  fi
fi

echo "# DO NOT DELETE" >> $file
echo >> $file

for a in $@
do
  flag=`echo $a | sed '/^[[:punct:]]/d'`
  if [ -z $flag ] ; then continue ; fi
  b=`echo $a | sed 's/\.c/\.o/'`
  cpp -M $a | sed 's/\\//g' | sed 's/[[:space:]]\+/ /g' | sed 's/^[[:space:]]//' | sed 's/[[:space:]]$//' | sed 's/[[:space:]]/\n/g' | grep -vE '\.o' | grep -vE "$a" | sed "s/^/$b: /" >> $file
done

