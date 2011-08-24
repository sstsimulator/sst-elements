#
# Common parameters for all of our plots.
#
set terminal postscript eps enhanced color colortext \
	dashed dashlength 4.0 linewidth 0.7 butt \
	"Helvetica" 48
# set size 2.5,1.5
set size 2.5,2.0
set rmargin 6	# Make room for the xtick label
set grid front
set grid y

# The 3 at the end says draw the border on the left and bottom only
set border lc rgbcolor "grey10" 3

# Don't put ticks on the border we just erased
set ytics nomirror
set xtics nomirror rotate by -45 scale 0

set tics out textcolor rgbcolor "#546319"


set grid lc rgbcolor "#606060"
set title tc rgbcolor "#546319"
set ylabel tc rgbcolor "#546319"
set y2label tc rgbcolor "#546319"
set y2tics tc rgbcolor "#546319"
set xlabel tc rgbcolor "#546319"

# pt 7 is a filled circle
# pt 9 is a filled triangle
# pt 13 is a filled diamond
# pt 11 is a filled upside-down triangle
# pt 15 is a filled pentagon
set style line 1 lc rgbcolor "#64751e" ps 2 pt 15 lw 10 lt 1
set style line 2 lc rgbcolor "#a5c232" ps 2 pt 7 lw 10 lt 1
set style line 3 lc rgbcolor "#754d1e" ps 2 pt 9 lw 10 lt 1
set style line 4 lc rgbcolor "#c27f32" ps 2 pt 11 lw 10 lt 1
set style line 5 lc rgbcolor "#8a8a8a" ps 2 pt 12 lw 10 lt 1
set style line 9 lc rgbcolor "#cccccc" ps 2 pt 15 lw 25 lt 1
set style line 10 lc rgbcolor "#C23838" ps 2 pt 15 lw 5 lt 1


# Dashed versions, no points
set style line 11 lc rgbcolor "#64751e" ps 0 pt 15 lw 10 lt 2
set style line 12 lc rgbcolor "#a5c232" ps 0 pt 7 lw 10 lt 2
set style line 13 lc rgbcolor "#754d1e" ps 0 pt 9 lw 10 lt 2
set style line 14 lc rgbcolor "#c27f32" ps 0 pt 11 lw 10 lt 2
set style line 15 lc rgbcolor "#8a8a8a" ps 0 pt 12 lw 10 lt 2
set style line 19 lc rgbcolor "#cccccc" ps 0 pt 15 lw 25 lt 2
set style line 20 lc rgbcolor "#C23838" ps 0 pt 15 lw 5 lt 2


# For errorbars
set style line 31 lc rgbcolor "#000000" ps 0 pt 15 lw 4 lt 1
set style line 32 lc rgbcolor "#C23838" ps 0 pt 15 lw 4 lt 1
set style line 33 lc rgbcolor "#000000" ps 0 pt 15 lw 4 lt 2


set datafile missing "-"


set xtics (\
		""		8, \
		""		16, \
		""		24, \
		"32"		32, \
		"64"		64, \
		"96"		96, \
		"128"		128, \
		"160"		160, \
		"192"		192, \
		"224"		224, \
		"256"		256, \
		"288"		288, \
		"320"		320, \
		"352"		352, \
		"384"		384, \
		"416"		416, \
		"448"		448, \
		"480"		480, \
		"512"		512)
