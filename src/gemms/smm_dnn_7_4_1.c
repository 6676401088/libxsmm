#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec( target (mic))
void smm_dnn_7_4_1(double* a,double* b,double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xb0;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],15);
for(i=0;i<7;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*4+0],15);
    xa0=_mm512_set1_pd(a[i*1+0]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,15);
    _MM512_MASK_STOREU_PD(&c[i*4+0],xc0,15);
}
#else
printf("cppgemm_2_7_1_4\n");
for(int m=0;m<7;m++){
   for(int n=0;n<4;n++){
      for(int k=0;k<1;k++){
         c[m*4+n]+=a[m*1+k]*b[k*4+n];
      }
   }
}
#endif
}
 
