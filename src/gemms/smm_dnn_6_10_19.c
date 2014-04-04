#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec( target (mic))
void smm_dnn_6_10_19(double* a,double* b,double* c){
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
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],255);
 xb1 = _MM512_MASK_LOADU_PD(&b[10+0],255);
 xb2 = _MM512_MASK_LOADU_PD(&b[20+0],255);
 xb3 = _MM512_MASK_LOADU_PD(&b[30+0],255);
 xb4 = _MM512_MASK_LOADU_PD(&b[40+0],255);
 xb5 = _MM512_MASK_LOADU_PD(&b[50+0],255);
 xb6 = _MM512_MASK_LOADU_PD(&b[60+0],255);
 xb7 = _MM512_MASK_LOADU_PD(&b[70+0],255);
 xb8 = _MM512_MASK_LOADU_PD(&b[80+0],255);
 xb9 = _MM512_MASK_LOADU_PD(&b[90+0],255);
 xb10 = _MM512_MASK_LOADU_PD(&b[100+0],255);
 xb11 = _MM512_MASK_LOADU_PD(&b[110+0],255);
 xb12 = _MM512_MASK_LOADU_PD(&b[120+0],255);
 xb13 = _MM512_MASK_LOADU_PD(&b[130+0],255);
 xb14 = _MM512_MASK_LOADU_PD(&b[140+0],255);
 xb15 = _MM512_MASK_LOADU_PD(&b[150+0],255);
 xb16 = _MM512_MASK_LOADU_PD(&b[160+0],255);
 xb17 = _MM512_MASK_LOADU_PD(&b[170+0],255);
 xb18 = _MM512_MASK_LOADU_PD(&b[180+0],255);
for(i=0;i<6;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*10+0],255);
    xa0=_mm512_set1_pd(a[i*19+0]);
    xa1=_mm512_set1_pd(a[i*19+1]);
    xa2=_mm512_set1_pd(a[i*19+2]);
    xa3=_mm512_set1_pd(a[i*19+3]);
    xa4=_mm512_set1_pd(a[i*19+4]);
    xa5=_mm512_set1_pd(a[i*19+5]);
    xa6=_mm512_set1_pd(a[i*19+6]);
    xa7=_mm512_set1_pd(a[i*19+7]);
    xa8=_mm512_set1_pd(a[i*19+8]);
    xa9=_mm512_set1_pd(a[i*19+9]);
    xa10=_mm512_set1_pd(a[i*19+10]);
    xa11=_mm512_set1_pd(a[i*19+11]);
    xa12=_mm512_set1_pd(a[i*19+12]);
    xa13=_mm512_set1_pd(a[i*19+13]);
    xa14=_mm512_set1_pd(a[i*19+14]);
    xa15=_mm512_set1_pd(a[i*19+15]);
    xa16=_mm512_set1_pd(a[i*19+16]);
    xa17=_mm512_set1_pd(a[i*19+17]);
    xa18=_mm512_set1_pd(a[i*19+18]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa7,xb7,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa8,xb8,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa9,xb9,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa10,xb10,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa11,xb11,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa12,xb12,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa13,xb13,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa14,xb14,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa15,xb15,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa16,xb16,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa17,xb17,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa18,xb18,xc0,255);
    _MM512_MASK_STOREU_PD(&c[i*10+0],xc0,255);
}
 xb0 = _MM512_MASK_LOADU_PD(&b[0+8],3);
 xb1 = _MM512_MASK_LOADU_PD(&b[10+8],3);
 xb2 = _MM512_MASK_LOADU_PD(&b[20+8],3);
 xb3 = _MM512_MASK_LOADU_PD(&b[30+8],3);
 xb4 = _MM512_MASK_LOADU_PD(&b[40+8],3);
 xb5 = _MM512_MASK_LOADU_PD(&b[50+8],3);
 xb6 = _MM512_MASK_LOADU_PD(&b[60+8],3);
 xb7 = _MM512_MASK_LOADU_PD(&b[70+8],3);
 xb8 = _MM512_MASK_LOADU_PD(&b[80+8],3);
 xb9 = _MM512_MASK_LOADU_PD(&b[90+8],3);
 xb10 = _MM512_MASK_LOADU_PD(&b[100+8],3);
 xb11 = _MM512_MASK_LOADU_PD(&b[110+8],3);
 xb12 = _MM512_MASK_LOADU_PD(&b[120+8],3);
 xb13 = _MM512_MASK_LOADU_PD(&b[130+8],3);
 xb14 = _MM512_MASK_LOADU_PD(&b[140+8],3);
 xb15 = _MM512_MASK_LOADU_PD(&b[150+8],3);
 xb16 = _MM512_MASK_LOADU_PD(&b[160+8],3);
 xb17 = _MM512_MASK_LOADU_PD(&b[170+8],3);
 xb18 = _MM512_MASK_LOADU_PD(&b[180+8],3);
for(i=0;i<6;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*10+8],3);
    xa0=_mm512_set1_pd(a[i*19+0]);
    xa1=_mm512_set1_pd(a[i*19+1]);
    xa2=_mm512_set1_pd(a[i*19+2]);
    xa3=_mm512_set1_pd(a[i*19+3]);
    xa4=_mm512_set1_pd(a[i*19+4]);
    xa5=_mm512_set1_pd(a[i*19+5]);
    xa6=_mm512_set1_pd(a[i*19+6]);
    xa7=_mm512_set1_pd(a[i*19+7]);
    xa8=_mm512_set1_pd(a[i*19+8]);
    xa9=_mm512_set1_pd(a[i*19+9]);
    xa10=_mm512_set1_pd(a[i*19+10]);
    xa11=_mm512_set1_pd(a[i*19+11]);
    xa12=_mm512_set1_pd(a[i*19+12]);
    xa13=_mm512_set1_pd(a[i*19+13]);
    xa14=_mm512_set1_pd(a[i*19+14]);
    xa15=_mm512_set1_pd(a[i*19+15]);
    xa16=_mm512_set1_pd(a[i*19+16]);
    xa17=_mm512_set1_pd(a[i*19+17]);
    xa18=_mm512_set1_pd(a[i*19+18]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa7,xb7,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa8,xb8,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa9,xb9,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa10,xb10,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa11,xb11,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa12,xb12,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa13,xb13,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa14,xb14,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa15,xb15,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa16,xb16,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa17,xb17,xc0,3);
    xc0=_mm512_mask3_fmadd_pd(xa18,xb18,xc0,3);
    _MM512_MASK_STOREU_PD(&c[i*10+8],xc0,3);
}
#else
printf("cppgemm_2_6_19_10\n");
for(int m=0;m<6;m++){
   for(int n=0;n<10;n++){
      for(int k=0;k<19;k++){
         c[m*10+n]+=a[m*19+k]*b[k*10+n];
      }
   }
}
#endif
}
 
