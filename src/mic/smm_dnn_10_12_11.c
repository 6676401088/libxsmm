#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec(target(mic))
void smm_dnn_10_12_11(const double* a, const double* b, double* c){
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
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],255);
 xb1 = _MM512_MASK_LOADU_PD(&b[12+0],255);
 xb2 = _MM512_MASK_LOADU_PD(&b[24+0],255);
 xb3 = _MM512_MASK_LOADU_PD(&b[36+0],255);
 xb4 = _MM512_MASK_LOADU_PD(&b[48+0],255);
 xb5 = _MM512_MASK_LOADU_PD(&b[60+0],255);
 xb6 = _MM512_MASK_LOADU_PD(&b[72+0],255);
 xb7 = _MM512_MASK_LOADU_PD(&b[84+0],255);
 xb8 = _MM512_MASK_LOADU_PD(&b[96+0],255);
 xb9 = _MM512_MASK_LOADU_PD(&b[108+0],255);
 xb10 = _MM512_MASK_LOADU_PD(&b[120+0],255);
for(i=0;i<10;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*12+0],255);
    xa0=_mm512_set1_pd(a[i*11+0]);
    xa1=_mm512_set1_pd(a[i*11+1]);
    xa2=_mm512_set1_pd(a[i*11+2]);
    xa3=_mm512_set1_pd(a[i*11+3]);
    xa4=_mm512_set1_pd(a[i*11+4]);
    xa5=_mm512_set1_pd(a[i*11+5]);
    xa6=_mm512_set1_pd(a[i*11+6]);
    xa7=_mm512_set1_pd(a[i*11+7]);
    xa8=_mm512_set1_pd(a[i*11+8]);
    xa9=_mm512_set1_pd(a[i*11+9]);
    xa10=_mm512_set1_pd(a[i*11+10]);
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
    _MM512_MASK_STOREU_PD(&c[i*12+0],xc0,255);
}
 xb0 = _MM512_MASK_LOADU_PD(&b[0+8],15);
 xb1 = _MM512_MASK_LOADU_PD(&b[12+8],15);
 xb2 = _MM512_MASK_LOADU_PD(&b[24+8],15);
 xb3 = _MM512_MASK_LOADU_PD(&b[36+8],15);
 xb4 = _MM512_MASK_LOADU_PD(&b[48+8],15);
 xb5 = _MM512_MASK_LOADU_PD(&b[60+8],15);
 xb6 = _MM512_MASK_LOADU_PD(&b[72+8],15);
 xb7 = _MM512_MASK_LOADU_PD(&b[84+8],15);
 xb8 = _MM512_MASK_LOADU_PD(&b[96+8],15);
 xb9 = _MM512_MASK_LOADU_PD(&b[108+8],15);
 xb10 = _MM512_MASK_LOADU_PD(&b[120+8],15);
for(i=0;i<10;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*12+8],15);
    xa0=_mm512_set1_pd(a[i*11+0]);
    xa1=_mm512_set1_pd(a[i*11+1]);
    xa2=_mm512_set1_pd(a[i*11+2]);
    xa3=_mm512_set1_pd(a[i*11+3]);
    xa4=_mm512_set1_pd(a[i*11+4]);
    xa5=_mm512_set1_pd(a[i*11+5]);
    xa6=_mm512_set1_pd(a[i*11+6]);
    xa7=_mm512_set1_pd(a[i*11+7]);
    xa8=_mm512_set1_pd(a[i*11+8]);
    xa9=_mm512_set1_pd(a[i*11+9]);
    xa10=_mm512_set1_pd(a[i*11+10]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa7,xb7,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa8,xb8,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa9,xb9,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa10,xb10,xc0,15);
    _MM512_MASK_STOREU_PD(&c[i*12+8],xc0,15);
}
#else
printf("cppgemm_2_10_11_12\n");
for(int m=0;m<10;m++){
   for(int n=0;n<12;n++){
      for(int k=0;k<11;k++){
         c[m*12+n]+=a[m*11+k]*b[k*12+n];
      }
   }
}
#endif
}
 
