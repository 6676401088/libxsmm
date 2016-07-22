#include <libxsmm_source.h>


LIBXSMM_EXTERN libxsmm_dmmfunction dmmdispatch(int m, int n, int k)
{
  return libxsmm_dmmdispatch(m, n, k,
    NULL/*lda*/, NULL/*ldb*/, NULL/*ldc*/,
    NULL/*alpha*/, NULL/*beta*/,
    NULL/*flags*/, NULL/*prefetch*/);
}

