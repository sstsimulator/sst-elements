set terminal postscript eps enhanced color solid "Helvetica" 28
set size 1.5,2.0
set grid


#
set output "ws_count.eps"
set xlabel "Destination rank"
set ylabel "Source rank"
set nokey
# set xtics 0,4,64-1
# set ytics 0,4,64-1
set cblabel "messages" # offset -1,0
set pm3d map flush begin
set pm3d corners2color c1
set palette gray  negative
splot "ws_count.dat" matrix



set format cb "%.0s %cB"
set output "ws_size.eps"
set cblabel "amount of data" # offset -1,0
splot "ws_size.dat" matrix
