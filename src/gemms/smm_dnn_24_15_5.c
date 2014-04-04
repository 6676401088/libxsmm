#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec(target(mic))
void smm_dnn_24_15_5(const double* a, const double* b, double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xa1;
__m512d xa2;
__m512d xa3;
__m512d xa4;
__m512d xb0;
__m512d xb1;
__m512d xb2;
__m512d xb3;
__m512d xb4;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],255);
 xb1 = _MM512_MASK_LOADU_PD(&b[15+0],255);
 xb2 = _MM512_MASK_LOADU_PD(&b[30+0],255);
 xb3 = _MM512_MASK_LOADU_PD(&b[45+0],255);
 xb4 = _MM512_MASK_LOADU_PD(&b[60+0],255);
for(i=0;i<24;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*15+0],255);
    xa0=_mm512_set1_pd(a[i*5+0]);
    xa1=_mm512_set1_pd(a[i*5+1]);
    xa2=_mm512_set1_pd(a[i*5+2]);
    xa3=_mm512_set1_pd(a[i*5+3]);
    xa4=_mm512_set1_pd(a[i*5+4]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,255);
    _MM512_MASK_STOREU_PD(&c[i*15+0],xc0,255);
}
 xb0 = _MM512_MASK_LOADU_PD(&b[0+8],127);
 xb1 = _MM512_MASK_LOADU_PD(&b[15+8],127);
 xb2 = _MM512_MASK_LOADU_PD(&b[30+8],127);
 xb3 = _MM512_MASK_LOADU_PD(&b[45+8],127);
 xb4 = _MM512_MASK_LOADU_PD(&b[60+8],127);
for(i=0;i<24;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*15+8],127);
    xa0=_mm512_set1_pd(a[i*5+0]);
    xa1=_mm512_set1_pd(a[i*5+1]);
    xa2=_mm512_set1_pd(a[i*5+2]);
    xa3=_mm512_set1_pd(a[i*5+3]);
    xa4=_mm512_set1_pd(a[i*5+4]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,127);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,127);
    _MM512_MASK_STOREU_PD(&c[i*15+8],xc0,127);
}
#else
printf("cppgemm_2_24_5_15\n");
for(int m=0;m<24;m++){
   for(int n=0;n<15;n++){
      for(int k=0;k<5;k++){
         c[m*15+n]+=a[m*5+k]*b[k*15+n];
      }
   }
}
#endif
}
 
