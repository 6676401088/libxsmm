#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec(target(mic))
void smm_dnn_8_6_20(const double* a, const double* b, double* c){
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
__m512d xa11;
__m512d xa12;
__m512d xa13;
__m512d xa14;
__m512d xa15;
__m512d xa16;
__m512d xa17;
__m512d xa18;
__m512d xa19;
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
__m512d xb11;
__m512d xb12;
__m512d xb13;
__m512d xb14;
__m512d xb15;
__m512d xb16;
__m512d xb17;
__m512d xb18;
__m512d xb19;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],63);
 xb1 = _MM512_MASK_LOADU_PD(&b[6+0],63);
 xb2 = _MM512_MASK_LOADU_PD(&b[12+0],63);
 xb3 = _MM512_MASK_LOADU_PD(&b[18+0],63);
 xb4 = _MM512_MASK_LOADU_PD(&b[24+0],63);
 xb5 = _MM512_MASK_LOADU_PD(&b[30+0],63);
 xb6 = _MM512_MASK_LOADU_PD(&b[36+0],63);
 xb7 = _MM512_MASK_LOADU_PD(&b[42+0],63);
 xb8 = _MM512_MASK_LOADU_PD(&b[48+0],63);
 xb9 = _MM512_MASK_LOADU_PD(&b[54+0],63);
 xb10 = _MM512_MASK_LOADU_PD(&b[60+0],63);
 xb11 = _MM512_MASK_LOADU_PD(&b[66+0],63);
 xb12 = _MM512_MASK_LOADU_PD(&b[72+0],63);
 xb13 = _MM512_MASK_LOADU_PD(&b[78+0],63);
 xb14 = _MM512_MASK_LOADU_PD(&b[84+0],63);
 xb15 = _MM512_MASK_LOADU_PD(&b[90+0],63);
 xb16 = _MM512_MASK_LOADU_PD(&b[96+0],63);
 xb17 = _MM512_MASK_LOADU_PD(&b[102+0],63);
 xb18 = _MM512_MASK_LOADU_PD(&b[108+0],63);
 xb19 = _MM512_MASK_LOADU_PD(&b[114+0],63);
for(i=0;i<8;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*6+0],63);
    xa0=_mm512_set1_pd(a[i*20+0]);
    xa1=_mm512_set1_pd(a[i*20+1]);
    xa2=_mm512_set1_pd(a[i*20+2]);
    xa3=_mm512_set1_pd(a[i*20+3]);
    xa4=_mm512_set1_pd(a[i*20+4]);
    xa5=_mm512_set1_pd(a[i*20+5]);
    xa6=_mm512_set1_pd(a[i*20+6]);
    xa7=_mm512_set1_pd(a[i*20+7]);
    xa8=_mm512_set1_pd(a[i*20+8]);
    xa9=_mm512_set1_pd(a[i*20+9]);
    xa10=_mm512_set1_pd(a[i*20+10]);
    xa11=_mm512_set1_pd(a[i*20+11]);
    xa12=_mm512_set1_pd(a[i*20+12]);
    xa13=_mm512_set1_pd(a[i*20+13]);
    xa14=_mm512_set1_pd(a[i*20+14]);
    xa15=_mm512_set1_pd(a[i*20+15]);
    xa16=_mm512_set1_pd(a[i*20+16]);
    xa17=_mm512_set1_pd(a[i*20+17]);
    xa18=_mm512_set1_pd(a[i*20+18]);
    xa19=_mm512_set1_pd(a[i*20+19]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa7,xb7,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa8,xb8,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa9,xb9,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa10,xb10,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa11,xb11,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa12,xb12,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa13,xb13,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa14,xb14,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa15,xb15,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa16,xb16,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa17,xb17,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa18,xb18,xc0,63);
    xc0=_mm512_mask3_fmadd_pd(xa19,xb19,xc0,63);
    _MM512_MASK_STOREU_PD(&c[i*6+0],xc0,63);
}
#else
printf("cppgemm_2_8_20_6\n");
for(int m=0;m<8;m++){
   for(int n=0;n<6;n++){
      for(int k=0;k<20;k++){
         c[m*6+n]+=a[m*20+k]*b[k*6+n];
      }
   }
}
#endif
}
 
