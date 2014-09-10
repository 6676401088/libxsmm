#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec(target(mic))
void smm_dnn_7_14_7(const double* a, const double* b, double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xa1;
__m512d xa2;
__m512d xa3;
__m512d xa4;
__m512d xa5;
__m512d xa6;
__m512d xb0;
__m512d xb1;
__m512d xb2;
__m512d xb3;
__m512d xb4;
__m512d xb5;
__m512d xb6;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],255);
 xb1 = _MM512_MASK_LOADU_PD(&b[14+0],255);
 xb2 = _MM512_MASK_LOADU_PD(&b[28+0],255);
 xb3 = _MM512_MASK_LOADU_PD(&b[42+0],255);
 xb4 = _MM512_MASK_LOADU_PD(&b[56+0],255);
 xb5 = _MM512_MASK_LOADU_PD(&b[70+0],255);
 xb6 = _MM512_MASK_LOADU_PD(&b[84+0],255);
for(i=0;i<7;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*14+0],255);
    xa0=_mm512_set1_pd(a[i*7+0]);
    xa1=_mm512_set1_pd(a[i*7+1]);
    xa2=_mm512_set1_pd(a[i*7+2]);
    xa3=_mm512_set1_pd(a[i*7+3]);
    xa4=_mm512_set1_pd(a[i*7+4]);
    xa5=_mm512_set1_pd(a[i*7+5]);
    xa6=_mm512_set1_pd(a[i*7+6]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,255);
    _MM512_MASK_STOREU_PD(&c[i*14+0],xc0,255);
}
 xb0 = _MM512_MASK_LOADU_PD(&b[0+8],63);
 xb1 = _MM512_MASK_LOADU_PD(&b[14+8],63);
 xb2 = _MM512_MASK_LOADU_PD(&b[28+8],63);
 xb3 = _MM512_MASK_LOADU_PD(&b[42+8],63);
 xb4 = _MM512_MASK_LOADU_PD(&b[56+8],63);
 xb5 = _MM512_MASK_LOADU_PD(&b[70+8],63);
 xb6 = _MM512_MASK_LOADU_PD(&b[84+8],63);
for(i=0;i<7;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*14+8],63);
    xa0=_mm512_set1_pd(a[i*7+0]);
    xa1=_mm512_set1_pd(a[i*7+1]);
    xa2=_mm512_set1_pd(a[i*7+2]);
    xa3=_mm512_set1_pd(a[i*7+3]);
    xa4=_mm512_set1_pd(a[i*7+4]);
    xa5=_mm512_set1_pd(a[i*7+5]);
    xa6=_mm512_set1_pd(a[i*7+6]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,63);
    _MM512_MASK_STOREU_PD(&c[i*14+8],xc0,63);
}
#else
printf("cppgemm_2_7_7_14\n");
for(int m=0;m<7;m++){
   for(int n=0;n<14;n++){
      for(int k=0;k<7;k++){
         c[m*14+n]+=a[m*7+k]*b[k*14+n];
      }
   }
}
#endif
}
 
