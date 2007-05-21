/************************************************************************/
/*                                                                      */
/*               Copyright 2001-2002 by Gunnar Kedenburg                */
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
/* Modifications by Pablo d'Angelo
 * updated to vigra 1.4 by Douglas Wilkins
 * as of 18 Febuary 2006:
 *  - Added import/export of UINT16 and UINT32 image types.
 * Modifications by Andrew Mihal
 * updated to vigra 1.4 by Douglas Wilkins
 * as of 18 Febuary 2006:
 *  - Moved some RowIterator declarations around to avoid using default ctors
 *    (cachedfileimages do not have default ctors for row iterators).
 *  - Added some case-specific optimizations
 */

/*!
  \file  impex.hxx
  \brief image import and export functions

  this file provides the declarations and implementations of importImage()
  and exportImage(). the matching implementation for the given datatype is
  selected by template metacode.
*/

#ifndef VIGRA_IMPEX_HXX
#define VIGRA_IMPEX_HXX

#if defined(_MSC_VER)
#pragma warning (disable: 4267)
#endif

#include "sized_int.hxx"
#include "stdimage.hxx"
#include "tinyvector.hxx"
#include "imageinfo.hxx"
#include "numerictraits.hxx"
#include "codec.hxx"
#include "accessor.hxx"
#include "inspectimage.hxx"
#include "transformimage.hxx"
#include "copyimage.hxx"
#include "multi_array.hxx"

// TODO
// next refactoring: pluggable conversion algorithms

namespace vigra
{
/** \addtogroup VigraImpex
**/
//@{

    /*!
      \brief used for reading bands after the source data type has been figured out.

        <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
        Namespace: vigra

        <b> Declaration:</b>

        \code
        namespace vigra {
            template< class ImageIterator, class Accessor, class SrcValueType >
            void read_bands( Decoder * dec, ImageIterator ys, Accessor a, SrcValueType )
        }
        \endcode

      \param dec decoder object through which the source data will be accessed
      \param ys  image iterator referencing the upper left pixel of the destination image
      \param a   image accessor for the destination image
    */
    template< class ImageIterator, class Accessor, class SrcValueType >
    void read_bands( Decoder * dec, ImageIterator ys, Accessor a, SrcValueType )
    {
        typedef unsigned int size_type;
        typedef typename ImageIterator::row_iterator DstRowIterator;
        typedef typename Accessor::value_type  AccessorValueType;
        typedef typename AccessorValueType::value_type DstValueType;

        const size_type width = dec->getWidth();
        const size_type height = dec->getHeight();
        const size_type num_bands = dec->getNumBands();

        vigra_precondition(num_bands == a.size(ys),
           "importImage(): number of bands (color channels) in file and destination image differ.");

        SrcValueType const * scanline;
        // MIHAL no default constructor available for cachedfileimages.
        DstRowIterator xs = ys.rowIterator();

        // iterate
		if (num_bands == 4) {
            // Speedup for this particular case
            unsigned int offset = dec->getOffset();
            SrcValueType const * scanline0;
            SrcValueType const * scanline1;
            SrcValueType const * scanline2;
            SrcValueType const * scanline3;
            for( size_type y = 0; y < height; ++y, ++ys.y ) {
                dec->nextScanline();
                xs = ys.rowIterator();
                scanline0 = static_cast< SrcValueType const * >
                    (dec->currentScanlineOfBand(0));
                scanline1 = static_cast< SrcValueType const * >
                    (dec->currentScanlineOfBand(1));
                scanline2 = static_cast< SrcValueType const * >
                    (dec->currentScanlineOfBand(2));
                scanline3 = static_cast< SrcValueType const * >
                    (dec->currentScanlineOfBand(3));
                for( size_type x = 0; x < width; ++x, ++xs ) {
/*
                    a.template setComponent<SrcValueType, DstRowIterator, 0>( *scanline0, xs );
                    a.template setComponent<SrcValueType, DstRowIterator, 1>( *scanline1, xs );
                    a.template setComponent<SrcValueType, DstRowIterator, 2>( *scanline2, xs );
                    a.template setComponent<SrcValueType, DstRowIterator, 3>( *scanline3, xs );
*/
					a.setComponent( *scanline0, xs, 0);
                    a.setComponent( *scanline1, xs, 1);
                    a.setComponent( *scanline2, xs, 2);
                    a.setComponent( *scanline3, xs, 3);
                    scanline0 += offset;
                    scanline1 += offset;
                    scanline2 += offset;
                    scanline3 += offset;
                }
            }
        }
        else {
			// General case
        for( size_type y = 0; y < height; ++y, ++ys.y ) {
            dec->nextScanline();
            for( size_type b = 0; b < num_bands; ++b ) {
                xs = ys.rowIterator();
                scanline = static_cast< SrcValueType const * >
                    (dec->currentScanlineOfBand(b));
                for( size_type x = 0; x < width; ++x, ++xs ) {
                    a.setComponent( *scanline, xs, b );
                    scanline += dec->getOffset();
                }
            }
        }
		}
    } // read_bands()

    /*!
      \brief used for reading bands after the source data type has been figured out.

        <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
        Namespace: vigra

        <b> Declaration:</b>

        \code
        namespace vigra {
            template< class ImageIterator, class Accessor, class SrcValueType >
            void read_band( Decoder * dec, ImageIterator ys, Accessor a, SrcValueType )
        }
        \endcode

      \param dec decoder object through which the source data will be accessed
      \param ys  image iterator referencing the upper left pixel of the destination image
      \param a   image accessor for the destination image
    */
    template< class ImageIterator, class Accessor, class SrcValueType >
    void read_band( Decoder * dec, ImageIterator ys, Accessor a, SrcValueType )
    {
        typedef unsigned int size_type;
        typedef typename ImageIterator::row_iterator DstRowIterator;
        typedef typename Accessor::value_type DstValueType;
        const size_type width = dec->getWidth();
        const size_type height = dec->getHeight();

        SrcValueType const * scanline;
        // MIHAL no default constructor available for cachedfileimages.
        DstRowIterator xs = ys.rowIterator();

        for( size_type y = 0; y < height; ++y, ++ys.y ) {
            dec->nextScanline();
            xs = ys.rowIterator();
            scanline = static_cast< SrcValueType const * >(dec->currentScanlineOfBand(0));
            for( size_type x = 0; x < width; ++x, ++xs )
                a.set( scanline[x], xs );
        }
    } // read_band()

    /*!
      \brief used for reading images of vector type, such as integer of float rgb.

        <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
        Namespace: vigra

        <b> Declaration:</b>

        \code
        namespace vigra {
            template< class ImageIterator, class Accessor >
            void importVectorImage( const ImageImportInfo & info, ImageIterator iter, Accessor a )
        }
        \endcode

      \param ImageIterator the image iterator type for the destination image
      \param Accessor      the image accessor type for the destination image
      \param info          user supplied image import information
      \param iter          image iterator referencing the upper left pixel of the destination image
      \param a             image accessor for the destination image
    */
    template< class ImageIterator, class Accessor >
    void importVectorImage( const ImageImportInfo & info, ImageIterator iter, Accessor a )
    {
        std::auto_ptr<Decoder> dec = decoder(info);
        std::string pixeltype = dec->getPixelType();

        if ( pixeltype == "UINT8" )
            read_bands( dec.get(), iter, a, (UInt8)0 );
        else if ( pixeltype == "INT16" )
            read_bands( dec.get(), iter, a, Int16() );
        else if ( pixeltype == "UINT16" )
            read_bands( dec.get(), iter, a, (UInt16)0 );
        else if ( pixeltype == "INT32" )
            read_bands( dec.get(), iter, a, Int32() );
        else if ( pixeltype == "UINT32" )
            read_bands( dec.get(), iter, a, (UInt32)0 );
        else if ( pixeltype == "FLOAT" )
            read_bands( dec.get(), iter, a, float() );
        else if ( pixeltype == "DOUBLE" )
            read_bands( dec.get(), iter, a, double() );
        else
            vigra_precondition( false, "invalid pixeltype" );

        // close the decoder
        dec->close();
    }

    /*!
      \brief used for reading images of  scalar type, such as integer and float grayscale.

        <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
        Namespace: vigra

        <b> Declaration:</b>

        \code
        namespace vigra {
            template < class ImageIterator, class Accessor >
            void importScalarImage( const ImageImportInfo & info, ImageIterator iter, Accessor a )
        }
        \endcode

      \param ImageIterator the image iterator type for the destination image
      \param Accessor      the image accessor type for the destination image
      \param info          user supplied image import information
      \param iter          image iterator referencing the upper left pixel of the destination image
      \param a             image accessor for the destination image
    */
    template < class ImageIterator, class Accessor >
    void importScalarImage( const ImageImportInfo & info, ImageIterator iter, Accessor a )
    {
        std::auto_ptr<Decoder> dec = decoder(info);
        std::string pixeltype = dec->getPixelType();

        if ( pixeltype == "UINT8" )
            read_band( dec.get(), iter, a, (UInt8)0 );
        else if ( pixeltype == "INT16" )
            read_band( dec.get(), iter, a, Int16() );
        else if ( pixeltype == "UINT16" )
            read_band( dec.get(), iter, a, (UInt16)0 );
        else if ( pixeltype == "INT32" )
            read_band( dec.get(), iter, a, Int32() );
        else if ( pixeltype == "UINT32" )
            read_band( dec.get(), iter, a, (UInt32)0 );
        else if ( pixeltype == "FLOAT" )
            read_band( dec.get(), iter, a, float() );
        else if ( pixeltype == "DOUBLE" )
            read_band( dec.get(), iter, a, double() );
        else
            vigra_precondition( false, "invalid pixeltype" );

        // close the decoder
        dec->close();
    }

    template < class ImageIterator, class Accessor >
    void importImage( const ImageImportInfo & info, ImageIterator iter, Accessor a, VigraFalseType )
    {
        importVectorImage( info, iter, a );
    }

    template < class ImageIterator, class Accessor >
    void importImage( const ImageImportInfo & info, ImageIterator iter, Accessor a, VigraTrueType )
    {
        importScalarImage( info, iter, a );
    }

/********************************************************/
/*                                                      */
/*                     importImage                      */
/*                                                      */
/********************************************************/

    /** \brief Read an image, given an \ref vigra::ImageImportInfo object.

        <b> Declarations:</b>

        pass arguments explicitly:
        \code
        namespace vigra {
            template <class ImageIterator, class Accessor>
            void
            importImage(ImageImportInfo const & image, ImageIterator iter, Accessor a)
        }
        \endcode

        use argument objects in conjunction with \ref ArgumentObjectFactories:
        \code
        namespace vigra {
            template <class ImageIterator, class Accessor>
            inline void
            importImage(ImageImportInfo const & image, pair<ImageIterator, Accessor> dest)
        }
        \endcode

        <b> Usage:</b>

        <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
        Namespace: vigra

        \code

        vigra::ImageImportInfo info("myimage.gif");

        if(info.isGrayscale())
        {
            // create byte image of appropriate size
            vigra::BImage in(info.width(), info.height());

            vigra::importImage(info, destImage(in)); // read the image
            ...
        }
        else
        {
            // create byte RGB image of appropriate size
            vigra::BRGBImage in(info.width(), info.height());

            vigra::importImage(info, destImage(in)); // read the image
            ...
        }

        \endcode

        <b> Preconditions:</b>

        <UL>

        <LI> the image file must be readable
        <LI> the file type must be one of

                <DL>
                <DT>"BMP"<DD> Microsoft Windows bitmap image file.
                <DT>"GIF"<DD> CompuServe graphics interchange format; 8-bit color.
                <DT>"JPEG"<DD> Joint Photographic Experts Group JFIF format; compressed 24-bit color. (only available if libjpeg is installed)
                <DT>"PNG"<DD> Portable Network Graphic. (only available if libpng is installed)
                <DT>"PBM"<DD> Portable bitmap format (black and white).
                <DT>"PGM"<DD> Portable graymap format (gray scale).
                <DT>"PNM"<DD> Portable anymap.
                <DT>"PPM"<DD> Portable pixmap format (color).
                <DT>"SUN"<DD> SUN Rasterfile.
                <DT>"TIFF"<DD> Tagged Image File Format. (only available if libtiff is installed.)
                <DT>"VIFF"<DD> Khoros Visualization image file.
                </DL>
        </UL>
    **/
    template < class ImageIterator, class Accessor >
    void importImage( const ImageImportInfo & info, ImageIterator iter, Accessor a )
    {
        typedef typename NumericTraits<typename Accessor::value_type>::isScalar is_scalar;
        importImage( info, iter, a, is_scalar() );
    }

    template < class ImageIterator, class Accessor >
    void importImage( const ImageImportInfo & info, pair< ImageIterator, Accessor > dest )
    {
        importImage( info, dest.first, dest.second );
    }

    /*!
      \brief used for writing bands after the source data type has been figured out.

        <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
        Namespace: vigra

        <b> Declaration:</b>

        \code
        namespace vigra {
            template< class ImageIterator, class Accessor, class DstValueType >
            void write_bands( Encoder * enc, ImageIterator ul, ImageIterator lr, Accessor a, DstValueType )
        }
        \endcode

      \param enc encoder object through which the destination data will be accessed
      \param ul  image iterator referencing the upper left pixel of the source image
      \param lr  image iterator referencing the lower right pixel of the source image
      \param a   image accessor for the source image
    */
    template< class ImageIterator, class Accessor, class DstValueType >
    void write_bands( Encoder * enc, ImageIterator ul, ImageIterator lr, Accessor a, DstValueType)
    {
        typedef unsigned int size_type;
        typedef typename ImageIterator::row_iterator SrcRowIterator;
        typedef typename Accessor::value_type  AccessorValueType;
        typedef typename AccessorValueType::value_type SrcValueType;

        // complete decoder settings
        const size_type width = lr.x - ul.x;
        const size_type height = lr.y - ul.y;
        enc->setWidth(width);
        enc->setHeight(height);
        const size_type num_bands = a.size(ul);
        enc->setNumBands(num_bands);
        enc->finalizeSettings();

        DstValueType * scanline;

        // iterate
        ImageIterator ys(ul);
        // MIHAL no default constructor available for cachedfileimages
        SrcRowIterator xs = ys.rowIterator();

		if (num_bands == 4) {
            // Speedup for this particular case
            unsigned int offset = enc->getOffset();
            DstValueType * scanline0;
            DstValueType * scanline1;
            DstValueType * scanline2;
            DstValueType * scanline3;
            for( size_type y = 0; y < height; ++y, ++ys.y ) {
                xs = ys.rowIterator();
                scanline0 = static_cast< DstValueType * >
                        (enc->currentScanlineOfBand(0));
                scanline1 = static_cast< DstValueType * >
                        (enc->currentScanlineOfBand(1));
                scanline2 = static_cast< DstValueType * >
                        (enc->currentScanlineOfBand(2));
                scanline3 = static_cast< DstValueType * >
                        (enc->currentScanlineOfBand(3));
                for( size_type x = 0; x < width; ++x, ++xs) {
/*
                    *scanline0 = a.template getComponent<SrcRowIterator, 0>( xs );
                    *scanline1 = a.template getComponent<SrcRowIterator, 1>( xs );
                    *scanline2 = a.template getComponent<SrcRowIterator, 2>( xs );
                    *scanline3 = a.template getComponent<SrcRowIterator, 3>( xs );
*/
                    *scanline0 = detail::RequiresExplicitCast<DstValueType>::cast(a.getComponent( xs, 0));
                    *scanline1 = detail::RequiresExplicitCast<DstValueType>::cast(a.getComponent( xs, 1));
                    *scanline2 = detail::RequiresExplicitCast<DstValueType>::cast(a.getComponent( xs, 2));
                    *scanline3 = detail::RequiresExplicitCast<DstValueType>::cast(a.getComponent( xs, 3));
                    scanline0 += offset;
                    scanline1 += offset;
                    scanline2 += offset;
                    scanline3 += offset;
                }
                enc->nextScanline();
            }
        }
        else {
			// General case
        for( size_type y = 0; y < height; ++y, ++ys.y ) {
            for( size_type b = 0; b < num_bands; ++b ) {
                xs = ys.rowIterator();
                scanline = static_cast< DstValueType * >
                    (enc->currentScanlineOfBand(b));
                for( size_type x = 0; x < width; ++x, ++xs ) {
                    *scanline = detail::RequiresExplicitCast<DstValueType>::cast(a.getComponent( xs, b ));
                    scanline += enc->getOffset();
                }
            }
            enc->nextScanline();
        }
		}
    } // write_bands()

    template< class MArray, class DstValueType >
    void write_bands( Encoder * enc, MArray const & array, DstValueType)
    {
        typedef unsigned int size_type;

        // complete decoder settings
        const size_type width = array.shape(0);
        const size_type height = array.shape(1);
        enc->setWidth(width);
        enc->setHeight(height);
        const size_type num_bands = array.shape(2);
        enc->setNumBands(num_bands);
        enc->finalizeSettings();

        DstValueType * scanline;

        // iterate
        for( size_type y = 0; y < height; ++y ) {
            for( size_type b = 0; b < num_bands; ++b ) {
                scanline = static_cast< DstValueType * >
                    (enc->currentScanlineOfBand(b));
                for( size_type x = 0; x < width; ++x) {
                    *scanline = array(x, y, b);
                    scanline += enc->getOffset();
                }
            }
            enc->nextScanline();
        }
    } // write_bands()

    /*!
      \brief used for writing bands after the source data type has been figured out.

        <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
        Namespace: vigra

        <b> Declaration:</b>

        \code
        namespace vigra {
            template< class ImageIterator, class Accessor, class DstValueType >
            void write_band( Encoder * enc, ImageIterator ul, ImageIterator lr, Accessor a, DstValueType )
        }
        \endcode

      \param enc encoder object through which the destination data will be accessed
      \param ul  image iterator referencing the upper left pixel of the source image
      \param lr  image iterator referencing the lower right pixel of the source image
      \param a   image accessor for the source image
    */
    template< class ImageIterator, class Accessor, class DstValueType >
    void write_band( Encoder * enc, ImageIterator ul, ImageIterator lr, Accessor a, DstValueType)
    {
        typedef unsigned int size_type;
        typedef typename ImageIterator::row_iterator SrcRowIterator;
        typedef typename Accessor::value_type SrcValueType;

        // complete decoder settings
        const size_type width = lr.x - ul.x;
        const size_type height = lr.y - ul.y;
        enc->setWidth(width);
        enc->setHeight(height);
        enc->setNumBands(1);
        enc->finalizeSettings();

        DstValueType * scanline;

        // iterate
        ImageIterator ys(ul);
        // MIHAL no default constructor available for cachedfileimages.
        SrcRowIterator xs = ys.rowIterator();
        size_type y;
        for(  y = 0; y < height; ++y, ++ys.y ) {
            xs = ys.rowIterator();
            scanline = static_cast< DstValueType * >(enc->currentScanlineOfBand(0));
            for( size_type x = 0; x < width; ++x, ++xs, ++scanline )
                *scanline = detail::RequiresExplicitCast<DstValueType>::cast(a(xs));
            enc->nextScanline();
        }
    } // write_band()

namespace detail {

    template < class SrcIterator, class SrcAccessor,
               class DestIterator, class DestAccessor >
    void mapScalarImageToLowerPixelType( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                         DestIterator dul, DestAccessor dget )
    {
        typedef typename SrcAccessor::value_type SrcValue;
        typedef typename DestAccessor::value_type DestValue;
        typedef typename NumericTraits<SrcValue>::RealPromote PromoteValue;

        FindMinMax<SrcValue> minmax;
        inspectImage( sul, slr, sget, minmax );
        double scale = (double)NumericTraits<DestValue>::max() / (minmax.max - minmax.min) -
                       (double)NumericTraits<DestValue>::min() / (minmax.max - minmax.min);
        double offset = (NumericTraits<DestValue>::min() / scale) - minmax.min ;
        transformImage( sul, slr, sget, dul, dget,
                        linearIntensityTransform( scale, offset ) );
    }

    // export scalar images with conversion (if necessary)
    template < class SrcIterator, class SrcAccessor, class T >
    void exportScalarImage(SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                           Encoder * enc, bool downcast, T zero)
    {
        if (!downcast) {
            write_band( enc, sul, slr, sget, zero );
        } else {
            // convert to unsigned char in the usual way
            BasicImage<T> image(slr-sul);
            mapScalarImageToLowerPixelType(sul, slr, sget, image.upperLeft(), image.accessor());
            write_band( enc, image.upperLeft(),
                        image.lowerRight(), image.accessor(), zero );
        }
    }

    template < class SrcIterator, class SrcAccessor,
               class MArray>
    void mapVectorImageToLowerPixelType( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                         MArray & array )
    {
        typedef typename SrcAccessor::value_type SrcValue;
        typedef typename SrcValue::value_type SrcComponent;
        typedef typename MArray::value_type DestValue;

        FindMinMax<SrcComponent> minmax;
        for(unsigned int i=0; i<sget.size(sul); ++i)
        {
            // FIXME dangelo - This will break with vector accessors that have a "by value" interface.
            // use VectorComponentValueAccessor instead, since it should work in both cases, even
            // if it might be a bit slower..
            //VectorElementAccessor<SrcAccessor> band(i, sget);
            VectorComponentValueAccessor<typename SrcAccessor::value_type> band(i);
            inspectImage( sul, slr, band, minmax );
        }
        double scale = (double)NumericTraits<DestValue>::max() / (minmax.max - minmax.min) -
                       (double)NumericTraits<DestValue>::min() / (minmax.max - minmax.min);
// FIXME DGSW - Original was not correct. Is this what was intended?
//        double offset = -minmax.min + NumericTraits<DestValue>::min() / scale;
        double offset = (NumericTraits<DestValue>::min() / scale) - minmax.min ;
        for(unsigned int i=0; i<sget.size(sul); ++i)
        {
            BasicImageView<DestValue> subImage = makeBasicImageView(array.bindOuter(i));
            // FIXME dangelo: use VectorComponentValueAccessor
            //VectorElementAccessor<SrcAccessor> band(i, sget);
            VectorComponentValueAccessor<typename SrcAccessor::value_type> band(i);
            transformImage( sul, slr, band, subImage.upperLeft(), subImage.accessor(),
                            linearIntensityTransform( scale, offset ) );
        }
    }

    // export vector images with conversion (if necessary)
    template < class SrcIterator, class SrcAccessor, class T >
    void exportVectorImage(SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                           Encoder * enc, bool downcast, T zero)
    {
        int bands = sget.size(sul);
        vigra_precondition(isBandNumberSupported(enc->getFileType(), bands),
           "exportImage(): file format does not support requested number of bands (color channels)");
        if ( !downcast )
        {
            write_bands( enc, sul, slr, sget, zero );
        }
        else
        {
            // convert to unsigned char in the usual way
            int w = slr.x - sul.x;
            int h = slr.y - sul.y;

            typedef vigra::MultiArray<3, T> MArray;
            MArray array(typename MArray::difference_type(w, h, bands));

            mapVectorImageToLowerPixelType(sul, slr, sget, array);

            write_bands( enc, array, zero );
        }
    }
} // namespace detail


    /*!
      \brief Deprecated.

        Use \ref exportImage() instead.

        <b> Declaration:</b>

        \code
        namespace vigra {
            template < class SrcIterator, class SrcAccessor >
            void exportFloatingVectorImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                            const ImageExportInfo & info )
        }
        \endcode
    */
    template < class SrcIterator, class SrcAccessor >
    void exportFloatingVectorImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                    const ImageExportInfo & info )
    {
        exportImage(sul, slr, sget, info);
    }

    /*!
      \brief Deprecated.

        Use \ref exportImage() instead.

        <b> Declaration:</b>

        \code
        namespace vigra {
            template < class SrcIterator, class SrcAccessor >
            void exportIntegralVectorImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                            const ImageExportInfo & info )
        }
        \endcode
    */
    template < class SrcIterator, class SrcAccessor >
    void exportIntegralVectorImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                    const ImageExportInfo & info )
    {
        exportImage(sul, slr, sget, info);
    }

    /*!
      \brief Deprecated.

        Use \ref exportImage() instead.

        <b> Declaration:</b>

        \code
        namespace vigra {
            template < class SrcIterator, class SrcAccessor >
            void exportFloatingScalarImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                            const ImageExportInfo & info )
        }
        \endcode
    */
    template < class SrcIterator, class SrcAccessor >
    void exportFloatingScalarImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                    const ImageExportInfo & info )
    {
        exportImage(sul, slr, sget, info);
    }

    /*!
      \brief Deprecated.

        Use \ref exportImage() instead.

        <b> Declaration:</b>

        \code
        namespace vigra {
            template < class SrcIterator, class SrcAccessor >
            void exportIntegralScalarImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                            const ImageExportInfo & info )
        }
        \endcode
    */
    template < class SrcIterator, class SrcAccessor >
    void exportIntegralScalarImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                                    const ImageExportInfo & info )
    {
        exportImage(sul, slr, sget, info);
    }

    template < class SrcIterator, class SrcAccessor >
    void exportImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                      const ImageExportInfo & info, VigraFalseType /*not scalar */)
    {
        typedef typename SrcAccessor::value_type AccessorValueType;
        typedef typename AccessorValueType::value_type SrcValueType;
        std::string pixeltype = info.getPixelType();
        std::auto_ptr<Encoder> enc = encoder(info);
        bool downcast = negotiatePixelType(enc->getFileType(),
                        TypeAsString<SrcValueType>::result(), pixeltype);
        enc->setPixelType(pixeltype);
        if(pixeltype == "UINT8")
            detail::exportVectorImage( sul, slr, sget, enc.get(), downcast, (UInt8)0);
        else if(pixeltype == "INT16")
            detail::exportVectorImage( sul, slr, sget, enc.get(), downcast, Int16());
        else if(pixeltype == "UINT16")
            detail::exportVectorImage( sul, slr, sget, enc.get(), downcast, (UInt16)0);
        else if(pixeltype == "INT32")
            detail::exportVectorImage( sul, slr, sget, enc.get(), downcast, Int32());
        else if(pixeltype == "UINT32")
            detail::exportVectorImage( sul, slr, sget, enc.get(), downcast, (UInt32)0);
        else if(pixeltype == "FLOAT")
            detail::exportVectorImage( sul, slr, sget, enc.get(), downcast, float());
        else if(pixeltype == "DOUBLE")
            detail::exportVectorImage( sul, slr, sget, enc.get(), downcast, double());
        enc->close();
    }

    template < class SrcIterator, class SrcAccessor >
    void exportImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                      const ImageExportInfo & info, VigraTrueType /*scalar*/ )
    {
        typedef typename SrcAccessor::value_type SrcValueType;
        std::string pixeltype = info.getPixelType();
        std::auto_ptr<Encoder> enc = encoder(info);
        bool downcast = negotiatePixelType(enc->getFileType(),
                           TypeAsString<SrcValueType>::result(), pixeltype);
        enc->setPixelType(pixeltype);
        if(pixeltype == "UINT8")
            detail::exportScalarImage( sul, slr, sget, enc.get(), downcast, (UInt8)0);
        else if(pixeltype == "INT16")
            detail::exportScalarImage( sul, slr, sget, enc.get(), downcast, Int16());
        else if(pixeltype == "UINT16")
            detail::exportScalarImage( sul, slr, sget, enc.get(), downcast, (UInt16)0);
        else if(pixeltype == "INT32")
            detail::exportScalarImage( sul, slr, sget, enc.get(), downcast, Int32());
        else if(pixeltype == "UINT32")
            detail::exportScalarImage( sul, slr, sget, enc.get(), downcast, (UInt32)0);
        else if(pixeltype == "FLOAT")
            detail::exportScalarImage( sul, slr, sget, enc.get(), downcast, float());
        else if(pixeltype == "DOUBLE")
            detail::exportScalarImage( sul, slr, sget, enc.get(), downcast, double());
        enc->close();
    }

/********************************************************/
/*                                                      */
/*                     exportImage                      */
/*                                                      */
/********************************************************/

/** \brief Write an image, given an \ref vigra::ImageExportInfo object.

    If the file format to be exported to supports the pixel type of the
    source image, the pixel type will be kept (e.g. <tt>float</tt>
    can be stored as TIFF without conversion, in contrast to most other
    image export toolkits). Otherwise, the pixel values are transformed
    to the range 0.255 and converted to <tt>unsigned char</tt>. Currently,
    the following file formats are supported. The pixel types given in
    brackets are those that are written without conversion:

    <DL>
    <DT>"BMP"<DD> Microsoft Windows bitmap image file (pixel types: UINT8 as gray and RGB).
    <DT>"GIF"<DD> CompuServe graphics interchange format; 8-bit color (pixel types: UINT8 as gray and RGB).
    <DT>"JPEG"<DD> Joint Photographic Experts Group JFIF format; compressed 24-bit color
                  (pixel types: UINT8 as gray and RGB). (only available if libjpeg is installed)
    <DT>"PNG"<DD> Portable Network Graphic (pixel types: UINT8 and UINT16 with up to 4 channels).
                  (only available if libpng is installed)
    <DT>"PBM"<DD> Portable bitmap format (black and white).
    <DT>"PGM"<DD> Portable graymap format (pixel types: UINT8, INT16, INT32 as gray scale)).
    <DT>"PNM"<DD> Portable anymap (pixel types: UINT8, INT16, INT32 as gray and RGB).
    <DT>"PPM"<DD> Portable pixmap format (pixel types: UINT8, INT16, INT32 as RGB).
    <DT>"SUN"<DD> SUN Rasterfile (pixel types: UINT8 as gray and RGB).
    <DT>"TIFF"<DD> Tagged Image File Format
                (pixel types: UINT8, INT16, INT32, FLOAT, DOUBLE with up to 4 channels).
                (only available if libtiff is installed.)
    <DT>"VIFF"<DD> Khoros Visualization image file
        (pixel types: UINT8, INT16, INT32, FLOAT, DOUBLE with arbitrary many channels).
    </DL>

    <b> Declarations:</b>

    pass arguments explicitly:
    \code
    namespace vigra {
        template <class SrcIterator, class SrcAccessor>
        void exportImage(SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                         ImageExportInfo const & info)
    }
    \endcode


    use argument objects in conjunction with \ref ArgumentObjectFactories:
    \code
    namespace vigra {
        template <class SrcIterator, class SrcAccessor>
        void exportImage(SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                         ImageExportInfo const & info)
    }
    \endcode

    <b> Usage:</b>

    <b>\#include</b> "<a href="impex_8hxx-source.html">vigra/impex.hxx</a>"<br>
    Namespace: vigra

    \code


    vigra::BRGBImage out(w, h);
    ...

    // write as JPEG image, using compression quality 80
    vigra::exportImage(srcImageRange(out),
                      vigra::ImageExportInfo("myimage.jpg").setCompression("80"));


    // force it to a particular pixel type (the pixel type must be supported by the
    // desired image file format, otherwise an \ref vigra::PreconditionViolation exception will be thrown)
    vigra::exportImage(srcImageRange(out),
                      vigra::ImageExportInfo("myINT16image.tif").setPixelType("INT16"));
    \endcode

    <b> Preconditions:</b>

    <UL>

    <LI> the image file must be writable.
    <LI> the file type must be one of the supported file types.


    </UL>
**/
    template < class SrcIterator, class SrcAccessor >
    inline
    void exportImage( SrcIterator sul, SrcIterator slr, SrcAccessor sget,
                      const ImageExportInfo & info )
    {
        typedef typename NumericTraits<typename SrcAccessor::value_type>::isScalar is_scalar;

        try
        {
            exportImage( sul, slr, sget, info, is_scalar() );
        }
        catch(Encoder::TIFFCompressionException &)
        {
            const_cast<ImageExportInfo &>(info).setCompression("");
            exportImage( sul, slr, sget, info, is_scalar() );
        }
    }

    template < class SrcIterator, class SrcAccessor >
    inline
    void exportImage( triple<SrcIterator, SrcIterator, SrcAccessor> src,
                      const ImageExportInfo & info )
    {
        exportImage( src.first, src.second, src.third, info );
    }

//@}

} // namespace vigra

#endif /* VIGRA_IMPEX_HXX */
