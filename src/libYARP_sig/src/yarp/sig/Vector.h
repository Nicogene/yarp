/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * Copyright (C) 2006-2010 RobotCub Consortium
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef YARP_SIG_VECTOR_H
#define YARP_SIG_VECTOR_H

#include <cstring>
#include <cstddef> //defines size_t
#include <memory>
#include <string>
#include <vector>

#include <yarp/os/Portable.h>
#include <yarp/os/ManagedBytes.h>
#include <yarp/os/Type.h>

#include <yarp/sig/api.h>
#include <yarp/os/Log.h>

/**
* \file Vector.h contains the definition of a Vector type
*/
namespace yarp {

    namespace sig {
        class VectorBase;
        template<class T> class VectorOf;
        // Swig(3.0.12) crashes when generating
        // ruby bindings without these guards.
        // Bindings for Vector are generated
        // anyways throught the %template directive
        // in the interface file.
#ifndef SWIG
        typedef VectorOf<double> Vector;
#endif
    }
}


/**
* \ingroup sig_class
*
* A Base class for a VectorOf<T>, provide default implementation for
* read/write methods. Warning: the current implementation assumes the same
* representation for data type (endianness).
*/
class YARP_sig_API yarp::sig::VectorBase : public yarp::os::Portable
{
public:
    virtual size_t getElementSize() const = 0;
    virtual int getBottleTag() const = 0;

    virtual size_t getListSize() const = 0;
    virtual const char *getMemoryBlock() const = 0;
    virtual char *getMemoryBlock() = 0;
    virtual void resize(size_t size) = 0;

    /*
    * Read vector from a connection.
    * return true iff a vector was read correctly
    */
    bool read(yarp::os::ConnectionReader& connection) override;

    /**
    * Write vector to a connection.
    * return true iff a vector was written correctly
    */
    bool write(yarp::os::ConnectionWriter& connection) const override;

protected:
    virtual std::string getFormatStr(int tag) const;

};

/*
* This is a simple function that maps a type into its corresponding BOTTLE tag.
* Used for bottle compatible serialization, called inside getBottleTag().
* Needs to be instantiated for each type T used in VectorOf<T>.
*/
template<class T>
inline int BottleTagMap () {
    /* make sure this is never called unspecified */
    yAssert(0);
    return 0;
  }

template<>
inline int BottleTagMap <double> () {
    return BOTTLE_TAG_FLOAT64;
  }

template<>
inline int BottleTagMap <int> () {
    return BOTTLE_TAG_INT32;
  }

/**
* \ingroup sig_class
*
* Provides:
* - push_back(), pop_back() to add/remove an element at the end of the vector
* - resize(), to create an array of elements
* - clear(), to clean the array (remove all elements)
* - use [] to access single elements without range checking
* - use size() to get the current size of the Vector
* - use operator= to copy Vectors
* - read/write network methods
* Warning: the class is designed to work with simple types (i.e. types
* that do not allocate internal memory). Template instantiation needs to
* be checked to avoid unresolved externals. Network communication assumes
* same data representation (endianness) between machines.
*/
template<class T>
class yarp::sig::VectorOf : public VectorBase,
                            public std::vector<T>
{
public:

    using std::vector<T>::vector;
    VectorOf() {
        this->reserve(16); // preallocate space for 16 elements
    }

    //TODO make it better
    /**
    * Builds a vector and initialize it with
    * values from 'p'. Copies memory.
    * @param s the size of the data to be copied
    * @param T* the pointer to the data
    */
    VectorOf(size_t s, const T *p)
    {
        this->resize(s);
        memcpy(this->data(), p, sizeof(T)*s);
    }


    void resize(size_t size) override {
        std::vector<T>::resize(size);
    }

    /**
     * Build a vector and initialize it with def.
     * @param s the size
     * @param def a default value used to fill the vector
     */
    void resize(size_t size, const T&def)
    {
        this->resize(size);
        std::fill(this->begin(), this->end(), def);
    }


    size_t getElementSize() const override {
        return sizeof(T);
    }

    int getBottleTag() const override {
        return BottleTagMap <T>();
    }

    size_t getListSize() const override
    {
        return this->size();
    }

    const char* getMemoryBlock() const override
    {
        return reinterpret_cast<const char*>(this->data());
    }

    char* getMemoryBlock() override
    {
        return reinterpret_cast<char*>(this->data());
    }

#ifndef YARP_NO_DEPRECATED // since YARP 3.2.0
    YARP_DEPRECATED_MSG("Use either data() if you need the pointer to the first element,"
                        " or cbegin() if you need the iterator")
    inline const T *getFirst() const
    {
        return this->data();
    }

    YARP_DEPRECATED_MSG("Use either data() if you need the pointer to the first element,"
                        " or begin() if you need the iterator")
    inline T *getFirst()
    {
        return this->data();
    }
#endif // YARP_NO_DEPRECATED

    /**
    * Get the length of the vector.
    * @return the length of the vector.
    */
    inline size_t length() const
    { return this->size();}

    /**
    * Zero the elements of the vector.
    */
    void zero()
    {
        std::fill(this->begin(), this->end(), 0);
    }

    /**
     * Set all elements of the vector to a scalar.
     * */

    const VectorOf<T> &operator=(T v)
    {
        std::fill(this->begin(), this->end(), v);
        return *this;
    }

    /**
    * Return a pointer to the first element of the vector.
    * @return a pointer to double (or nullptr if the vector is of zero length)
    */
    inline T *data()
    { return this->empty() ? nullptr : &(this->at(0)); }

    /**
    * Return a pointer to the first element of the vector,
    * const version
    * @return a (const) pointer to double (or nullptr if the vector is of zero length)
    */
    inline const T *data() const
    { return this->empty() ? nullptr : &(this->at(0)); }
    /**
    * Single element access, no range check.
    * @param i the index of the element to access.
    * @return a reference to the requested element.
    */
    inline T &operator()(size_t i)
    {
        return this->data()[i];
    }

    /**
    * Single element access, no range check, const version.
    * @param i the index of the element to access.
    * @return a reference to the requested element.
    */
    inline const T &operator()(size_t i) const
    {
        return this->data()[i];
    }

    /**
    * Creates a string object containing a text representation of the object. Useful for printing.
    * To get a nice format the optional parameters precision and width may be used (same meaning as in printf and cout).
    * @param precision the number of digits to be printed after the decimal point.
    * @param width minimum number of characters to be printed. If the value to be printed is shorter than this number, the result is padded with blank spaces. The value is never truncated.
    * If width is specified the inter-value separator is a blank space, otherwise it is a tab.
    * Warning: the string format might change in the future. This method
    * is here to ease debugging.
    */
    std::string toString(int precision=-1, int width=-1) const
    {
        std::string ret = "";
        size_t c = 0;
        const size_t buffSize = 256;
        char tmp[buffSize];
        std::string formatStr;
        if (getBottleTag() == BOTTLE_TAG_FLOAT64) {
            if (width<0) {
                formatStr = "% .*lf\t";
                for (c=0;c<length();c++) {
                    snprintf(tmp, buffSize, formatStr.c_str(), precision, (*this)[c]);
                    ret+=tmp;
                }
            }
            else{
                formatStr = "% *.*lf ";
                for (c=0;c<length();c++){
                    snprintf(tmp, buffSize, formatStr.c_str(), width, precision, (*this)[c]);
                    ret+=tmp;
                }
            }
        }
        else {
            formatStr = "%" + getFormatStr(getBottleTag()) + " ";
            for (c=0;c<length();c++) {
                snprintf(tmp, buffSize, formatStr.c_str(), (*this)[c]);
                ret+=tmp;
            }
        }

        if (length()>=1)
            return ret.substr(0, ret.length()-1);
        return ret;
    }

    // FIXME to do it better with vector<T>::const_iterator
    /**
    * Creates and returns a new vector, being the portion of the original
    * vector defined by the first and last indexes of the items to be included
    * in the subvector. The indexes are checked: if wrong, a null vector is
    * returned.
    */
    VectorOf<T> subVector(unsigned int first, unsigned int last) const
    {
        VectorOf<T> ret;
        if ((first<=last)&&((int)last<(int)this->size()))
        {
            ret.resize(last-first+1);
            for (unsigned int k=first; k<=last; k++)
                ret[k-first]=(*this)[k];
        }
        return ret;
    }

    /**
     * Set a portion of this vector with the values of the specified vector.
     * If the specified vector v is to big the method does not resize the vector,
     * but return false.
     *
     * @param position index of the first value to set
     * @param v vector containing the values to set
     * @return true if the operation succeeded, false otherwise
     */
    bool setSubvector(int position, const VectorOf<T> &v)
    {
        if (position+v.size() > this->size())
            return false;
        for (size_t i=0;i<v.size();i++)
            (*this)[position+i] = v(i);
        return true;
    }

    yarp::os::Type getType() const override {
        return yarp::os::Type::byName("yarp/vector");
    }
};


#ifdef _MSC_VER
/*YARP_sig_EXTERN*/ template class YARP_sig_API yarp::sig::VectorOf<double>;
#endif

#endif // YARP_SIG_VECTOR_H
