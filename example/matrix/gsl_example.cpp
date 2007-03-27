// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include <stdio.h>

//#define GSL_DLL

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_matrix_double.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>

//#include <gsl/gsl_math.h>
//#include <gsl/gsl_chebyshev.h>

#include <yarp/sig/Matrix.h>
#include <yarp/sig/Vector.h>

using namespace yarp::sig;

int main(int argc, const char **argv)
{
    Vector v1(5);
    Vector v2(5);
    v1=10;
    v2=1;

    double r;
    gsl_blas_ddot((gsl_vector *)(v1.getGslVector()), 
                  (gsl_vector *)(v2.getGslVector()), &r);

    printf("Res vector: %lf\n", r);

    Matrix m1;
    Matrix m2;
    Matrix m3;
    m1.resize(2,3);
    m2.resize(3,2);
    m1=3;
    m2=3;

    m3.resize(2,2);
    m3.zero();

    //gsl_matrix_view A1 = gsl_matrix_view_array(m1.data(), 2, 3);
    //gsl_matrix_view A2 = gsl_matrix_view_array(m2.data(), 3, 2);
    //gsl_matrix_view A3 = gsl_matrix_view_array(m3.data(), 2, 2);

    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 
        1.0, 
        (const gsl_matrix *) m1.getGslMatrix(), 
        (const gsl_matrix *) m2.getGslMatrix(),
        0.0, 
        (gsl_matrix *) m3.getGslMatrix());

    printf("Result: (%s)", m3.toString().c_str());

    return 0; 
}

