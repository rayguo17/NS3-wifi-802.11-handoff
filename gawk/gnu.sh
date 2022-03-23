#!/usr/bin/gnuplot -persist
set title "throughput"
set terminal png size 640,480
set xlabel "time/seconds"
set ylabel "throughput/kbits"
set output "test.png"
plot "myThroughput" using 1:2 title "throughput" with linespoints pointtype 1
