#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec(target(mic))
void smm_dnn_9_5_10(const double* a, const double* b, double* c){
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
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],31);
 xb1 = _MM512_MASK_LOADU_PD(&b[5+0],31);
 xb2 = _MM512_MASK_LOADU_PD(&b[10+0],31);
 xb3 = _MM512_MASK_LOADU_PD(&b[15+0],31);
 xb4 = _MM512_MASK_LOADU_PD(&b[20+0],31);
 xb5 = _MM512_MASK_LOADU_PD(&b[25+0],31);
 xb6 = _MM512_MASK_LOADU_PD(&b[30+0],31);
 xb7 = _MM512_MASK_LOADU_PD(&b[35+0],31);
 xb8 = _MM512_MASK_LOADU_PD(&b[40+0],31);
 xb9 = _MM512_MASK_LOADU_PD(&b[45+0],31);
for(i=0;i<9;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*5+0],31);
    xa0=_mm512_set1_pd(a[i*10+0]);
    xa1=_mm512_set1_pd(a[i*10+1]);
    xa2=_mm512_set1_pd(a[i*10+2]);
    xa3=_mm512_set1_pd(a[i*10+3]);
    xa4=_mm512_set1_pd(a[i*10+4]);
    xa5=_mm512_set1_pd(a[i*10+5]);
    xa6=_mm512_set1_pd(a[i*10+6]);
    xa7=_mm512_set1_pd(a[i*10+7]);
    xa8=_mm512_set1_pd(a[i*10+8]);
    xa9=_mm512_set1_pd(a[i*10+9]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa7,xb7,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa8,xb8,xc0,31);
    xc0=_mm512_mask3_fmadd_pd(xa9,xb9,xc0,31);
    _MM512_MASK_STOREU_PD(&c[i*5+0],xc0,31);
}
#else
printf("cppgemm_2_9_10_5\n");
for(int m=0;m<9;m++){
   for(int n=0;n<5;n++){
      for(int k=0;k<10;k++){
         c[m*5+n]+=a[m*10+k]*b[k*5+n];
      }
   }
}
#endif
}
 
