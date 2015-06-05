MPARM = 1
NPARM = 2
KPARM = 3
FLOPS = 8

HIM = -1
HIN = HIM
HIK = HIM
MN = 23
PEAK = 0 #985.6

BASENAME = "smm-test"
FILENAME = system("sh -c \"echo ${FILENAME}\"")
if (FILENAME eq "") {
  FILENAME = BASENAME.".pdf"
}

FILECOUNT = 1 # initial file number
# MULTI =-1: multiple files; no titles
# MULTI = 0: multiple files with titles
# MULTI = 1: single file with titles
MULTI = system("sh -c \"echo ${MULTI}\"")
if (MULTI eq "") {
  MULTI = 1
}

stats BASENAME.".dat" using (column(MPARM)*column(NPARM)*column(KPARM)) nooutput; MNK = STATS_stddev**(1.0/3.0); MAXMNK = STATS_max
stats BASENAME.".dat" using (log(column(FLOPS))) nooutput; GEO = sprintf("%.0f", exp(STATS_sum/STATS_records))
stats BASENAME.".dat" using FLOPS nooutput; MED = sprintf("%.0f", STATS_median)
stats BASENAME.".dat" using NPARM nooutput; XN = int(STATS_max)

MAX(A, B) = A < B ? B : A
IX(I1, J1, NJ) = int(MAX(I1 - 1, 0) * NJ + MAX(J1 - 1, 0))
I1(IX, NJ) = int(IX / NJ) + 1
J1(IX, NJ) = int(IX) % NJ + 1

set table BASENAME."-avg.dat"
plot BASENAME.".dat" using (IX(column(MPARM), column(NPARM), XN)):FLOPS smooth unique
unset table
set table BASENAME."-cdf.dat"
plot BASENAME.".dat" using FLOPS:(1.0) smooth cumulative
unset table
stats BASENAME."-cdf.dat" using (("".strcol(3)."" eq "i")?($2):(1/0)) nooutput; FREQSUM = STATS_max

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
set origin -0.03, 0
set pm3d interpolate 0, 0
#set colorbox horizontal user origin 0, 0.1 size 1, 0.1
set autoscale fix
if (0<HIM) { set xrange [*:HIM] }
if (0<HIN) { set yrange [*:HIN] }
if (0<HIK) { set zrange [*:HIK] }
if (0>HIM) { set xrange [*:MNK] }
if (0>HIN) { set yrange [*:MNK] }
if (0>HIK) { set zrange [*:MNK] }
set xlabel "M"
set ylabel "N" offset -3.0
set zlabel "K" offset 1.0
set ticslevel 0
set cblabel "GFLOP/s" offset 1.5
set format x "%g"; set format y "%g"; set format z "%g"; set format cb "%g"
splot BASENAME.".dat" using MPARM:NPARM:KPARM:FLOPS notitle with points pointtype 7 linetype palette

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
if (MULTI>-1) { set title "Performance (Binned by Problem Size)" }
set style fill solid 0.05 border -1
set boxwidth 0.5
set grid y2tics lc "grey"
unset key
unset xtics
set x2tics ("Small" 0, "Medium" 1, "Larger" 2) scale 0
set xrange [-0.5:2.5]
unset ytics
set y2tics nomirror
set y2label "GFLOP/s"
plot  BASENAME.".dat" using (0.0):((((0.0)<(column(MPARM)*column(NPARM)*column(KPARM)))&&((column(MPARM)*column(NPARM)*column(KPARM))<=(MAXMNK*1.0/3.0)))?column(FLOPS):1/0) notitle smooth unique with boxes, \
      BASENAME.".dat" using (1.0):((((MAXMNK*1.0/3.0)<(column(MPARM)*column(NPARM)*column(KPARM)))&&((column(MPARM)*column(NPARM)*column(KPARM))<=(MAXMNK*2.0/3.0)))?column(FLOPS):1/0) notitle smooth unique with boxes, \
      BASENAME.".dat" using (2.0):((((MAXMNK*2.0/3.0)<(column(MPARM)*column(NPARM)*column(KPARM)))&&((column(MPARM)*column(NPARM)*column(KPARM))<=(MAXMNK)))?column(FLOPS):1/0) notitle smooth unique with boxes

reset
if (MULTI<=0) { set output "".FILECOUNT."-".FILENAME; FILECOUNT = FILECOUNT + 1 }
if (MULTI>-1) {
  set title "Performance (CDF)"
  set xlabel "Probability\n\nGeo. Mean: ".GEO." GFLOP/s  Median: ".MED." GFLOP/s"
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
x = 0.5 * (100 + MAX(0, g(0)))
h(x) = d * x + c
fit [x*0.95:x*1.05] h(x) BASENAME."-cdf.dat" using (("".strcol(3)."" eq "i")?(100*$2/FREQSUM):(1/0)):1 via c, d
set arrow 1 from x, h(x) to x, 0
set label 1 sprintf("%.0f%%", x) at x, 0.5 * h(x) left offset 1
set arrow 2 from x, h(x) to 2 * x, h(x)
set label 2 sprintf("%.0f GFLOP/s", h(x)) at 1.5 * x, h(x) centre offset 0, -1
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
  plot BASENAME.".dat" using (floor((column(MPARM)*column(NPARM)*column(KPARM))**(1.0/3.0)+0.5)):(100.0*column(FLOPS)/PEAK) notitle smooth sbezier with points pointtype 7 pointsize 0.5
}
