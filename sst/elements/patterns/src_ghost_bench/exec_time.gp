load "header.gp"

set format y "%.1s %cs"
set ylabel "Execution time"
set xlabel "Number of nodes"

set key top right reverse invert

unset y2tics
set yrange [0:]

set output "exec_time_3D.eps"
set title "Execution time 3-D"
plot  "Redsky3D.out" \
	    using 1:3 title "Red Sky" with linespoints ls 1, \
	 "" using 1:3:2:4 notitle with errorbars ls 31

set xtics (\
		""		8, \
		""		16, \
		""		32, \
		""		64, \
		""		128, \
		"256"		256, \
		"512"		512, \
		"768"		768, \
		"1,024"		1024, \
		"1,280"		1280, \
		"1,536"		1536, \
		"1,792"		1792, \
		"2,048"		2048, \
		"2,304"		2304, \
		"2,560"		2560, \
		"2,816"		2816, \
		"3,072"		3072, \
		"3,328"		3328, \
		"3,584"		3584, \
		"3,840"		3840, \
		"4,096"		4096)

set output "exec_time_2D.eps"
set title "Execution time 2-D"
plot  "Redsky2D.out" \
	    using 1:3 title "Red Sky" with linespoints ls 1, \
	 "" using 1:3:2:4 notitle with errorbars ls 31, \
      "Glory2D.out" \
	    using 1:3 title "Glory" with linespoints ls 2, \
	 "" using 1:3:2:4 notitle with errorbars ls 31



set ylabel "Cumulative compute time"
set output "comp_time_2D.eps"
set title "Compute time 2-D"
plot  "Redsky2D.out" \
	    using 1:8 title "Red Sky" with linespoints ls 1, \
	 "" using 1:8:7:9 notitle with errorbars ls 31, \
      "Glory2D.out" \
	    using 1:8 title "Glory" with linespoints ls 2, \
	 "" using 1:8:7:9 notitle with errorbars ls 31



set ylabel "Cumulative communication time"
set output "comm_time_2D.eps"
set title "Communication time 2-D"
plot  "Redsky2D.out" \
	    using 1:13 title "Red Sky" with linespoints ls 1, \
	 "" using 1:13:12:14 notitle with errorbars ls 31, \
      "Glory2D.out" \
	    using 1:13 title "Glory" with linespoints ls 2, \
	 "" using 1:13:12:14 notitle with errorbars ls 31



set format y "%.6f"
set ylabel "Ratio"
set output "border_2D.eps"
set title "Border to area ratio 2-D"
plot  "Redsky2D.out" \
	    using 1:27 notitle with lines ls 1



set format y "%.3f"
set ylabel "X/Y ratio"
set output "xy_ratio_2D.eps"
set title "X to Y ratio 2-D"
plot  "Redsky2D.out" \
	    using 1:($28 / $29) notitle with lines ls 1



set format y "%.3f"
set format y2 "%.1s %cs"
set yrange [-1:]
set y2range [0:20]
set ylabel "X/Y ratio"
set y2label "Execution time"
set output "xy_ratio_exec_2D.eps"
set title "X to Y ratio and execution time 2-D"
set key left noinvert
set y2tics
plot  "Redsky2D.out" \
	    using 1:($28 / $29) title "X/Y ratio" with linespoints ls 1, \
	 "" using 1:3 axes x1y2 title "Red Sky exec time" with lines ls 2
unset y2tics



set format y "%.1s %cB"
set ylabel "Bytes"
set output "bytes_sent.eps"
set title "Total amount of data sent 2-D"
set key left
plot  "Redsky2D.out" \
	    using 1:($17 * 1024 * 1024) title "Red Sky" with lines ls 1



set format y "%.1s %cF"
set ylabel "Flops"
set output "flop_2D.eps"
set title "Total floating point operations 2-D"
set key left invert
plot  "Redsky2D.out" \
	    using 1:20 title "Red Sky" with lines ls 1, \
      "Glory2D.out" \
	    using 1:20 title "Glory" with lines ls 2



set format y "%.1s %cF/s"
set ylabel "Flops per second"
set output "flops_2D.eps"
set title "Floating point operations per second 2-D"
set key left noinvert
set xrange [0:1024]
plot  "Redsky2D.out" \
	    using 1:22 title "Red Sky" with lines ls 1, \
	 "" using 1:22:21:23 notitle with errorbars ls 31, \
      "Glory2D.out" \
	    using 1:22 title "Glory" with lines ls 2, \
	 "" using 1:22:21:23 notitle with errorbars ls 31
