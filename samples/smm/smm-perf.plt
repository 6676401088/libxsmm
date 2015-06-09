MPARM = 1
NPARM = 2
KPARM = 3
FLOPS = 7

HIM = -1
HIN = HIM
HIK = HIM
MN = 23
PEAK = 0 #985.6

BASENAME = "smm"
FILENAME = system("sh -c \"echo ${FILENAME}\"")
if (FILENAME eq "") {
  FILENAME = BASENAME."-perf.pdf"
}

FILECOUNT = 1 # initial file number
# MULTI =-1: multiple files; no titles
# MULTI = 0: multiple files with titles
# MULTI = 1: single file with titles
MULTI = system("sh -c \"echo ${MULTI}\"")
if (MULTI eq "") {
  MULTI = 1
}

stats BASENAME."-specialized.dat" using (column(MPARM)*column(NPARM)*column(KPARM)) nooutput; MNK = STATS_stddev**(1.0/3.0); MAXMNK = STATS_max
stats BASENAME."-specialized.dat" using (log(column(FLOPS))) nooutput; GEO = exp(STATS_sum/STATS_records)
stats BASENAME."-specialized.dat" using FLOPS nooutput; MED = STATS_median; MINFLOPS = STATS_min; MAXFLOPS = STATS_max
stats BASENAME."-specialized.dat" using NPARM nooutput; XN = int(STATS_max)

MAX(A, B) = A < B ? B : A
ACC = sprintf("%%.%if", ceil(MAX(1.0 / log10(MED) - 1.0, 0)))

IX(I1, J1, NJ) = int(MAX(I1 - 1, 0) * NJ + MAX(J1 - 1, 0))
I1(IX, NJ) = int(IX / NJ) + 1
J1(IX, NJ) = int(IX) % NJ + 1

set table BASENAME."-avg.dat"
plot BASENAME."-specialized.dat" using (IX(column(MPARM), column(NPARM), XN)):FLOPS smooth unique
unset table
set table BASENAME."-cdf.dat"
plot BASENAME."-specialized.dat" using FLOPS:(1.0) smooth cumulative
unset table
stats BASENAME."-cdf.dat" using (("".strcol(3)."" eq "i")?($2):(1/0)) nooutput; FREQSUM = STATS_max; FREQN = STATS_records

if (MULTI==1) {
  set output FILENAME
}

FILEEXT = system("sh -c \"echo ".FILENAME." | sed 's/.\\+\\.\\(.\\+\\)/\\1/'\"")
set terminal FILEEXT
set termoption enhanced
#set termoption font ",12"
save_encoding = GPVAL_ENCODING
set encoding utf8


reset
if (MULTI<=0) { set output "".FILECOUNT."-".FILENAME; FILECOUNT = FILECOUNT + 1 }
if (MULTI>-1) { set title "Performance" }
set style fill solid 0.4 border -1
set style data histograms
set style histogram cluster #gap 2
#set boxwidth 0.5 relative
set grid y2tics lc "grey"
set key left #spacing 0.5
set xtics rotate by -45 scale 0; set bmargin 6
set ytics format ""
set y2tics nomirror
set y2label "GFLOP/s"
set yrange [0:*]
plot  BASENAME."-inlined.dat" using FLOPS title "Inlined", \
      BASENAME."-blas.dat" using FLOPS title "BLAS", \
      BASENAME."-dispatched.dat" using FLOPS title "Dispatched", \
      BASENAME."-specialized.dat" using FLOPS:xtic("(".strcol(MPARM).",".strcol(NPARM).",".strcol(KPARM).")") title "Specialized"

reset
if (MULTI<=0) { set output "".FILECOUNT."-".FILENAME; FILECOUNT = FILECOUNT + 1 }
if (MULTI>-1) { set title "Performance (K-Average)" }
set origin -0.02, 0
set dgrid3d #9, 9
set pm3d interpolate 0, 0 map
set autoscale fix
set xlabel "M"
set ylabel "N" offset -1.5
set cblabel "GFLOP/s" offset 0.5
set format x "%g"; set format y "%g"; set format cb "%g"
set mxtics 2
splot BASENAME."-avg.dat" using (("".strcol(3)."" eq "i")?(I1($1, XN)):(1/0)):(("".strcol(3)."" eq "i")?(J1($1, XN)):(1/0)):2 notitle with pm3d

reset
if (MULTI<=0) { set output "".FILECOUNT."-".FILENAME; FILECOUNT = FILECOUNT + 1 }
if (MULTI>-1) { set title "Performance (Average per Bin of Problem Size)" }
set style fill solid 0.4 border -1
set boxwidth 0.5
set grid y2tics lc "grey"
unset key
unset xtics
set x2tics ("Small" 0, "Medium" 1, "Larger" 2) scale 0
set xrange [-0.5:2.5]
set ytics format ""
set y2tics nomirror
set y2label "GFLOP/s"
plot  BASENAME."-specialized.dat" \
      using (0.0):((((0.0)<(column(MPARM)*column(NPARM)*column(KPARM)))&&((column(MPARM)*column(NPARM)*column(KPARM))<=(13*13*13)))?column(FLOPS):1/0) notitle smooth unique with boxes, \
  ""  using (1.0):((((13*13*13)<(column(MPARM)*column(NPARM)*column(KPARM)))&&((column(MPARM)*column(NPARM)*column(KPARM))<=(23*23*23)))?column(FLOPS):1/0) notitle smooth unique with boxes, \
  ""  using (2.0):((((23*23*23)<(column(MPARM)*column(NPARM)*column(KPARM)))&&((column(MPARM)*column(NPARM)*column(KPARM))<=(MAXMNK)))?column(FLOPS):1/0) notitle smooth unique with boxes

reset
if (MULTI<=0) { set output "".FILECOUNT."-".FILENAME; FILECOUNT = FILECOUNT + 1 }
if (MULTI>-1) {
  set title "Performance (CDF)"
  set xlabel "Probability\n\nMin.: ".sprintf(ACC, MINFLOPS)." GFLOP/s  Geo. Mean: ".sprintf(ACC, GEO)." GFLOP/s  Median: ".sprintf(ACC, MED)." GFLOP/s  Max.: ".sprintf(ACC, MAXFLOPS)." GFLOP/s"
} else {
  set xlabel "Probability"
}
set y2label "GFLOP/s"
set y2tics nomirror
unset ytics
set format x "%g%%"
set format y "%g"
set fit quiet
f(x) = b * x + a
fit f(x) BASENAME."-cdf.dat" using (("".strcol(3)."" eq "i")?(100*$2/FREQSUM):(1/0)):1 via a, b
g(x) = (x - a) / b
x50 = 0.5 * (100 + MAX(0, g(0)))
h(x) = d * x + c
dx = 100 / FREQN
fit [x50-1.2*dx:x50+1.2*dx] h(x) BASENAME."-cdf.dat" using (("".strcol(3)."" eq "i")?(100*$2/FREQSUM):(1/0)):1 via c, d
set arrow 1 from x50, h(x50) to x50, 0
set label 1 sprintf("%.0f%%", x50) at x50, 0.5 * h(x50) left offset 1
set arrow 2 from x50, h(x50) to 100, h(x50)
set label 2 sprintf(ACC." GFLOP/s", h(x50)) at 0.5 * (x50 + 100.0), h(x50) centre offset 0, -1
set autoscale fix
set xrange [0:100]
set yrange [0:*]
plot BASENAME."-cdf.dat" using (("".strcol(3)."" eq "i")?(100*$2/FREQSUM):(1/0)):1 notitle with lines

if (0 < PEAK) {
  reset
  if (MULTI<=0) { set output "".FILECOUNT."-".FILENAME; FILECOUNT = FILECOUNT + 1 }
  if (MULTI>-1) { set title "Performance and Efficiency" }
  set xlabel "(M N K)^{-1/3}"
  set ylabel "Efficiency"
  set y2label "GFLOP/s"
  set y2tics
  set ytics nomirror
  set format x "%g"; set format y "%g%%"
  set mxtics 2
  set mytics 2
  set my2tics 2
  set autoscale fix
  set yrange [0:100]
  set y2range [0:PEAK]
  plot BASENAME."-specialized.dat" using (floor((column(MPARM)*column(NPARM)*column(KPARM))**(1.0/3.0)+0.5)):(100.0*column(FLOPS)/PEAK) notitle smooth sbezier with points pointtype 7 pointsize 0.5
}
