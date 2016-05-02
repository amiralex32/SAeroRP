set terminal postscript portrait enhanced lw 2 "Helvetica" 14

set size 1.0, 0.66

#-------------------------------------------------------
set out "aerorp-pdr.eps"
#set title "Packet Delivered Ratio"
set xlabel "No Of Nodes"
set xrange [10:100]
set ylabel "% Packet Delivered Ratio --- average of 10 trials"
set yrange [0:110]

plot "AeroRPPDR.data" with lines title "aerorp" , "SAeroRPPDR.data" with lines title "secure aerorp"
