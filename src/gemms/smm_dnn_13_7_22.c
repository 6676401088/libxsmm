#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec( target (mic))
void smm_dnn_13_7_22(double* a,double* b,double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xa1;
__m512d xa2;
__m512d xa3;
__m512d xa4;
__m512d xa5;
__m512d xa6;
__m512d xa7;
__m512d xa8;
__m512d xa9;
__m512d xa10;
__m512d xa11;
__m512d xa12;
__m512d xa13;
__m512d xa14;
__m512d xa15;
__m512d xa16;
__m512d xa17;
__m512d xa18;
__m512d xa19;
__m512d xa20;
__m512d xa21;
__m512d xb0;
__m512d xb1;
__m512d xb2;
__m512d xb3;
__m512d xb4;
__m512d xb5;
__m512d xb6;
__m512d xb7;
__m512d xb8;
__m512d xb9;
__m512d xb10;
__m512d xb11;
__m512d xb12;
__m512d xb13;
__m512d xb14;
__m512d xb15;
__m512d xb16;
__m512d xb17;
__m512d xb18;
__m512d xb19;
__m512d xb20;
__m512d xb21;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],127);
 xb1 = _MM512_MASK_LOADU_PD(&b[7+0],127);
 xb2 = _MM512_MASK_LOADU_PD(&b[14+0],127);
 xb3 = _MM512_MASK_LOADU_PD(&b[21+0],127);
 xb4 = _MM512_MASK_LOADU_PD(&b[28+0],127);
 xb5 = _MM512_MASK_LOADU_PD(&b[35+0],127);
 xb6 = _MM512_MASK_LOADU_PD(&b[42+0],127);
 xb7 = _MM512_MASK_LOADU_PD(&b[49+0],127);
 xb8 = _MM512_MASK_LOADU_PD(&b[56+0],127);
 xb9 = _MM512_MASK_LOADU_PD(&b[63+0],127);
 xb10 = _MM512_MASK_LOADU_PD(&b[70+0],127);
 xb11 = _MM512_MASK_LOADU_PD(&b[77+0],127);
 xb12 = _MM512_MASK_LOADU_PD(&b[84+0],127);
 xb13 = _MM512_MASK_LOADU_PD(&b[91+0],127);
 xb14 = _MM512_MASK_LOADU_PD(&b[98+0],127);
 xb15 = _MM512_MASK_LOADU_PD(&b[105+0],127);
 xb16 = _MM512_MASK_LOADU_PD(&b[112+0],127);
 xb17 = _MM512_MASK_LOADU_PD(&b[119+0],127);
 xb18 = _MM512_MASK_LOADU_PD(&b[126+0],127);
 xb19 = _MM512_MASK_LOADU_PD(&b[133+0],127);
 xb20 = _MM512_MASK_LOADU_PD(&b[140+0],127);
 xb21 = _MM512_MASK_LOADU_PD(&b[147+0],127);
for(i=0;i<13;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*7+0],127);
    xa0=_mm512_set1_pd(a[i*22+0]);
    xa1=_mm512_set1_pd(a[i*22+1]);
    xa2=_mm512_set1_pd(a[i*22+2]);
    xa3=_mm512_set1_pd(a[i*22+3]);
    xa4=_mm512_set1_pd(a[i*22+4]);
    xa5=_mm512_set1_pd(a[i*22+5]);
    xa6=_mm512_set1_pd(a[i*22+6]);
    xa7=_mm512_set1_pd(a[i*22+7]);
    xa8=_mm512_set1_pd(a[i*22+8]);
    xa9=_mm512_set1_pd(a[i*22+9]);
    xa10=_mm512_set1_pd(a[i*22+10]);
    xa11=_mm512_set1_pd(a[i*22+11]);
    xa12=_mm512_set1_pd(a[i*22+12]);
    xa13=_mm512_set1_pd(a[i*22+13]);
    xa14=_mm512_set1_pd(a[i*22+14]);
    xa15=_mm512_set1_pd(a[i*22+15]);
    xa16=_mm512_set1_pd(a[i*22+16]);
    xa17=_mm512_set1_pd(a[i*22+17]);
    xa18=_mm512_set1_pd(a[i*22+18]);
    xa19=_mm512_set1_pd(a[i*22+19]);
    xa20=_mm512_set1_pd(a[i*22+20]);
    xa21=_mm512_set1_pd(a[i*22+21]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa7,xb7,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa8,xb8,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa9,xb9,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa10,xb10,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa11,xb11,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa12,xb12,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa13,xb13,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa14,xb14,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa15,xb15,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa16,xb16,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa17,xb17,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa18,xb18,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa19,xb19,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa20,xb20,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa21,xb21,xc0,127);
    _MM512_MASK_STOREU_PD(&c[i*7+0],xc0,127);
}
#else
printf("cppgemm_2_13_22_7\n");
for(int m=0;m<13;m++){
   for(int n=0;n<7;n++){
      for(int k=0;k<22;k++){
         c[m*7+n]+=a[m*22+k]*b[k*7+n];
      }
   }
}
#endif
}
 
