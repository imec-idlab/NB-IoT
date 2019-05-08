  set view map;
  set term x11;
  set xlabel "X"
  set ylabel "Y"
  set cblabel "SINR (dB)"
  set title "SINR color map"
  plot "lena-dual-stripe.rem" using ($1):($2):(10*log10($4)) with image
  set terminal pngcairo
  #set term postscript eps enhanced "Arial" 24
  #set term postscript eps color blacktext "Arial" 14
  #set output 'lena-dual-stripe.eps'
  set output 'lena-dual-stripe.png'
  replot
