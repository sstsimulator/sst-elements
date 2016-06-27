#!/sbin/ash
/sbin/mark_app
for i in `seq 1 $NCPUS`; do ./test-app & done
wait
/sbin/mark_app END
