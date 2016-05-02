set terminal postscript portrait enhanced lw 2 "Helvetica" 14

set size 1.0, 0.66

set output "AeroRP_overhead.eps"

set xrange [10:100]

set yrange [0:1.0]

set xlabel "NO OF NODES"

set ylabel "Packet overhead"

set grid

plot 'AeroRP.data' using 1:2 title 'AeroRP' with linespoints

