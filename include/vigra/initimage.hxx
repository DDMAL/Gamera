/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2002 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.5.0, Dec 07 2006 )                                    */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de          or                  */
/*        vigra@kogs1.informatik.uni-hamburg.de                         */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */                
/*                                                                      */
/************************************************************************/
 
 
#ifndef VIGRA_INITIMAGE_HXX
#define VIGRA_INITIMAGE_HXX

#include "utilities.hxx"
#include "iteratortraits.hxx"
#include "functortraits.hxx"

namespace vigra {

/** \addtogroup InitAlgo Algorithms to Initialize Images
    
    Init images or image borders
*/
//@{

/********************************************************/
/*                                                      */
/*                       initLine                       */
/*                                                      */
/********************************************************/

template <class DestIterator, class DestAccessor, class VALUETYPE>
void
initLineImpl(DestIterator d, DestIterator dend, DestAccessor dest,
             VALUETYPE v, VigraFalseType)
{
    for(; d != dend; ++d)
        dest.set(v, d);
}

template <class DestIterator, class DestAccessor, class FUNCTOR>
void
initLineImpl(DestIterator d, DestIterator dend, DestAccessor dest,
             FUNCTOR const & f, VigraTrueType)
{
    for(; d != dend; ++d)
        dest.set(f(), d);
}

template <class DestIterator, class DestAccessor, class VALUETYPE>
inline void
initLine(DestIterator d, DestIterator dend, DestAccessor dest,
         VALUETYPE v)
{
    initLineImpl(d, dend, dest, v, typename FunctorTraits<VALUETYPE>::isInitializer());
}

template <class DestIterator, class DestAccessor, class FUNCTOR>
inline void
initLineFunctor(DestIterator d, DestIterator dend, DestAccessor dest,
         FUNCTOR f)
{
    initLineImpl(d, dend, dest, f, VigraTrueType());
}

template <class DestIterator, class DestAccessor, 
          class MaskIterator, class MaskAccessor, 
          class VALUETYPE>
void
initLineIfImpl(DestIterator d, DestIterator dend, DestAccessor dest,
               MaskIterator m, MaskAccessor mask,
               VALUETYPE v, VigraFalseType)
{
    for(; d != dend; ++d, ++m)
        if(mask(m))
            dest.set(v, d);
}

template <class DestIterator, class DestAccessor, 
          class MaskIterator, class MaskAccessor, 
          class FUNCTOR>
void
initLineIfImpl(DestIterator d, DestIterator dend, DestAccessor dest,
               MaskIterator m, MaskAccessor mask,
               FUNCTOR const & f, VigraTrueType)
{
    for(; d != dend; ++d, ++m)
        if(mask(m))
            dest.set(f(), d);
}

template <class DestIterator, class DestAccessor, 
          class MaskIterator, class MaskAccessor, 
          class VALUETYPE>
inline void
initLineIf(DestIterator d, DestIterator dend, DestAccessor dest,
           MaskIterator m, MaskAccessor mask,
           VALUETYPE v)
{
    initLineIfImpl(d, dend, dest, m, mask, v, typename FunctorTraits<VALUETYPE>::isInitializer());
}

template <class DestIterator, class DestAccessor, 
          class MaskIterator, class MaskAccessor, 
          class FUNCTOR>
void
initLineFunctorIf(DestIterator d, DestIterator dend, DestAccessor dest,
                  MaskIterator m, MaskAccessor mask,
                  FUNCTOR f)
{
    initLineIfImpl(d, dend, dest, m, mask, f, VigraTrueType());
}

/********************************************************/
/*                                                      */
/*                        initImage                     */
/*                                                      */
/********************************************************/

/** \brief Write a value to every pixel in an image or rectangular ROI.

    This function can be used to init the image.
    It uses an accessor to access the pixel data.
    
    <b> Declarations:</b>
    
    pass arguments explicitly:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, class VALUETYPE>
        void
        initImage(ImageIterator upperleft, ImageIterator lowerright, 
              Accessor a, VALUETYPE v)
    }
    \endcode

    use argument objects in conjunction with \ref ArgumentObjectFactories:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, class VALUETYPE>
        void
        initImage(triple<ImageIterator, ImageIterator, Accessor> img, VALUETYPE v)
    }
    \endcode
    
    <b> Usage:</b>
    
        <b>\#include</b> "<a href="initimage_8hxx-source.html">vigra/initimage.hxx</a>"<br>
        Namespace: vigra
    
    \code
    vigra::BImage img(100, 100);
    
    // zero the image
    vigra::initImage(destImageRange(img),
                     vigra::NumericTraits<vigra::BImage::PixelType>::zero());
    \endcode

    <b> Required Interface:</b>
    
    \code
    ImageIterator upperleft, lowerright;
    ImageIterator::row_iterator ix = upperleft.rowIterator();
    
    Accessor accessor;
    VALUETYPE v;
    
    accessor.set(v, ix); 
    \endcode
    
*/
template <class ImageIterator, class Accessor, class VALUETYPE>
void
initImage(ImageIterator upperleft, ImageIterator lowerright, 
          Accessor a,  VALUETYPE v)
{
    int w = lowerright.x - upperleft.x;
    
    for(; upperleft.y < lowerright.y; ++upperleft.y)
    {
        initLine(upperleft.rowIterator(), 
                 upperleft.rowIterator() + w, a, v);
    }
}
    
template <class ImageIterator, class Accessor, class VALUETYPE>
inline 
void
initImage(triple<ImageIterator, ImageIterator, Accessor> img, VALUETYPE v)
{
    initImage(img.first, img.second, img.third, v);
}
    
/********************************************************/
/*                                                      */
/*                        initImage                     */
/*                                                      */
/********************************************************/

/** \brief Write the result of a functor call to every pixel in an image or rectangular ROI.

    This function can be used to init the image by calling the given 
    functor for each pixel.
    It uses an accessor to access the pixel data.
    
    <b> Declarations:</b>
    
    pass arguments explicitly:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, class FUNCTOR>
        void
        initImageWithFunctor(ImageIterator upperleft, ImageIterator lowerright, 
                  Accessor a,  FUNCTOR f);
    }
    \endcode

    use argument objects in conjunction with \ref ArgumentObjectFactories:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, class FUNCTOR>
        void
        initImageWithFunctor(triple<ImageIterator, ImageIterator, Accessor> img, FUNCTOR f);
    }
    \endcode
    
    <b> Usage:</b>
    
        <b>\#include</b> "<a href="initimage_8hxx-source.html">vigra/initimage.hxx</a>"<br>
        Namespace: vigra
    
    \code
    struct Counter {
        Counter() : count(0) {}
        
        int operator()() const { return count++; }
    
        mutable int count;
    };
    
    vigra::IImage img(100, 100);
    
    // write the current count in every pixel
    vigra::initImageWithFunctor(destImageRange(img), Counter());
    \endcode

    <b> Required Interface:</b>
    
    \code
    ImageIterator upperleft, lowerright;
    ImageIterator::row_iterator ix = upperleft.rowIterator();
    
    Accessor accessor;
    Functor f;
    
    accessor.set(f(), ix); 
    \endcode
    
*/
template <class ImageIterator, class Accessor, class FUNCTOR>
void
initImageWithFunctor(ImageIterator upperleft, ImageIterator lowerright, 
          Accessor a,  FUNCTOR f)
{
    int w = lowerright.x - upperleft.x;
    
    for(; upperleft.y < lowerright.y; ++upperleft.y)
    {
        initLineFunctor(upperleft.rowIterator(), 
                 upperleft.rowIterator() + w, a, f);
    }
}
    
template <class ImageIterator, class Accessor, class FUNCTOR>
inline 
void
initImageWithFunctor(triple<ImageIterator, ImageIterator, Accessor> img, FUNCTOR f)
{
    initImageWithFunctor(img.first, img.second, img.third, f);
}
    
/********************************************************/
/*                                                      */
/*                      initImageIf                     */
/*                                                      */
/********************************************************/

/** \brief Write value to pixel in the image if mask is true.

    This function can be used to init a region-of-interest of the image.
    It uses an accessor to access the pixel data.
    
    <b> Declarations:</b>
    
    pass arguments explicitly:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, 
              class MaskImageIterator, class MaskAccessor,
              class VALUETYPE>
        void
        initImageIf(ImageIterator upperleft, ImageIterator lowerright, Accessor a,
              MaskImageIterator mask_upperleft, MaskAccessor ma,
              VALUETYPE v)
    }
    \endcode    
    
    use argument objects in conjunction with \ref ArgumentObjectFactories:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, 
              class MaskImageIterator, class MaskAccessor,
              class VALUETYPE>
        void
        initImageIf(triple<ImageIterator, ImageIterator, Accessor> img, 
            pair<MaskImageIterator, MaskAccessor> mask,
            VALUETYPE v)
    }
    \endcode
    
    <b> Usage:</b>
    
        <b>\#include</b> "<a href="initimage_8hxx-source.html">vigra/initimage.hxx</a>"<br>
        Namespace: vigra
    
    \code
    vigra::BImage img(100, 100);
    vigra::BImage mask(100, 100);
    
    // zero the ROI
    vigra::initImageIf(destImageRange(img), 
                maskImage(mask),
                vigra::NumericTraits<vigra::BImage::PixelType>::zero());
    \endcode

    <b> Required Interface:</b>
    
    \code
    ImageIterator upperleft, lowerright;
    MaskImageIterator mask_upperleft;
    ImageIterator::row_iterator ix = upperleft.rowIterator();
    MaskImageIterator::row_iterator mx = mask_upperleft.rowIterator();
    
    Accessor accessor;
    MaskAccessor mask_accessor;
    VALUETYPE v;
    
    if(mask_accessor(mx)) accessor.set(v, ix); 
    \endcode
    
*/
template <class ImageIterator, class Accessor, 
          class MaskImageIterator, class MaskAccessor,
          class VALUETYPE>
void
initImageIf(ImageIterator upperleft, ImageIterator lowerright, Accessor a,
          MaskImageIterator mask_upperleft, MaskAccessor ma,
          VALUETYPE v)
{
    int w = lowerright.x - upperleft.x;
        
    for(; upperleft.y < lowerright.y; ++upperleft.y, ++mask_upperleft.y)
    {
        initLineIf(upperleft.rowIterator(), 
                   upperleft.rowIterator() + w, a, 
                   mask_upperleft.rowIterator(), ma, v);
    }
}
    
template <class ImageIterator, class Accessor, 
          class MaskImageIterator, class MaskAccessor,
          class VALUETYPE>
inline 
void
initImageIf(triple<ImageIterator, ImageIterator, Accessor> img, 
            pair<MaskImageIterator, MaskAccessor> mask,
            VALUETYPE v)
{
    initImageIf(img.first, img.second, img.third, mask.first, mask.second, v);
}
    
/********************************************************/
/*                                                      */
/*                    initImageBorder                   */
/*                                                      */
/********************************************************/

/** \brief Write value to the specified border pixels in the image.

    A pixel is initialized if its distance to the border 
    is at most 'borderwidth'.
    It uses an accessor to access the pixel data.
    
    <b> Declarations:</b>
    
    pass arguments explicitly:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, class VALUETYPE>
        void
        initImageBorder(ImageIterator upperleft, ImageIterator lowerright, 
                Accessor a,  int border_width, VALUETYPE v)
    }
    \endcode

    use argument objects in conjunction with \ref ArgumentObjectFactories:
    \code
    namespace vigra {
        template <class ImageIterator, class Accessor, class VALUETYPE>
        void
        initImageBorder(triple<ImageIterator, ImageIterator, Accessor> img, 
                int border_width, VALUETYPE v)
    }
    \endcode
    
    <b> Usage:</b>
    
        <b>\#include</b> "<a href="initimage_8hxx-source.html">vigra/initimage.hxx</a>"<br>
        Namespace: vigra
    
    \code
    vigra::BImage img(100, 100);
    
    // zero a border of 5 pixel
    vigra::initImageBorder(destImageRange(img),
                    5, vigra::NumericTraits<vigra::BImage::PixelType>::zero());
    \endcode

    <b> Required Interface:</b>
    
    see \ref initImage()
    
*/
template <class ImageIterator, class Accessor, class VALUETYPE>
inline 
void
initImageBorder(ImageIterator upperleft, ImageIterator lowerright, 
                Accessor a,  int border_width, VALUETYPE v)
{
    int w = lowerright.x - upperleft.x;
    int h = lowerright.y - upperleft.y;
    
    int hb = (border_width > h) ? h : border_width;
    int wb = (border_width > w) ? w : border_width;
    
    initImage(upperleft, upperleft+Diff2D(w,hb), a, v);
    initImage(upperleft, upperleft+Diff2D(wb,h), a, v);
    initImage(upperleft+Diff2D(0,h-hb), lowerright, a, v);
    initImage(upperleft+Diff2D(w-wb,0), lowerright, a, v);
}
    
template <class ImageIterator, class Accessor, class VALUETYPE>
inline 
void
initImageBorder(triple<ImageIterator, ImageIterator, Accessor> img, 
                int border_width, VALUETYPE v)
{
    initImageBorder(img.first, img.second, img.third, border_width, v);
}
    
//@}


} // namespace vigra

#endif // VIGRA_INITIMAGE_HXX
