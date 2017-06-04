set term png size 1280,960
set output 'tmp/rs.png'
set title "RS neuron"
plot 'neurons.dat' using 1:2 title 'float' with lines, 'neurons.dat' using 1:3 title 'fixed' with lines

set output 'tmp/ib.png'
set title "IB neuron"
plot 'neurons.dat' using 1:4 title 'float' with lines, 'neurons.dat' using 1:5 title 'fixed' with lines

set output 'tmp/ch.png'
set title "CH neuron"
plot 'neurons.dat' using 1:6 title 'float' with lines, 'neurons.dat' using 1:7 title 'fixed' with lines

set output 'tmp/fs.png'
set title "FS neuron"
plot 'neurons.dat' using 1:8 title 'float' with lines, 'neurons.dat' using 1:9 title 'fixed' with lines

set output 'tmp/lts.png'
set title "LTS neuron"
plot 'neurons.dat' using 1:10 title 'float' with lines, 'neurons.dat' using 1:11 title 'fixed' with lines

set output 'tmp/rz.png'
set title "RZ neuron"
plot 'neurons.dat' using 1:12 title 'float' with lines, 'neurons.dat' using 1:13 title 'fixed' with lines

set output 'tmp/tc.png'
set title "TC neuron"
plot 'neurons.dat' using 1:14 title 'float' with lines, 'neurons.dat' using 1:15 title 'fixed' with lines
