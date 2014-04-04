#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec(target(mic))
void smm_dnn_8_8_5(const double* a, const double* b, double* c){
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
 xb1 = _MM512_MASK_LOADU_PD(&b[8+0],255);
 xb2 = _MM512_MASK_LOADU_PD(&b[16+0],255);
 xb3 = _MM512_MASK_LOADU_PD(&b[24+0],255);
 xb4 = _MM512_MASK_LOADU_PD(&b[32+0],255);
for(i=0;i<8;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*8+0],255);
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
    _MM512_MASK_STOREU_PD(&c[i*8+0],xc0,255);
}
#else
printf("cppgemm_2_8_5_8\n");
for(int m=0;m<8;m++){
   for(int n=0;n<8;n++){
      for(int k=0;k<5;k++){
         c[m*8+n]+=a[m*5+k]*b[k*8+n];
      }
   }
}
#endif
}
 
