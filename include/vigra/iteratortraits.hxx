/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2002 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.2.0, Aug 07 2003 )                                    */
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
 
 
#ifndef VIGRA_ITERATORTRAITS_HXX
#define VIGRA_ITERATORTRAITS_HXX

#include <vigra/accessor.hxx>
#include <vigra/imageiteratoradapter.hxx>

namespace vigra {

/** \addtogroup ImageIterators
*/
//@{
/** \brief Export associated information for each image iterator.

    The IteratorTraits class contains the following fields:

    \code
    template <class T> 
    struct IteratorTraits 
    {
        typedef T                                     Iterator;
        typedef Iterator                              iterator;
        typedef typename iterator::iterator_category  iterator_category;
        typedef typename iterator::value_type         value_type;
        typedef typename iterator::reference          reference;
        typedef typename iterator::index_reference    index_reference;
        typedef typename iterator::pointer            pointer;
        typedef typename iterator::difference_type    difference_type;
        typedef typename iterator::row_iterator       row_iterator;
        typedef typename iterator::column_iterator    column_iterator;
        typedef StandardAccessor<value_type>          DefaultAccessor;
        typedef StandardAccessor<value_type>          default_accessor;
    };
    \endcode
    
    By (partially) specializing this template for an iterator class
    the defaults given above can be changed as approiate. For example, iterators
    for rgb images are associated with <TT>RGBAccessor<value_type></TT> 
    instead of <TT>StandardAccessor<value_type></TT>. To get the accessor
    associated with a given iterator, use code like this:
    
    \code
    template <class Iterator>
    void foo(Iterator i)
    {
        typedef typename IteratorTraits<Iterator>::DefaultAccessor Accessor;
        Accessor a;
        ...
    }
    \endcode
    
    This technique is, for example, used by the 
    \ref IteratorBasedArgumentObjectFactories. The possibility to retrieve the default accessor by means of a traits
    class is especially important since this information is not
    contained in the iterator directly.
    
    <b>\#include</b> "<a href="iteratortraits_8hxx-source.html">vigra/iteratortraits.hxx</a>"
    Namespace: vigra
*/
template <class T> 
struct IteratorTraits 
{
    typedef T                                     Iterator;
    typedef Iterator                              iterator;
    typedef typename iterator::iterator_category  iterator_category;
    typedef typename iterator::value_type         value_type;
    typedef typename iterator::reference          reference;
    typedef typename iterator::index_reference    index_reference;
    typedef typename iterator::pointer            pointer;
    typedef typename iterator::difference_type    difference_type;
    typedef typename iterator::row_iterator       row_iterator;
    typedef typename iterator::column_iterator    column_iterator;
    typedef StandardAccessor<value_type>          DefaultAccessor;
    typedef StandardAccessor<value_type>          default_accessor;
};

/***********************************************************/

/** \page ArgumentObjectFactories Argument Object Factories
    
    Factory functions to create argument objects which simplify long argument lists.

    <DL>
    <DT>
        <IMG BORDER=0 ALT="-" SRC="documents/bullet.gif"> 
        \ref ImageBasedArgumentObjectFactories
        <DD>
    <DT>
        <IMG BORDER=0 ALT="-" SRC="documents/bullet.gif"> 
        \ref IteratorBasedArgumentObjectFactories
        <DD>
    </DL>

    Long argument lists provide for greater flexibility of functions,
    but they are also tedious and error prone, when we don't need
    the flexibility. Thus, we define argument objects which
    automatically provide reasonable defaults for those arguments that we 
    didn't specify explicitly. 
    
    The argument objects are created via a number of factory functions.
    Since these functions have descriptive names, they also serve
    to improve readability: the name of each factory tells te purpose of its
    argument object. 
    
    Consider the following example. Without argument objects we had to 
    write something like this (cf. \ref copyImageIf()):
    
    \code
    vigra::BImage img1, img2, img3;
    
    // fill img1 and img2 ...
    
    vigra::copyImageIf(img1.upperLeft(), img1.lowerRight(), img1.accessor(),
                img2.upperLeft(), img2.accessor(),
                img3.upperLeft(), img3.accessor());
    \endcode
    
    Using the argument object factories, this becomes much shorter and
    more readable:
    
    \code
    vigra::copyImageIf(srcImageRange(img1),
                maskImage(img2),
                destImage(img3));
    \endcode
    
    The names of the factories clearly tell which image is source, mask, 
    and destination. In addition, the suffix <TT>Range</TT> must be used 
    for those argument objects that need to specify the lower right
    corner of the region of interest. Typically, this is only the first
    source argument, but sometimes the first destiniation argument must 
    also contain a range.
    
    The factory functions come in two flavours: Iterator based and 
    image based factories. Above we have seen the image based variant.
    The iterator based variant would look like this:
    
    \code
    vigra::copyImageIf(srcIterRange(img1.upperLeft(), img1.lowerRight()),
                maskIter(img2.upperLeft()),
                destIter(img3.upperLeft()));
    \endcode
    
    These factory functions contain the word <TT>Iter</TT> instead of the word 
    <TT>Image</TT>,  They would normally be used if we couldn't access the 
    images (for example, within a function which got passed iterators)
    or if we didn't want to operate on the entire image. The default 
    accessor is obtained via \ref vigra::IteratorTraits.
    
    All factory functions also allow to specify accessors explicitly. This
    is useful if we can't use the default accessor. This variant looks 
    like this:
    
    \code
    vigra::copyImageIf(srcImageRange(img1),
                maskImage(img2, MaskPredicateAccessor()),
                destImage(img3));    
    \endcode
    
    or
    
    \code
    vigra::copyImageIf(srcIterRange(img1.upperLeft(), img1.lowerRight()),
                maskIter(img2.upperLeft(), MaskPredicateAccessor()),
                destIter(img3.upperLeft()));
    \endcode
    
    All versions can be mixed freely within one explession.
    Technically, the argument objects are simply defined as 
    pairs and triples of iterators and accessor so that all algorithms 
    should declare a call interface version based on pairs and triples 
    (see for example \ref copyImageIf()).

  \section ImageBasedArgumentObjectFactories Image Based Argument Object Factories
        
    <b>Include:</b> automatically included with the image classes<br>
    Namespace: vigra
    
    These factories can be used to create argument objects when we 
    are given instances or subclasses of \ref vigra::BasicImage (see
    \ref StandardImageTypes for instances defined per default).
    These factory functions access <TT>img.upperLeft()</TT>, 
    <TT>img.lowerRight()</TT>, and <TT>img.accessor()</TT> to obtain the iterators
    and accessor for the given image (unless the accessor is 
    given explicitly). The following factory functions are provided:
    
    <table>
    <tr><td>
        \htmlonly
        <th bgcolor="#f0e0c0" colspan=2 align=left>
        \endhtmlonly
        <TT>\ref vigra::BasicImage "vigra::BasicImage<SomeType>" img;</TT>
        \htmlonly
        </th>
        \endhtmlonly
    </td></tr>
    <tr><td>
        
    <TT>srcImageRange(img)</TT>
    </td><td>
        create argument object containing upper left, lower right, and
        default accessor of source image
        
    </td></tr>
    <tr><td>
        
    <TT>srcImageRange(img, SomeAccessor())</TT>
    </td><td>
        create argument object containing upper left, lower right
        of source image, and given accessor
        
    </td></tr>
    <tr><td>
        
    <TT>srcImage(img)</TT>
    </td><td>
        create argument object containing upper left, and
        default accessor of source image
        
    </td></tr>
    <tr><td>
        
    <TT>srcImage(img, SomeAccessor())</TT>
    </td><td>
        create argument object containing upper left
        of source image, and given accessor
        
    </td></tr>
    <tr><td>
    
    <TT>maskImage(img)</TT>
    </td><td>
        create argument object containing upper left, and
        default accessor of mask image
        
    </td></tr>
    <tr><td>
        
    <TT>maskImage(img, SomeAccessor())</TT>
    </td><td>
        create argument object containing upper left
        of mask image, and given accessor
        
    </td></tr>
    <tr><td>
    
    <TT>destImageRange(img)</TT>
    </td><td>
        create argument object containing upper left, lower right, and
        default accessor of destination image
        
    </td></tr>
    <tr><td>
        
    <TT>destImageRange(img, SomeAccessor())</TT>
    </td><td>
        create argument object containing upper left, lower right
        of destination image, and given accessor
        
    </td></tr>
    <tr><td>
        
    <TT>destImage(img)</TT>
    </td><td>
        create argument object containing upper left, and
        default accessor of destination image
        
    </td></tr>
    <tr><td>
        
    <TT>destImage(img, SomeAccessor())</TT>
    </td><td>
        create argument object containing upper left
        of destination image, and given accessor
        
    </td></tr>
    </table>


  \section IteratorBasedArgumentObjectFactories Iterator Based Argument Object Factories
        
    <b>\#include</b> "<a href="iteratortraits_8hxx-source.html">vigra/iteratortraits.hxx</a>"
    Namespace: vigra
    
    These factories can be used to create argument objects when we 
    are given \ref ImageIterators.
    These factory functions use \ref vigra::IteratorTraits to
    get the default accessor for the given iterator unless the 
    accessor is given explicitly. The following factory functions 
    are provided:
    
    <table>
    <tr><td>
        \htmlonly
        <th bgcolor="#f0e0c0" colspan=2 align=left>
        \endhtmlonly
        <TT>\ref vigra::BasicImage::Iterator "vigra::BasicImage<SomeType>::Iterator" i1, i2;</TT>
        \htmlonly
        </th>
        \endhtmlonly
    </td></tr>
    <tr><td>
        
    <TT>srcIterRange(i1, i2)</TT>
    </td><td>
        create argument object containing the given iterators and
        corresponding default accessor (for source image)
        
    </td></tr>
    <tr><td>
        
    <TT>srcIterRange(i1, i2, SomeAccessor())</TT>
    </td><td>
        create argument object containing given iterators and
        accessor (for source image)
        
    </td></tr>
    <tr><td>
        
    <TT>srcIter(i1)</TT>
    </td><td>
        create argument object containing the given iterator and
        corresponding default accessor (for source image)
        
    </td></tr>
    <tr><td>
        
    <TT>srcIter(i1, SomeAccessor())</TT>
    </td><td>
        create argument object containing given iterator and
        accessor (for source image)
        
    </td></tr>
    <tr><td>
    
    <TT>maskIter(i1)</TT>
    </td><td>
        create argument object containing the given iterator and
        corresponding default accessor (for mask image)
        
    </td></tr>
    <tr><td>
        
    <TT>maskIter(i1, SomeAccessor())</TT>
    </td><td>
        create argument object containing given iterator and
        accessor (for mask image)
        
    </td></tr>
    <tr><td>
    
    <TT>destIterRange(i1, i2)</TT>
    </td><td>
        create argument object containing the given iterators and
        corresponding default accessor (for destination image)
        
    </td></tr>
    <tr><td>
        
    <TT>destIterRange(i1, i2, SomeAccessor())</TT>
    </td><td>
        create argument object containing given iterators and
        accessor (for destination image)
        
    </td></tr>
    <tr><td>
        
    <TT>destIter(i1)</TT>
    </td><td>
        create argument object containing the given iterator and
        corresponding default accessor (for destination image)
        
    </td></tr>
    <tr><td>
        
    <TT>destIter(i1, SomeAccessor())</TT>
    </td><td>
        create argument object containing given iterator and
        accessor (for destination image)
        
    </td></tr>
    </table>
*/

template <class Iterator, class Accessor>
inline triple<Iterator, Iterator, Accessor>
srcIterRange(Iterator const & upperleft, Iterator const & lowerright, Accessor a)
{
    return triple<Iterator, Iterator, Accessor>(upperleft, lowerright, a);
}

template <class Iterator, class Accessor>
inline pair<Iterator, Accessor>
srcIter(Iterator const & upperleft, Accessor a)
{
    return pair<Iterator, Accessor>(upperleft, a);
}

template <class Iterator, class Accessor>
inline pair<Iterator, Accessor>
maskIter(Iterator const & upperleft, Accessor a)
{
    return pair<Iterator, Accessor>(upperleft, a);
}

template <class Iterator, class Accessor>
inline pair<Iterator, Accessor>
destIter(Iterator const & upperleft, Accessor a)
{
    return pair<Iterator, Accessor>(upperleft, a);
}


template <class Iterator, class Accessor>
inline triple<Iterator, Iterator, Accessor>
destIterRange(Iterator const & upperleft, Iterator const & lowerright, Accessor a)
{
    return triple<Iterator, Iterator, Accessor>(upperleft, lowerright, a);
}

template <class Iterator>
inline pair<Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>
srcIter(Iterator const & upperleft)
{
    return pair<Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>(
                  upperleft,
                  IteratorTraits<Iterator>::DefaultAccessor());
}

template <class Iterator>
inline triple<Iterator, Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>
srcIterRange(Iterator const & upperleft, Iterator const & lowerright)
{
    return triple<Iterator, Iterator, 
                  typename IteratorTraits<Iterator>::DefaultAccessor>(
                  upperleft, lowerright, 
                  IteratorTraits<Iterator>::DefaultAccessor());
}

template <class Iterator>
inline pair<Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>
maskIter(Iterator const & upperleft)
{
    return pair<Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>(
                  upperleft,
                  IteratorTraits<Iterator>::DefaultAccessor());
}

template <class Iterator>
inline pair<Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>
destIter(Iterator const & upperleft)
{
    return pair<Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>(
                  upperleft,
                  IteratorTraits<Iterator>::DefaultAccessor());
}

template <class Iterator>
inline triple<Iterator, Iterator, typename IteratorTraits<Iterator>::DefaultAccessor>
destIterRange(Iterator const & upperleft, Iterator const & lowerright)
{
    return triple<Iterator, Iterator, 
                  typename IteratorTraits<Iterator>::DefaultAccessor>(
                  upperleft, lowerright, 
                  IteratorTraits<Iterator>::DefaultAccessor());
}

//@}

} // namespace vigra

#endif // VIGRA_ITERATORTRAITS_HXX
