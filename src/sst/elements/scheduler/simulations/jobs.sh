#while :
#do

echo Running
qstat | grep " r " | wc -l;
echo Pending
qstat | grep " qw " | wc -l
echo Suspended
qstat | grep " S " | wc -l
echo Rerunning
qstat | grep " Rr " | wc -l
echo Error waiting
qstat | grep " Eqw " | wc -l
#sleep 10 

#echo
#echo

#done


