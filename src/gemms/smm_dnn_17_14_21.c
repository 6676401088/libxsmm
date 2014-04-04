#include <immintrin.h>
#include <micsmmmisc.h>
#include <mkl.h>
__declspec( target (mic))
void smm_dnn_17_14_21(double* a,double* b,double* c){
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
__m512d xa20;
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
__m512d xb20;
__m512d xc0;
 xb0 = _MM512_MASK_LOADU_PD(&b[0+0],255);
 xb1 = _MM512_MASK_LOADU_PD(&b[14+0],255);
 xb2 = _MM512_MASK_LOADU_PD(&b[28+0],255);
 xb3 = _MM512_MASK_LOADU_PD(&b[42+0],255);
 xb4 = _MM512_MASK_LOADU_PD(&b[56+0],255);
 xb5 = _MM512_MASK_LOADU_PD(&b[70+0],255);
 xb6 = _MM512_MASK_LOADU_PD(&b[84+0],255);
 xb7 = _MM512_MASK_LOADU_PD(&b[98+0],255);
 xb8 = _MM512_MASK_LOADU_PD(&b[112+0],255);
 xb9 = _MM512_MASK_LOADU_PD(&b[126+0],255);
 xb10 = _MM512_MASK_LOADU_PD(&b[140+0],255);
 xb11 = _MM512_MASK_LOADU_PD(&b[154+0],255);
 xb12 = _MM512_MASK_LOADU_PD(&b[168+0],255);
 xb13 = _MM512_MASK_LOADU_PD(&b[182+0],255);
 xb14 = _MM512_MASK_LOADU_PD(&b[196+0],255);
 xb15 = _MM512_MASK_LOADU_PD(&b[210+0],255);
 xb16 = _MM512_MASK_LOADU_PD(&b[224+0],255);
 xb17 = _MM512_MASK_LOADU_PD(&b[238+0],255);
 xb18 = _MM512_MASK_LOADU_PD(&b[252+0],255);
 xb19 = _MM512_MASK_LOADU_PD(&b[266+0],255);
 xb20 = _MM512_MASK_LOADU_PD(&b[280+0],255);
for(i=0;i<17;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*14+0],255);
    xa0=_mm512_set1_pd(a[i*21+0]);
    xa1=_mm512_set1_pd(a[i*21+1]);
    xa2=_mm512_set1_pd(a[i*21+2]);
    xa3=_mm512_set1_pd(a[i*21+3]);
    xa4=_mm512_set1_pd(a[i*21+4]);
    xa5=_mm512_set1_pd(a[i*21+5]);
    xa6=_mm512_set1_pd(a[i*21+6]);
    xa7=_mm512_set1_pd(a[i*21+7]);
    xa8=_mm512_set1_pd(a[i*21+8]);
    xa9=_mm512_set1_pd(a[i*21+9]);
    xa10=_mm512_set1_pd(a[i*21+10]);
    xa11=_mm512_set1_pd(a[i*21+11]);
    xa12=_mm512_set1_pd(a[i*21+12]);
    xa13=_mm512_set1_pd(a[i*21+13]);
    xa14=_mm512_set1_pd(a[i*21+14]);
    xa15=_mm512_set1_pd(a[i*21+15]);
    xa16=_mm512_set1_pd(a[i*21+16]);
    xa17=_mm512_set1_pd(a[i*21+17]);
    xa18=_mm512_set1_pd(a[i*21+18]);
    xa19=_mm512_set1_pd(a[i*21+19]);
    xa20=_mm512_set1_pd(a[i*21+20]);
    xc0=_mm512_mask3_fmadd_pd(xa0,xb0,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa1,xb1,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa2,xb2,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa3,xb3,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa4,xb4,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa5,xb5,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa6,xb6,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa7,xb7,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa8,xb8,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa9,xb9,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa10,xb10,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa11,xb11,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa12,xb12,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa13,xb13,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa14,xb14,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa15,xb15,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa16,xb16,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa17,xb17,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa18,xb18,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa19,xb19,xc0,255);
    xc0=_mm512_mask3_fmadd_pd(xa20,xb20,xc0,255);
    _MM512_MASK_STOREU_PD(&c[i*14+0],xc0,255);
}
 xb0 = _MM512_MASK_LOADU_PD(&b[0+8],63);
 xb1 = _MM512_MASK_LOADU_PD(&b[14+8],63);
 xb2 = _MM512_MASK_LOADU_PD(&b[28+8],63);
 xb3 = _MM512_MASK_LOADU_PD(&b[42+8],63);
 xb4 = _MM512_MASK_LOADU_PD(&b[56+8],63);
 xb5 = _MM512_MASK_LOADU_PD(&b[70+8],63);
 xb6 = _MM512_MASK_LOADU_PD(&b[84+8],63);
 xb7 = _MM512_MASK_LOADU_PD(&b[98+8],63);
 xb8 = _MM512_MASK_LOADU_PD(&b[112+8],63);
 xb9 = _MM512_MASK_LOADU_PD(&b[126+8],63);
 xb10 = _MM512_MASK_LOADU_PD(&b[140+8],63);
 xb11 = _MM512_MASK_LOADU_PD(&b[154+8],63);
 xb12 = _MM512_MASK_LOADU_PD(&b[168+8],63);
 xb13 = _MM512_MASK_LOADU_PD(&b[182+8],63);
 xb14 = _MM512_MASK_LOADU_PD(&b[196+8],63);
 xb15 = _MM512_MASK_LOADU_PD(&b[210+8],63);
 xb16 = _MM512_MASK_LOADU_PD(&b[224+8],63);
 xb17 = _MM512_MASK_LOADU_PD(&b[238+8],63);
 xb18 = _MM512_MASK_LOADU_PD(&b[252+8],63);
 xb19 = _MM512_MASK_LOADU_PD(&b[266+8],63);
 xb20 = _MM512_MASK_LOADU_PD(&b[280+8],63);
for(i=0;i<17;i+=1){
    xc0 = _MM512_MASK_LOADU_PD(&c[i*14+8],63);
    xa0=_mm512_set1_pd(a[i*21+0]);
    xa1=_mm512_set1_pd(a[i*21+1]);
    xa2=_mm512_set1_pd(a[i*21+2]);
    xa3=_mm512_set1_pd(a[i*21+3]);
    xa4=_mm512_set1_pd(a[i*21+4]);
    xa5=_mm512_set1_pd(a[i*21+5]);
    xa6=_mm512_set1_pd(a[i*21+6]);
    xa7=_mm512_set1_pd(a[i*21+7]);
    xa8=_mm512_set1_pd(a[i*21+8]);
    xa9=_mm512_set1_pd(a[i*21+9]);
    xa10=_mm512_set1_pd(a[i*21+10]);
    xa11=_mm512_set1_pd(a[i*21+11]);
    xa12=_mm512_set1_pd(a[i*21+12]);
    xa13=_mm512_set1_pd(a[i*21+13]);
    xa14=_mm512_set1_pd(a[i*21+14]);
    xa15=_mm512_set1_pd(a[i*21+15]);
    xa16=_mm512_set1_pd(a[i*21+16]);
    xa17=_mm512_set1_pd(a[i*21+17]);
    xa18=_mm512_set1_pd(a[i*21+18]);
    xa19=_mm512_set1_pd(a[i*21+19]);
    xa20=_mm512_set1_pd(a[i*21+20]);
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
    xc0=_mm512_mask3_fmadd_pd(xa20,xb20,xc0,63);
    _MM512_MASK_STOREU_PD(&c[i*14+8],xc0,63);
}
#else
printf("cppgemm_2_17_21_14\n");
for(int m=0;m<17;m++){
   for(int n=0;n<14;n++){
      for(int k=0;k<21;k++){
         c[m*14+n]+=a[m*21+k]*b[k*14+n];
      }
   }
}
#endif
}
 
