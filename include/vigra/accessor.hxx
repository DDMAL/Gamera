/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2001 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.1.4, Nov 23 2001 )                                    */
/*    ( Version 1.1.4, Nov 23 2001 )                                    */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/
 
#ifndef VIGRA_ACCESSOR_HXX
#define VIGRA_ACCESSOR_HXX

#include "vigra/utilities.hxx"
#include "vigra/numerictraits.hxx"

namespace vigra {

namespace detail {

template <class T>
struct RequiresExplicitCast {
    template <class U>
    static U const & cast(U const & v)
        { return v; }
};

#define VIGRA_SPECIALIZED_CAST(type) \
    template <> \
    struct RequiresExplicitCast<type> { \
        static type cast(float const & v) \
            { return NumericTraits<type>::fromRealPromote(v); } \
        static type cast(double const & v) \
            { return NumericTraits<type>::fromRealPromote(v); } \
        template <class U> \
        static U cast(U v) \
            { return v; } \
 \
    };


VIGRA_SPECIALIZED_CAST(signed char)
VIGRA_SPECIALIZED_CAST(unsigned char)
VIGRA_SPECIALIZED_CAST(short)
VIGRA_SPECIALIZED_CAST(unsigned short)
VIGRA_SPECIALIZED_CAST(int)
VIGRA_SPECIALIZED_CAST(unsigned int)
VIGRA_SPECIALIZED_CAST(long)
VIGRA_SPECIALIZED_CAST(unsigned long)

template <>
struct RequiresExplicitCast<float> {
    template <class U>
    static U cast(U v)
        { return v; }
};

template <>
struct RequiresExplicitCast<double> {
    template <class U>
    static U cast(U v)
        { return v; }
};

#undef VIGRA_SPECIALIZED_CAST

} // namespace detail

/** \addtogroup DataAccessors Data Accessors

    Basic templates to encapsulate access to the data of an iterator.
    
    Data accessors are used to allow for flexible access to the data
    an interator points to. When we access the data directly, we
    are bound to what <TT>operator*()</TT> returns, if this method exists at 
    all. Encapsulating access in an accessor enables a better
    decoupling of data structures and algorithms. 
    <a href="documents/DataAccessors.ps">This paper</a> contains
    a detailed description of the concept.
*/
//@{

/********************************************************/
/*                                                      */
/*                     StandardAccessor                 */
/*                                                      */
/********************************************************/

/** \brief Encapsulate access to the values an iterator points to.

    StandardAccessor is a trivial accessor that simply encapsulates 
    the iterator's operator*() and operator[]() in its 
    read and write functions. It passes its arguments <em>by reference</em>. 
    If you want to return items by value, you 
    must use StandardValueAccessor instead of StandardAccessor.
    Both accessors have different optimization properties --
    StandardAccessor is usually faster for compound pixel types,
    while StandardValueAccessor is faster for the built-in types.

    When a floating point number is assigned by means of an accessor
    with integral value_type, the value is rounded and clipped as approriate.

    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>
    Namespace: vigra
*/
template <class VALUETYPE>
class StandardAccessor
{
  public:
        /** the value_type
        */
    typedef VALUETYPE value_type;
    
        /** read the current data item
        */
    template <class ITERATOR>
    VALUETYPE const & operator()(ITERATOR const & i) const { return *i; }
    
    VALUETYPE const & operator()(VALUETYPE const * i) const { return *i; }
    
        /** read the data item at an offset (can be 1D or 2D or higher order difference).
        */
    template <class ITERATOR, class DIFFERENCE>
    VALUETYPE const & operator()(ITERATOR & i, DIFFERENCE diff) const
    { 
        return i[diff]; 
    }
    
        /** Write the current data item. The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR>
    void set(V const & value, ITERATOR & i) const 
    { *i = detail::RequiresExplicitCast<VALUETYPE>::cast(value); }
    
        /** Write the data item at an offset (can be 1D or 2D or higher order difference)..
            The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR, class DIFFERENCE>
    void set(V const & value, ITERATOR & i, DIFFERENCE diff) const 
    { 
        i[diff]= detail::RequiresExplicitCast<VALUETYPE>::cast(value); 
    }
};

/** \brief Encapsulate access to the values an iterator points to.

    StandardValueAccessor is a trivial accessor that simply encapsulates 
    the iterator's operator*() and operator[]() in its 
    read and write functions. It passes its arguments <em>by value</em>. 
    If the iterator returns its items by reference (such as \ref vigra::ImageIterator), 
    you can also use StandardAccessor.
    These accessors have different optimization properties --
    StandardAccessor is usually faster for compound pixel types,
    while StandardValueAccessor is faster for the built-in types.
    
    When a floating point number is assigned by means of an accessor
    with integral value_type, the value is rounded and clipped as approriate.

    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>
    Namespace: vigra
*/
template <class VALUETYPE>
class StandardValueAccessor
{
  public:
        /** the value_type
        */
    typedef VALUETYPE value_type;
    
        /** Read the current data item. The type <TT>ITERATOR::reference</TT>
            is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class ITERATOR>
    VALUETYPE operator()(ITERATOR & i) const 
        { return detail::RequiresExplicitCast<VALUETYPE>::cast(*i); }
    
        /** Read the data item at an offset (can be 1D or 2D or higher order difference).
            The type <TT>ITERATOR::index_reference</TT>
            is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class ITERATOR, class DIFFERENCE>
    VALUETYPE operator()(ITERATOR & i, DIFFERENCE diff) const
    { 
        return detail::RequiresExplicitCast<VALUETYPE>::cast(i[diff]); 
    }
        /** Write the current data item. The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR>
    void set(V value, ITERATOR & i) const 
        { *i = detail::RequiresExplicitCast<VALUETYPE>::cast(value); }
    
        /** Write the data item at an offset (can be 1D or 2D or higher order difference)..
            The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR, class DIFFERENCE>
    void set(V value, ITERATOR & i, DIFFERENCE diff) const 
    { 
        i[diff]= detail::RequiresExplicitCast<VALUETYPE>::cast(value); 
    }
};

/********************************************************/
/*                                                      */
/*                StandardConstAccessor                 */
/*                                                      */
/********************************************************/

/** \brief Encapsulate read access to the values an iterator points to.

    StandardConstAccessor is a trivial accessor that simply encapsulates 
    the iterator's operator*() and operator[]() in its 
    read functions. It passes its arguments <em>by reference</em>. 
    If the iterator returns its items by value (such as \ref vigra::CoordinateIterator), you 
    must use StandardConstValueAccessor instead of StandardConstAccessor.
    Both accessors also have different optimization properties --
    StandardConstAccessor is usually faster for compound pixel types,
    while StandardConstValueAccessor is faster for the built-in types.

    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>
    Namespace: vigra
*/
template <class VALUETYPE>
class StandardConstAccessor
{
  public:
    typedef VALUETYPE value_type;
    
        /** read the current data item
        */
    template <class ITERATOR>
    VALUETYPE const & operator()(ITERATOR & i) const 
        { return *i; }
    
        /** read the data item at an offset (can be 1D or 2D or higher order difference).
        */
    template <class ITERATOR, class DIFFERENCE>
    VALUETYPE const & operator()(ITERATOR & i, DIFFERENCE diff) const
    { 
        return i[diff]; 
    }
};

/** \brief Encapsulate access to the values an iterator points to.

    StandardConstValueAccessor is a trivial accessor that simply encapsulates 
    the iterator's operator*() and operator[]() in its 
    read functions. It passes its arguments <em>by value</em>. 
    If the iterator returns its items by reference (such as \ref vigra::ConstImageIterator), 
    you can also use StandardConstAccessor.
    These accessors have different optimization properties --
    StandardConstAccessor is usually faster for compound pixel types,
    while StandardConstValueAccessor is faster for the built-in types.

    When an iterator passes a floating point number to an accessor
    with integral value_type, the value is rounded and clipped as approriate.

    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>
    Namespace: vigra
*/
template <class VALUETYPE>
class StandardConstValueAccessor
{
  public:
    typedef VALUETYPE value_type;
    
        /** Read the current data item. The type <TT>ITERATOR::reference</TT>
            is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class ITERATOR>
    VALUETYPE operator()(ITERATOR & i) const 
        { return detail::RequiresExplicitCast<VALUETYPE>::cast(*i); }
    
        /** Read the data item at an offset (can be 1D or 2D or higher order difference).
            The type <TT>ITERATOR::index_reference</TT>
            is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class ITERATOR, class DIFFERENCE>
    VALUETYPE operator()(ITERATOR & i, DIFFERENCE diff) const
    { 
        return detail::RequiresExplicitCast<VALUETYPE>::cast(i[diff]); 
    }
};

/********************************************************/
/*                                                      */
/*                 VectorComponentAccessor              */
/*                                                      */
/********************************************************/

/** \brief Accessor for one component of a vector.

    This accessor allows to select a single component (a single 'band')
    of a vector valued pixel type. The pixel type must support
    <TT>operator[]</TT>. The index of the component to be selected
    is passed in the constructor. The accessor returns its items 
    <em>by reference</em>. If you want to pass/return items by value,
    use VectorComponentValueAccessor. If a floating point number
    is assigned by means of an accessor with integral value_type, the
    value is rounded and clipped as appropriate. 

    <b>Usage:</b>
    
    \code
    vigra::BRGBImage image(w,h);
    
    // init red channel with 255
    initImage(destImageRange(image, 
                             VectorComponentAccessor<vigra::BRGBImage::value_type>(0)),
              255);
    \endcode
    
    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>
    Namespace: vigra
    
*/
template <class VECTORTYPE>
class VectorComponentAccessor
{
    int index_;
  public:
        /** the value_type
        */
    typedef typename VECTORTYPE::value_type value_type;
    
        /** determine the component to be accessed
        */
    VectorComponentAccessor(int index)
    : index_(index)
    {}
    
        /** read the current data item
        */
    template <class ITERATOR>
    value_type const & operator()(ITERATOR & i) const 
        { return (*i)[index_]; }
    
        /** read the data item at an offset (can be 1D or 2D or higher order difference).
        */
    template <class ITERATOR, class DIFFERENCE>
    value_type const & operator()(ITERATOR & i, DIFFERENCE diff) const
    { 
        return i[diff][index_]; 
    }
    
        /** Write the current data item. The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>value_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR>
    void set(V const & value, ITERATOR & i) const 
    { 
        (*i)[index_] = detail::RequiresExplicitCast<value_type>::cast(value); 
    }
    
        /** Write the data item at an offset (can be 1D or 2D or higher order difference)..
            The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>value_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR, class DIFFERENCE>
    void set(V const & value, ITERATOR & i, DIFFERENCE diff) const 
    { 
        i[diff][index_]= detail::RequiresExplicitCast<value_type>::cast(value); 
    }
};

/** \brief Accessor for one component of a vector.

    This accessor allows to select a single component (a single 'band')
    of a vector valued pixel type. The pixel type must support
    <TT>operator[]</TT>. The index of the component to be selected
    is passed in the constructor. The accessor returns its items 
    <em>by value</em>. If you want to pass/return items by reference,
    use VectorComponentAccessor. If a floating point number
    is assigned by means of an accessor with integral value_type, the
    value is rounded and clipped as appropriate. 

    <b>Usage:</b>
    
    \code
    vigra::BRGBImage image(w,h);
    
    // init red channel with 255
    initImage(destImageRange(image, 
                             VectorComponentValueAccessor<vigra::BRGBImage::value_type>(0)),
              255);
    \endcode
    
    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>
    Namespace: vigra
    
*/
template <class VECTORTYPE>
class VectorComponentValueAccessor
{
    int index_;
  public:
        /** the value_type
        */
    typedef typename VECTORTYPE::value_type value_type;
    
        /** determine the component to be accessed
        */
    VectorComponentValueAccessor(int index)
    : index_(index)
    {}
    
        /** Read the current data item.
            The type <TT>ITERATOR::index_reference::value_type</TT>
            is automatically converted to <TT>value_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class ITERATOR>
    value_type operator()(ITERATOR & i) const 
        { return detail::RequiresExplicitCast<value_type>::cast((*i)[index_]); }
    
        /** Read the data item at an offset (can be 1D or 2D or higher order difference).
            The type <TT>ITERATOR::index_reference::value_type</TT>
            is automatically converted to <TT>value_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class ITERATOR, class DIFFERENCE>
    value_type operator()(ITERATOR & i, DIFFERENCE diff) const
    { 
        return detail::RequiresExplicitCast<value_type>::cast(i[diff][index_]); 
    }
    
        /** Write the current data item. The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>value_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR>
    void set(V value, ITERATOR & i) const 
    { 
        (*i)[index_] = detail::RequiresExplicitCast<value_type>::cast(value); 
    }
    
        /** Write the data item at an offset (can be 1D or 2D or higher order difference)..
            The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>value_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR, class DIFFERENCE>
    void set(V value, ITERATOR & i, DIFFERENCE diff) const 
    { 
        i[diff][index_]= detail::RequiresExplicitCast<value_type>::cast(value); 
    }
};

/********************************************************/
/*                                                      */
/*                    SequenceAccessor                  */
/*                                                      */
/********************************************************/

/** \brief Accessor for items that are STL compatible sequences.

    It encapsulates access to the sequences' begin() and end()
    functions.
    
    <b>Usage:</b>

    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>
    Namespace: vigra
    
    \code
    typedef std::list<std::list<int> > ListOfLists;
    
    ListOfLists ll;
    ...
    
    typedef vigra::SequenceAccessor<ListOfLists::value_type> ListOfListsAccessor;
    ListOfListsAccessor a;
    for(ListOfLists::iterator li = ll.begin(); li != ll.end(); ++li) 
    {
        for(ListOfListsAccessor::iterator i = a.begin(li); i != a.end(li); ++i) 
        {
            *i = 10;
        }
    }
    \endcode
*/
template <class SEQUENCE>
class SequenceAccessor
: public StandardAccessor<SEQUENCE>
{
  public:
    /** the sequence's value_type
    */
    typedef typename SEQUENCE::value_type component_type;

    /** the sequence's iterator type
    */
    typedef typename SEQUENCE::iterator iterator;
    
    /** get begin iterator for sequence at given iterator position
    */
    template <class ITERATOR>
    iterator begin(ITERATOR & i) const
    { 
        return (*i).begin(); 
    }
    
    /** get end iterator for sequence at given iterator position
    */
    template <class ITERATOR>
    iterator end(ITERATOR & i)  const
    {
         return (*i).end(); 
    }
    
    /** get begin iterator for sequence at an offset
        of given iterator position
    */
    template <class ITERATOR, class DIFFERENCE>
    iterator begin(ITERATOR & i, DIFFERENCE diff)  const
    { 
        return i[diff].begin(); 
    }
    
    /** get end iterator for sequence at a 2D difference vector
        of given iterator position
    */
    template <class ITERATOR, class DIFFERENCE>
    iterator end(ITERATOR & i, DIFFERENCE diff)  const
    { 
        return i[diff].end(); 
    }

    /** get size of sequence at given iterator position
    */
    template <class ITERATOR>
    int size(ITERATOR & i) const { return (*i).size(); }

    /** get size of sequence at 2D difference vector of given iterator position
    */
    template <class ITERATOR, class DIFFERENCE>
    int size(ITERATOR & i, DIFFERENCE diff) const { return i[diff].size(); }
};

/********************************************************/
/*                                                      */
/*                     VectorAccessor                   */
/*                                                      */
/********************************************************/

/** \brief Accessor for items that are STL compatible vectors.

    It encapsulates access to a vector's access functionality.
    
    <b> Usage:</b>
    
    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>    
    Namespace: vigra
        
    The accessor has two modes of operation:
    
    <ol>
    <li> Access the vector's iterator via the <TT>begin()</TT> and <TT>end()</TT>
    functions:
    
    \code
    typedef std::list<std::vector<int> > ListOfVectors;
    
    ListOfVectors ll;
    ...
    
    typedef vigra::SequenceAccessor<ListOfVectors::value_type> ListOfVectorsAccessor;
    ListOfVectorsAccessor a;
    for(ListOfVectors::iterator li = ll.begin(); li != ll.end(); ++li) 
    {
        for(ListOfVectorsAccessor::iterator i = a.begin(li); i != a.end(li); ++i) 
        {
            *i = 10;
        }
    }
    \endcode
    <li> Access the vector's components via an index (internally calls 
    the vector's <TT>operator[]</TT> ):
    \code
    typedef std::list<std::vector<int> > ListOfVectors;
    
    ListOfVectors ll;
    ...
    
    typedef vigra::SequenceAccessor<ListOfVectors::value_type> ListOfVectorsAccessor;
    ListOfVectorsAccessor a;
    for(ListOfVectors::iterator li = ll.begin(); li != ll.end(); ++li) 
    {
        for(int i = 0; i != a.size(li); ++i) 
        {
            a.setComponent(10, li, i);
        }
    }
    \endcode
    </ol>
    
    <b> Required Interface:</b>
    
    \code
    VECTOR v;
    VECTOR::iterator i;
    value_type d;
    int index;
    
    d = v[index];
    v[index] = d;
    i = v.begin();
    i = v.end();
    v.size();
    \endcode
*/
template <class VECTOR>
class VectorAccessor
: public SequenceAccessor<VECTOR>
{
  public:
        /** the vector's value_type
        */
    typedef typename VECTOR::value_type component_type;

        /** Read the component data at given vector index
            at given iterator position 
        */
    template <class ITERATOR>
    component_type const & getComponent(ITERATOR & i, int idx) const 
    { 
        return (*i)[idx]; 
    }
    
        /** Set the component data at given vector index
            at given iterator position. The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>component_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR>
    void setComponent(V const & value, ITERATOR & i, int idx) const
    { 
        (*i)[idx] = detail::RequiresExplicitCast<component_type>::cast(value); 
    }
    
        /** Read the component data at given vector index
            at an offset of given iterator position
        */
    template <class ITERATOR, class DIFFERENCE>
    component_type const & getComponent(ITERATOR & i, DIFFERENCE diff, int idx) const
    { 
        return i[diff][idx]; 
    }
    
    /** Set the component data at given vector index
        at an offset of given iterator position. The type <TT>V</TT> of the passed
        in <TT>value</TT> is automatically converted to <TT>component_type</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
    */
    template <class V, class ITERATOR, class DIFFERENCE>
    void 
    setComponent(V const & value, ITERATOR & i, DIFFERENCE diff, int idx) const 
    { 
        i[diff][idx] = detail::RequiresExplicitCast<component_type>::cast(value); 
    }
};


/********************************************************/
/*                                                      */
/*                  InterpolatingAccessor               */
/*                                                      */
/********************************************************/

/** \brief Bilinear interpolation at non-integer positions.

    This accessor allows an image be accessed at arbitrary non-integer
    coordinates and performs an bi-linear interpolation to
    obtain a pixel value.
    It uses the given ACCESSOR (which is usually the
    accessor originally associated with the iterator)
    to access data.
    
    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"
    Namespace: vigra
    
    <b> Required Interface:</b>
    
    \code
    ITERATOR iter;
    ACCESSOR a;
    VALUETYPE destvalue;
    float s;
    int x, y;
    
    destvalue = s * a(iter, x, y) + s * a(iter, x, y);
    
    \endcode
*/
template <class ACCESSOR, class VALUETYPE>
class BilinearInterpolatingAccessor
{
  public:
    /** the iterators' pixel type
    */
    typedef VALUETYPE value_type;
    
    /** init from given accessor
    */
    BilinearInterpolatingAccessor(ACCESSOR a)
    : a_(a)
    {}
    
    /** Interpolate the data item at a non-integer position.
        Ensure that no outside pixels are accessed if 
        (x, y) is near the image border (as long as
        0 <= x <= width-1, 0 <= y <= height-1).
    */
    template <class ITERATOR>
    value_type operator()(ITERATOR & i, float x, float y) const 
    { 
        int ix = x;
        int iy = y;
        float dx = x - ix;
        float dy = y - iy;
        
        value_type ret;
        
        // avoid dereferencing the iterator outside its range
        if(dx == 0.0)
        {
            if(dy == 0.0)
            {
                ret = a_(i, Diff2D(ix, iy));
            }
            else
            {
                ret = (1.0 - dy) * a_(i, Diff2D(ix, iy)) +
                  dy * a_(i, Diff2D(ix, iy + 1));
            }
        }
        else
        {
            if(dy == 0.0)
            {
                ret = (1.0 - dx) * a_(i, Diff2D(ix, iy)) + 
                  dx * a_(i, Diff2D(ix + 1, iy));
            }
            else
            {
                ret = (1.0 - dx) * (1.0 - dy) * a_(i, Diff2D(ix, iy)) +
                  dx * (1.0 - dy) * a_(i, Diff2D(ix + 1, iy)) +
                  (1.0 - dx) * dy * a_(i, Diff2D(ix, iy + 1)) +
                  dx * dy * a_(i, Diff2D(ix + 1, iy + 1));
            }
        }
            
        return ret;
    }

    /** Interpolate the data item at a non-integer position.
        This function works as long as 0 <= x < width-1, 
        0 <= y < height-1. It is slightly faster than <TT>operator()</TT>.
    */
    template <class ITERATOR>
    value_type unchecked(ITERATOR & i, float x, float y) const 
    { 
    int ix = x;
        int iy = y;
        float dx = x - ix;
        float dy = y - iy;
        return (1.0 - dx) * (1.0 - dy) * a_(i, Diff2D(ix, iy)) +
               dx * (1.0 - dy) * a_(i, Diff2D(ix + 1, iy)) +
               (1.0 - dx) * dy * a_(i, Diff2D(ix, iy + 1)) +
               dx * dy * a_(i, Diff2D(ix + 1, iy + 1));
    }
    
  private:
    ACCESSOR a_;
};

/********************************************************/
/*                                                      */
/*                 MultiImageAccessor2                  */
/*                                                      */
/********************************************************/

/** \brief Access two images simultaneously.

    This accessor is used when two images need to be treated as one
    because an algorithm accepts only one image. For example, 
    \ref seededRegionGrowing() uses only one image two calculate
    the cost for aggregating each pixel into a region. Somtimes, we
    need more information to calcuate this cost, for example gray value
    and local gradient magnitude. These values can be stored in two images,
    which appear as only one when we pass a <TT>MultiImageAccessor2</TT> to
    the lagorithms. Of course, the cost functor must accept a <TT>pair</TT> 
    of values for this to work. Instead of an actual image iterator, we
    pass a <a href="CoordinateIterator.html">CoordinateIterator</a> which 
    selects the right pixels form both images.
    
    <b> Usage:</b>

    <b>\#include</b> "<a href="accessor_8hxx-source.html">vigra/accessor.hxx</a>"<br>    
    Namespace: vigra
    
    \code
    using namespace vigra;
    
    FImage gray_values(w,h), gradient_magnitude(w,h);
    IImage seeds(w,h), labels(w,h);
    
    seededRegionGrowing(
        srcIterRange(CoordinateIterator(), CoordinateIterator(w,h),
           MultiImageAccessor2<FImage::iterator, FImage::Accessor,
                               FImage::iterator, FImage::Accessor>
                              (gray_values.upperLeft(), gray_values.accessor(),
                               gradient_magnitude.upperLeft(), gradient_magnitude.accessor())), 
        srcImage(seeds), 
        destImage(labels), 
        SomeCostFunctor());       
    \endcode   
*/

template <class Iter1, class Acc1, class Iter2, class Acc2>
class MultiImageAccessor2
{
  public:
        /** The accessors value_type: construct a pair that contains
            the corresponding image values.
        */
    typedef pair<typename Acc1::value_type, typename Acc2::value_type>
            value_type;
    
        /** Construct from two image iterators and associated accessors.
        */
    MultiImageAccessor2(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
    : i1_(i1), a1_(a1), i2_(i2), a2_(a2)
    {}

        /** read the current data item
        */
    template <class DIFFERENCE>
    value_type operator()(DIFFERENCE d) const
    { 
        return std::make_pair(a1_(i1_, d), a2_(i2_, i.x, i.y)); 
    }
    
        /** read the data item at an offset
        */
    template <class DIFFERENCE1, class DIFFERENCE2>
    value_type operator()(DIFFERENCE1 d1, DIFFERENCE2 d2) const
    { 
        d2 += d1;
        return std::make_pair(a1_(i1_, d2), a2_(i2_, d2)); 
    }
    
  private:
    Iter1 i1_;
    Acc1 a1_;
    Iter2 i2_;
    Acc2 a2_;
};

//@}

} // namespace vigra

#endif // VIGRA_ACCESSOR_HXX
