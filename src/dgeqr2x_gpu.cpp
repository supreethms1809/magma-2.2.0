/*
    -- MAGMA (version 2.2.0) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2016
       
       @author Stan Tomov

       @generated from src/zgeqr2x_gpu.cpp, normal z -> d, Sun Nov 20 20:20:21 2016

*/
#include "magma_internal.h"

/***************************************************************************//**
    Purpose
    -------
    DGEQR2 computes a QR factorization of a real m by n matrix A:
    A = Q * R.

    This expert routine requires two more arguments than the standard
    dgeqr2, namely, dT and ddA, explained below. The storage for A is
    also not as in the LAPACK's dgeqr2 routine (see below).

    The first is used to output the triangular
    n x n factor T of the block reflector used in the factorization.
    The second holds the diagonal nxn blocks of A, i.e., the diagonal
    submatrices of R.

    This version implements the right-looking QR.
    A hard-coded requirement for N is to be <= min(M, 128). For larger N one
    should use a blocking QR version.

    Arguments
    ---------
    @param[in]
    m       INTEGER
            The number of rows of the matrix A.  M >= 0.

    @param[in]
    n       INTEGER
            The number of columns of the matrix A. 0 <= N <= min(M, 128).

    @param[in,out]
    dA      DOUBLE PRECISION array, dimension (LDDA,N)
            On entry, the m by n matrix A.
            On exit, the orthogonal matrix Q as a
            product of elementary reflectors (see Further Details).
    \n
            the elements on and above the diagonal of the array
            contain the min(m,n) by n upper trapezoidal matrix R (R is
            upper triangular if m >= n); the elements below the diagonal,
            with the array TAU, represent the orthogonal matrix Q as a
            product of elementary reflectors (see Further Details).

    @param[in]
    ldda    INTEGER
            The leading dimension of the array A.  LDDA >= max(1,M).

    @param[out]
    dtau    DOUBLE PRECISION array, dimension (min(M,N))
            The scalar factors of the elementary reflectors (see Further
            Details).

    @param[out]
    dT      DOUBLE PRECISION array, dimension N x N.
            Stores the triangular N x N factor T of the block reflector
            used in the factorization. The lower triangular part is 0.

    @param[out]
    ddA     DOUBLE PRECISION array, dimension N x N.
            Stores the elements of the upper N x N diagonal block of A.
            LAPACK stores this array in A. There are 0s below the diagonal.

    @param
    dwork   (workspace) DOUBLE PRECISION array, dimension (N)

    @param[out]
    info    INTEGER
      -     = 0: successful exit
      -     < 0: if INFO = -i, the i-th argument had an illegal value

    Further Details
    ---------------
    The matrix Q is represented as a product of elementary reflectors

       Q = H(1) H(2) . . . H(k), where k = min(m,n).

    Each H(i) has the form

       H(i) = I - tau * v * v'

    where tau is a real scalar, and v is a real vector with
    v(1:i-1) = 0 and v(i) = 1; v(i+1:m) is stored on exit in A(i+1:m,i),
    and tau in TAU(i).

    @ingroup magma_geqr2
*******************************************************************************/
extern "C" magma_int_t
magma_dgeqr2x_gpu(
    magma_int_t m, magma_int_t n,
    magmaDouble_ptr dA, magma_int_t ldda,
    magmaDouble_ptr dtau,
    magmaDouble_ptr dT,
    magmaDouble_ptr ddA,
    magmaDouble_ptr        dwork,
    magma_int_t *info)
{
    #define dA(i_,j_) (dA + (i_) + (j_)*ldda)
    
    magma_int_t i, min_mn;

    magmaDouble_ptr dnorm = dwork;
    magmaDouble_ptr dwork2 = (magmaDouble_ptr)(dwork + 2*n);

    magma_queue_t queue;
    magma_device_t cdev;
    magma_getdevice( &cdev );
    magma_queue_create( cdev, &queue );

    *info = 0;
    if (m < 0) {
        *info = -1;
    } else if (n < 0 || n > min(m, 128)) {
        *info = -2;
    } else if (ldda < max(1,m)) {
        *info = -4;
    }
    if (*info != 0) {
        magma_xerbla( __func__, -(*info) );
        return *info;
    }

    /* Compute the norms of the trailing columns */
    min_mn = min(m,n);
    // magmablas_dnrm2_cols( m, min_mn, dA(0,0), ldda, dnorm, queue );

    for (i = 0; i < min_mn; ++i) {
        /*  Generate elementary reflector H(i) to annihilate A(i+1:m,i) */
        magmablas_dnrm2_cols( m-i, 1, dA(i,i), ldda, dnorm+i, queue );
        magma_dlarfgx_gpu( m-i, dA(i, i), dA(min(i+1,m), i), dtau+i, dnorm+i,
                           ddA + i + i*n, i, queue );
        
        if (i < n) {
            /* Apply H(i)' to A(i:m,i+1:n) from the left */
            magma_dlarfx_gpu( m-i, n-i-1, dA(i, i), dtau+i,
                              //dA(i, i+1), ldda, dnorm+i+1,
                              dA(i, 0), ldda, dnorm+i+1,
                              dT, i, dwork2, queue );
        }
    }
    
    magma_queue_destroy( queue );

    return *info;
} /* magma_dgeqr2 */
