  set view map;
  set term x11;
  set xlabel "X (meter)"
  set ylabel "Y (meter)"
  set cblabel "SINR (dB)"
  plot "lena-simple-epc.rem" using ($1):($2):(10*log10($4)) with image notitle
  set terminal pngcairo
  #set term postscript eps enhanced "Arial" 24
  #set term postscript eps color blacktext "Arial" 14
  #set output 'lena-simple-epc.eps'
  set output 'lena-simple-epc.png'
  replot
