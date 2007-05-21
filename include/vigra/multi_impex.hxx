/************************************************************************/
/*                                                                      */
/*               Copyright 2003 by Gunnar Kedenburg                     */
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


#ifndef VIGRA_MULTI_IMPEX_HXX
#define VIGRA_MULTI_IMPEX_HXX

#include <memory>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>
#include "config.hxx"
#include "basicimageview.hxx"
#include "impex.hxx"
#include "multi_array.hxx"

namespace vigra {

VIGRA_EXPORT void findImageSequence(const std::string &name_base,
                       const std::string &name_ext,
                       std::vector<std::string> & numbers);

/** \addtogroup VolumeImpex Import/export of volume data.
*/

//@{

/********************************************************/
/*                                                      */
/*                    importVolume                      */
/*                                                      */
/********************************************************/

/** \brief Function for importing a 3D volume.

    The data are expected to be stored in a by-slice manner,
    where the slices are enumerated from <tt>name_base+"[0-9]+"+name_ext</tt>.
    <tt>name_base</tt> may contain a path. All slice files with the same name base and
    extension are considered part of the same volume. Slice numbers must be non-negative,
    but can otherwise start anywhere and need not be successive. Slices will be read
    in ascending numerical (not lexicographic) order. All slices must have the
    same size. The <tt>volume</tt> will be reshaped to match the count and
    size of the slices found.

    <b>\#include</b>
    "<a href="multi__impex_8hxx-source.html">vigra/multi_impex.hxx</a>"

    Namespace: vigra
*/
template <class T, class Allocator>
void importVolume (MultiArray <3, T, Allocator> & volume,
                   const std::string &name_base,
                   const std::string &name_ext)
{
    std::vector<std::string> numbers;
    findImageSequence(name_base, name_ext, numbers);

    std::string message("importVolume(): No files matching '");
    message += name_base + "[0-9]+" + name_ext + "' found.";
    vigra_precondition(numbers.size() > 0, message.c_str());

    for (unsigned int i = 0; i < numbers.size(); ++i)
    {
        // build the filename
        std::string name = name_base + numbers[i] + name_ext;

        // import the image
        ImageImportInfo info (name.c_str ());

        // reshape the array according to size of first image
        if(i == 0)
        {
            typedef typename MultiArray <3, T>::difference_type Size;
            volume.reshape(Size(info.width(), info.height(), numbers.size()));
        }

        // generate a basic image view to the current layer
        MultiArrayView <2, T> array_view (volume.bindOuter (i));
        BasicImageView <T> view = makeBasicImageView (array_view);
        vigra_precondition(view.size() == info.size(),
            "importVolume(): image size mismatch.");

        importImage (info, destImage(view));
    }
}


/********************************************************/
/*                                                      */
/*                    exportVolume                      */
/*                                                      */
/********************************************************/

/** \brief Function for exporting a 3D volume.

    The volume is exported in a by-slice manner, where the number of slices equals
    the depth of the volume. The file names will be enumerated like
    <tt>name_base+"000"+name_ext</tt>, <tt>name_base+"001"+name_ext</tt> etc.
    (the actual number of zeros depends on the depth).

    <b>\#include</b>
    "<a href="multi__impex_8hxx-source.html">vigra/multi_impex.hxx</a>"

    Namespace: vigra
*/
template <class T, class Tag>
void exportVolume (MultiArrayView <3, T, Tag> const & volume,
                   const std::string &name_base,
                   const std::string &name_ext)
{

    const unsigned int depth = volume.shape (2);
    int numlen = static_cast <int> (std::ceil (std::log10 ((double)depth)));
    for (unsigned int i = 0; i < depth; ++i)
    {

        // build the filename
        std::stringstream stream;
        stream << std::setfill ('0') << std::setw (numlen) << i;
        std::string name_num;
        stream >> name_num;
        std::string name = name_base + name_num + name_ext;

        // generate a basic image view to the current layer
        MultiArrayView <2, T, Tag> array_view (volume.bindOuter (i));
        BasicImageView <T> view = makeBasicImageView (array_view);

        // export the image
        ImageExportInfo info(name.c_str ());
        exportImage(srcImageRange(view), info);
    }
}

//@}

} // namespace vigra

#endif // VIGRA_MULTI_IMPEX_HXX
