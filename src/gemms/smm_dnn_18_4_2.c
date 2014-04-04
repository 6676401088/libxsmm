#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec( target (mic))
void smm_dnn_18_4_2(double* a,double* b,double* c){
#ifdef __MIC__
int i;
__m512d xa0;
__m512d xa1;
__m512d xb0;
__m512d xb1;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],15);
 xb1 = _MM512_MASK_LOADU_PD(&b[4+0],15);
for(i=0;i<18;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*4+0],15);
    xa0=_mm512_set1_pd(a[i*2+0]);
    xa1=_mm512_set1_pd(a[i*2+1]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,15);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,15);
    _MM512_MASK_STOREU_PD(&c[i*4+0],xc0,15);
}
#else
printf("cppgemm_2_18_2_4\n");
for(int m=0;m<18;m++){
   for(int n=0;n<4;n++){
      for(int k=0;k<2;k++){
         c[m*4+n]+=a[m*2+k]*b[k*4+n];
      }
   }
}
#endif
}
 
