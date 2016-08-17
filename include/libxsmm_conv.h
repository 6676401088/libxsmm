/******************************************************************************
** Copyright (c) 2016, Intel Corporation                                     **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Alexander Heinecke, Hans Pabst (Intel Corp.)
******************************************************************************/
#ifndef LIBXSMM_CONV_H
#define LIBXSMM_CONV_H

#include <libxsmm.h>

/** Opaque handles which represents convolutions and LIBXSMM datatypes */
typedef struct LIBXSMM_RETARGETABLE libxsmm_conv_handle libxsmm_conv_handle;
typedef struct LIBXSMM_RETARGETABLE libxsmm_conv_layer libxsmm_conv_layer;
typedef struct LIBXSMM_RETARGETABLE libxsmm_conv_bias libxsmm_conv_bias;
typedef struct LIBXSMM_RETARGETABLE libxsmm_conv_filter libxsmm_conv_filter;
typedef unsigned int libxsmm_conv_err_t;

/** Define error and warning codes */
#define LIBXSMM_CONV_SUCCESS                           0
#define LIBXSMM_CONV_WARN_FALLBACK                 90000
#define LIBXSMM_CONV_ERR_GENERAL                  100000
#define LIBXSMM_CONV_ERR_CREATE_HANDLE            100001
#define LIBXSMM_CONV_ERR_UNSUPPORTED_DATATYPE     100002
#define LIBXSMM_CONV_ERR_INVALID_BLOCKING         100003
#define LIBXSMM_CONV_ERR_INVALID_HANDLE           100004
#define LIBXSMM_CONV_ERR_DATA_NOT_BOUND           100005
#define LIBXSMM_CONV_ERR_CREATE_LAYER             100006
#define LIBXSMM_CONV_ERR_INVALID_LAYER            100007
#define LIBXSMM_CONV_ERR_CREATE_FILTER            100008
#define LIBXSMM_CONV_ERR_INVALID_FILTER           100009
#define LIBXSMM_CONV_ERR_CREATE_BIAS              100010
#define LIBXSMM_CONV_ERR_INVALID_BIAS             100011
#define LIBXSMM_CONV_ERR_MISMATCH_LAYER           100012
#define LIBXSMM_CONV_ERR_INVALID_HANDLE_LAYER     100013
#define LIBXSMM_CONV_ERR_MISMATCH_FILTER          100014
#define LIBXSMM_CONV_ERR_INVALID_HANDLE_FILTER    100015
#define LIBXSMM_CONV_ERR_INVALID_KIND             100016

/** Kinds of supported convolution operations. */
typedef enum libxsmm_conv_kind {
  /** Forward convolution. */
  LIBXSMM_CONV_KIND_FWD,
  /** Forward convolution, fused Bias */
  LIBXSMM_CONV_KIND_FWD_BIAS,
  /** Forward convolution, fused Bias and ReLU */
  LIBXSMM_CONV_KIND_FWD_BIAS_RELU,
  /** Backward convolution. */
  LIBXSMM_CONV_KIND_BWD,
  /** Backward convolution, fused ReLU */
  LIBXSMM_CONV_KIND_BWD_RELU,
  /** Updated weights. */
  LIBXSMM_CONV_KIND_UPD,
  /** Updated weights, fused Bias */
  LIBXSMM_CONV_KIND_UPD_BIAS
} libxsmm_conv_kind;

/** Typ of algorithm used for convolutions. */
typedef enum libxsmm_conv_algo {
  /** direct convolution. */
  LIBXSMM_CONV_ALGO_DIRECT
} libxsmm_conv_algo;

/** Denotes the element/pixel type of an image/channel. */
typedef enum libxsmm_conv_datatype {
  LIBXSMM_CONV_DATATYPE_FP32,
  LIBXSMM_CONV_DATATYPE_INT32,
  LIBXSMM_CONV_DATATYPE_INT16,
  LIBXSMM_CONV_DATATYPE_INT8
} libxsmm_conv_datatype;

/** struct which holds description of convolution */
typedef struct LIBXSMM_RETARGETABLE libxsmm_conv_desc {
  int N;           /* number of images in mini-batch */
  int C;           /* number of input feature maps */
  int H;           /* height of input image */
  int W;           /* width of input image */
  int K;           /* number of output feature maps */
  int R;           /* height of filter kernel */
  int S;           /* width of filter kernel */
  int u;           /* vertical stride */
  int v;           /* horizontal stride */
  int pad_h;       /* height of zero-padding */
  int pad_w;       /* width of zero-padding */
  int splits;      /* number of splits */
} libxsmm_conv_desc;

/** get string of error code */
LIBXSMM_API char* libxsmm_conv_get_error(libxsmm_conv_err_t code);

/** Create a handle (non-NULL if successful), and pre-build all JIT-code versions. */
LIBXSMM_API libxsmm_conv_handle* libxsmm_conv_create_handle(
  libxsmm_conv_desc     conv_desc,
  libxsmm_conv_datatype conv_datatype,
  libxsmm_conv_algo     conv_algo );

LIBXSMM_API libxsmm_conv_handle* libxsmm_conv_create_handle_check(
  libxsmm_conv_desc     conv_desc,
  libxsmm_conv_datatype conv_datatype,
  libxsmm_conv_algo     conv_algo,
  libxsmm_conv_err_t*   status );

/** Release the given convolution handle. */
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_destroy_handle(const libxsmm_conv_handle* handle);

/** Create layers, filters and bias (non-NULL if successful) */
LIBXSMM_API libxsmm_conv_layer* libxsmm_conv_create_input_layer(const libxsmm_conv_handle* handle);
LIBXSMM_API libxsmm_conv_layer* libxsmm_conv_create_output_layer(const libxsmm_conv_handle* handle);
LIBXSMM_API libxsmm_conv_filter* libxsmm_conv_create_filter(const libxsmm_conv_handle* handle);
LIBXSMM_API libxsmm_conv_bias* libxsmm_conv_create_bias(const libxsmm_conv_handle* handle);

LIBXSMM_API libxsmm_conv_layer* libxsmm_conv_create_input_layer_check(const libxsmm_conv_handle* handle, libxsmm_conv_err_t* status);
LIBXSMM_API libxsmm_conv_layer* libxsmm_conv_create_output_layer_check(const libxsmm_conv_handle* handle, libxsmm_conv_err_t* status);
LIBXSMM_API libxsmm_conv_filter* libxsmm_conv_create_filter_check(const libxsmm_conv_handle* handle, libxsmm_conv_err_t* status);
LIBXSMM_API libxsmm_conv_bias* libxsmm_conv_create_bias_check(const libxsmm_conv_handle* handle, libxsmm_conv_err_t* status);

/** Bind layers, filters and bias to convolutions operation */
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_bind_input_layer(libxsmm_conv_handle* handle, const libxsmm_conv_layer* layer);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_bind_output_layer(libxsmm_conv_handle* handle, const libxsmm_conv_layer* layer);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_bind_filter(libxsmm_conv_handle* handle, const libxsmm_conv_filter* filter);

/** Release layers, filters and bias from convolutions operation */
#if 0
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_release_input_layer(libxsmm_conv_handle* handle, const libxsmm_conv_layer* layer);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_release_output_layer(libxsmm_conv_handle* handle, const libxsmm_conv_layer* layer);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_release_filter(libxsmm_conv_handle* handle, const libxsmm_conv_filter* filter);
#endif

/** Release the given layer, filters, bias handle. */
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_destroy_layer(const libxsmm_conv_layer* layer);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_destroy_filter(const libxsmm_conv_filter* filter);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_destroy_bias(const libxsmm_conv_bias* bias);

/**
 * Copy-in from a plain format such as input := [img][splits][ofm][ifm].
 * The index specifies the actual channel number, and an eventual
 * padding is defined by the handle (pitch/stride).
 */
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_copyin_layer(const libxsmm_conv_layer* layer, const void* data);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_copyin_filter(const libxsmm_conv_filter* filter, const void* data);
/*LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_copyin_bias(const libxsmm_conv_bias* bias, const void* data);*/
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_zero_layer(const libxsmm_conv_layer* layer);

/**
 * Copy-out into a plain format such as output := [img][splits][ofm][ifm].
 * The index specifies the actual channel number, and an eventual
 * padding is defined by the handle (pitch/stride).
 */
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_copyout_layer(const libxsmm_conv_layer* layer, void* data);
LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_copyout_filter(const libxsmm_conv_filter* filter, void* data);
/*LIBXSMM_API libxsmm_conv_err_t libxsmm_conv_copyout_bias(const libxsmm_conv_bias* bias, void* data);*/

/** Run the convolution identified by the handle; may use threads internally. */
LIBXSMM_API void libxsmm_convolve(libxsmm_conv_handle* handle, libxsmm_conv_kind kind);

/** Run the convolution identified by the handle; takes a thread id. */
LIBXSMM_API libxsmm_conv_err_t libxsmm_convolve_st(libxsmm_conv_handle* handle, libxsmm_conv_kind kind,
  /*unsigned*/int start_thread, /*unsigned*/int tid, /*unsigned*/int num_threads);

#if defined(LIBXSMM_BUILD) || defined(LIBXSMM_CONV_INTERNAL_API) /* Internal API */

/** Function type used for convolutions (single-precision); the actual signature depends on the kind of convolution. */
typedef LIBXSMM_RETARGETABLE void (*libxsmm_sconvfunction)(const float* input1, const float* input2, float* output,
                                                           const float* ipf1, const float* ipf2, const float* opf);

/** Code generation routine for a forward-convolution kernel. Call libxsmm_release_kernel in order to deallocate the JIT'ted code. */
LIBXSMM_API libxsmm_sconvfunction libxsmm_create_sconv_forward(const libxsmm_convolution_forward_descriptor* descriptor);

/** Code generation routine for a backward-convolution kernel. Call libxsmm_release_kernel in order to deallocate the JIT'ted code. */
LIBXSMM_API libxsmm_sconvfunction libxsmm_create_sconv_backward(const libxsmm_convolution_backward_descriptor* descriptor);

/** Code generation routine for a convolution kernel as specified by descriptor. */
LIBXSMM_API libxsmm_sconvfunction libxsmm_create_sconv_update_weights(const libxsmm_convolution_weight_update_descriptor* descriptor);

#endif
#endif /*LIBXSMM_CONV_H*/

