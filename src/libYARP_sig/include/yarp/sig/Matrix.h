// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
* Copyright (C) 2007 Lorenzo Natale
* CopyPolicy: Released under the terms of the GNU GPL v2.0.
*
*/

// $Id: Matrix.h,v 1.12 2007-03-27 13:14:12 natta Exp $ 

#ifndef _YARP2_MATRIX_
#define _YARP2_MATRIX_

#include <stdlib.h> //defines size_t
#include <yarp/os/Portable.h>
#include <yarp/sig/Vector.h>

/**
* \file Matrix.h contains the definition of a Matrix type 
*/
namespace yarp {
    namespace sig {
        class Matrix;
    }
}


/**
* \ingroup sig_class
*
* A class for a Matrix. A Matrix can be sent/read to/from 
* a port. Use the [] operator to access the r row of the matrix.
* The function returns a pointer so [][] access the r,c element
* in the matrix.
*/
class yarp::sig::Matrix: public yarp::os::Portable
{
private:
    VectorOf<double> storage;
    void *gslData;
    double *first;
    double **matrix; //double pointer access to elements

    int nrows;
    int ncols;

    /**
    * Update pointer to data, call this every time you
    * change the size of the object.
    */
    void updatePointers();

    void allocGslData();
    void freeGslData();
    void updateGslData();

public:
    Matrix():
          first(0),
          matrix(0),
          nrows(0),
          ncols(0)
      {
          allocGslData();
      }

      Matrix(int r, int c):
      first(0),
          matrix(0),
          nrows(r),
          ncols(c)
      {
          storage.resize(r*c,0.0);
          allocGslData();
          updatePointers();
      }

      /**
      * Copy constructor.
      */
      Matrix(const Matrix &m):
      storage(m.storage),
          first(0),
          matrix(0)
      {
          nrows=m.nrows;
          ncols=m.ncols;
          allocGslData();
          updatePointers();
      }

      /**
      * Destructor.
      */
      ~Matrix();

      /**
      * Copy operator.
      */
      const Matrix &operator=(const Matrix &r);

      /**
      * Set all elements of the matrix to a given value.
      * @param v a scalar
      * @return a reference to the object
      */
      const Matrix &operator=(double v);

      /**
      * Return number of rows.
      */
      int rows() const
      { return nrows; }

      /**
      * Return number of columns.
      */
      int cols() const
      { return ncols; }

      /**
      * Resize the matrix.
      * @param r number of rows
      * @param c number of columns
      */
      void resize(int r, int c);

      /**
      * Single element access, no range check. Returns
      * a pointer to the r-th row.
      * @param r row number.
      * @return a pointer to the first element of the row.
      */
      inline double *operator[](int r)
      { return matrix[r]; }

      /**
      * Single element access, no range check (const version). 
      * Returns a pointer to the r-th row.
      * @param r row number.
      * @return a (const) pointer to the first element of the row.
      */
      inline const double *operator[](int r) const
      { return matrix[r]; }

      /**
      * Single element access, no range check. 
      * @param r row number.
      * @param c col number.
      * @return a reference to the element.
      */
      inline const double &operator()(int r, int c) const
      { return matrix[r][c]; }

      /**
      * Single element access, no range check. 
      * @param r row number.
      * @param c col number.
      * @return a (const) reference to the element.
      */
      inline double &operator()(int r, int c)
      { return matrix[r][c]; }

      /**
      * Zero the matrix. Set all elements of the matrix to zero.
      */
      void zero();

      /**
      * Return the transposed of the matrix. 
      * @return the transposed copy of the matrix.
      */
      Matrix transposed();

      /**
      * Build an identity matrix, don't resize.
      * @return a reference to the object.
      */
      const Matrix &eye();

      /**
      * Extract a submatrix from (r1,c1 to r2,c2)
      * @param r1 start point row
      * @param c1 start point col
      * @param r2 end point row
      * @param c2 end point col
      * @return the sumbatrix
      */
      Matrix submatrix(int r1, int c1, int r2, int c2) const;

      /**
      * Get a row of the matrix as a vector.
      * @param r the row number
      * @return a vector which contains the requested row
      */
      Vector getRow(int r) const;

      /**
      * Get a columns of the matrix as a vector.
      * @param c the column number
      * @return a vector which contains the requested row
      */
      Vector getCol(int c) const;

      /**
      * Print matrix to a string. Useful for display, writing to text.
      * Warning: the string format might change in the future. This method
      * is here to ease debugging.
      * @return a const string which contain the matrix in text form.
      */
      yarp::os::ConstString toString() const;

      /**
      * Return a pointer to the first element
      * @return the pointer to the first element
      */
      inline double *data()
      {return storage.getFirst();}

      /**
      * Return a pointer to the first element (const version).
      * @return the (const) pointer to the first element
      */
      inline const double *data() const
      {return storage.getFirst();}

      /**
      * True iff all elements of a match all element of b.
      */
      bool operator==(const yarp::sig::Matrix &r) const;

      /**
      * Return a void pointer to a gsl compatible structure
      * which can be safely cast to gsl_matrix *.
      */
      void *getGslMatrix()
      { return gslData; }

      /**
      * Return a void pointer to a gsl compatible structure
      * which can be safely cast to gsl_matrix *.
      */
      const void *getGslMatrix() const
      { return gslData; }

      ///////// Serialization methods
      /*
      * Read vector from a connection.
      * return true iff a vector was read correctly
      */
      virtual bool read(yarp::os::ConnectionReader& connection);

      /**
      * Write vector to a connection.
      * return true iff a vector was written correctly
      */
      virtual bool write(yarp::os::ConnectionWriter& connection);

};

#endif

