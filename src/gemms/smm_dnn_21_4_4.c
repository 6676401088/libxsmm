#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec( target (mic))
void smm_dnn_21_4_4(double* a,double* b,double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xa1;
__m512d xa2;
__m512d xa3;
__m512d xb0;
__m512d xb1;
__m512d xb2;
__m512d xb3;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],15);
 xb1 = _MM512_MASK_LOADU_PD(&b[4+0],15);
 xb2 = _MM512_MASK_LOADU_PD(&b[8+0],15);
 xb3 = _MM512_MASK_LOADU_PD(&b[12+0],15);
for(i=0;i<21;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*4+0],15);
    xa0=_mm512_set1_pd(a[i*4+0]);
    xa1=_mm512_set1_pd(a[i*4+1]);
    xa2=_mm512_set1_pd(a[i*4+2]);
    xa3=_mm512_set1_pd(a[i*4+3]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,15);
    _MM512_MASK_STOREU_PD(&c[i*4+0],xc0,15);
}
#else
printf("cppgemm_2_21_4_4\n");
for(int m=0;m<21;m++){
   for(int n=0;n<4;n++){
      for(int k=0;k<4;k++){
         c[m*4+n]+=a[m*4+k]*b[k*4+n];
      }
   }
}
#endif
}
 
