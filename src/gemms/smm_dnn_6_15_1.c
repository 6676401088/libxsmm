#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec( target (mic))
void smm_dnn_6_15_1(double* a,double* b,double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xb0;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],255);
for(i=0;i<6;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*15+0],255);
    xa0=_mm512_set1_pd(a[i*1+0]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,255);
    _MM512_MASK_STOREU_PD(&c[i*15+0],xc0,255);
}
 xb0 = _MM512_MASK_LOADU_PD(&b[0+8],127);
for(i=0;i<6;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*15+8],127);
    xa0=_mm512_set1_pd(a[i*1+0]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,127);
    _MM512_MASK_STOREU_PD(&c[i*15+8],xc0,127);
}
#else
printf("cppgemm_2_6_1_15\n");
for(int m=0;m<6;m++){
   for(int n=0;n<15;n++){
      for(int k=0;k<1;k++){
         c[m*15+n]+=a[m*1+k]*b[k*15+n];
      }
   }
}
#endif
}
 
