#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec(target(mic))
void smm_dnn_21_3_6(const double* a, const double* b, double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xa1;
__m512d xa2;
__m512d xa3;
__m512d xa4;
__m512d xa5;
__m512d xb0;
__m512d xb1;
__m512d xb2;
__m512d xb3;
__m512d xb4;
__m512d xb5;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],7);
 xb1 = _MM512_MASK_LOADU_PD(&b[3+0],7);
 xb2 = _MM512_MASK_LOADU_PD(&b[6+0],7);
 xb3 = _MM512_MASK_LOADU_PD(&b[9+0],7);
 xb4 = _MM512_MASK_LOADU_PD(&b[12+0],7);
 xb5 = _MM512_MASK_LOADU_PD(&b[15+0],7);
for(i=0;i<21;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*3+0],7);
    xa0=_mm512_set1_pd(a[i*6+0]);
    xa1=_mm512_set1_pd(a[i*6+1]);
    xa2=_mm512_set1_pd(a[i*6+2]);
    xa3=_mm512_set1_pd(a[i*6+3]);
    xa4=_mm512_set1_pd(a[i*6+4]);
    xa5=_mm512_set1_pd(a[i*6+5]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,7);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,7);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,7);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,7);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,7);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,7);
    _MM512_MASK_STOREU_PD(&c[i*3+0],xc0,7);
}
#else
printf("cppgemm_2_21_6_3\n");
for(int m=0;m<21;m++){
   for(int n=0;n<3;n++){
      for(int k=0;k<6;k++){
         c[m*3+n]+=a[m*6+k]*b[k*3+n];
      }
   }
}
#endif
}
 
