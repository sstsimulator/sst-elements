load "header.gp"

set format y "%.1s"
set ylabel "Execution time (seconds)"
set xlabel "Number of nodes"

set key bottom left reverse
set yrange [0:]
unset y2tics

set output "exec_imbalanced.eps"
set title "Execution time 2-D"
plot  \
    "Redsky2Dimbalanced.out" \
	using 1:3 title "Red Sky imbalanced" with linespoints ls 2, \
    "Redsky2Dbalanced.out" \
	using 1:3 title "Red Sky balanced" with linespoints ls 1, \
    "Redsky2D.out" \
	using 1:3 title "Red Sky no delay" with linespoints ls 3
