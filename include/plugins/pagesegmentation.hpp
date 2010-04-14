/*
 *
 * Copyright (C) 2007-2009 Christoph Dalitz, Stefan Ruloff,
 *                         Maria Elhachimi, Ilya Stoyanov, Robert Butz
 *               2010      Christoph Dalitz, Tobias Bolten
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef cd20070814_pagesegmentation
#define cd20070814_pagesegmentation

#include <Python.h>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include "gamera.hpp"
#include "gameramodule.hpp"
#include "gamera_limits.hpp"
#include "connected_components.hpp"
#include "plugins/listutilities.hpp"
#include "plugins/projections.hpp"
#include "plugins/segmentation.hpp"
#include "plugins/image_utilities.hpp"


namespace Gamera {

/* Function: median
 * Calculates the middle height of the CCs.
 * Used for setting defualt parameters in 
 * runlength_smearing and projection_cutting
 */
int pagesegmentation_median_height(ImageList* ccs) {
  vector<int> ccs_heights;
  ImageList::iterator i;

  if (ccs->empty()) {
    throw std::runtime_error("pagesegmentation_median_height: no CC's found in image.");
  }
  for (i = ccs->begin(); i != ccs->end(); ++i) {
    ccs_heights.push_back( (*i)->nrows() );
  }
  return median(&ccs_heights);
}


/*****************************************************************************
* Run Length Smearing
* IN:   Cx - Minimal length of white runs in the rows
*   Cy - Minimal length of white runs in the columns
*   Csm- Minimal length of white runs row-wise in the almost final image
*       
*   If you choose "-1" the algorithm will determine the
*   median character length in the image to obtain the values for Cx,Cy or 
*   Csm.
******************************************************************************/
template<class T>
ImageList* runlength_smearing(T &image, int Cx, int Cy, int Csm) {
    typedef OneBitImageView VIEW;
    typedef OneBitImageData DATA;
    typedef typename T::value_type COLOR;

    DATA* X_Data = new DATA(image.size(), image.origin());
    VIEW* X_View = new VIEW(*X_Data);
    image_copy_fill(image, *X_View);
    
    DATA* Y_Data = new DATA(image.size(), image.origin());
    VIEW* Y_View = new VIEW(*Y_Data);
    image_copy_fill(image, *Y_View);

    DATA* AND_Data = new DATA(image.size(), image.origin());
    VIEW* AND_View = new VIEW(*AND_Data);

    int Ctemp = 0;
    size_t NRows = image.nrows();
    size_t NCols = image.ncols();
    size_t x, y;
    COLOR Black = black(image);
    //COLOR White = white(image);

    // int Csm = 17, Cy = 100, Cx = 100;
    // The user can't define values for the white runs
    // Default behavior if one parameter is less then zero
    if (Csm <= 0 || Cy <= 0 || Cx <= 0) {
        ImageList* ccs_temp = cc_analysis(image);
        int Median = pagesegmentation_median_height(ccs_temp);

        for (ImageList::iterator i = ccs_temp->begin(); 
                i != ccs_temp->end(); i++) {
            delete *i;
        }
        delete ccs_temp;

        if (Csm <= 0) {
            Csm = 3 * Median;
        }
        if (Cy <= 0) {
            Cy = 20 * Median;
        }
        if (Cx <= 0) {
            Cx = 20 * Median;
        }
    }


    // Horizontal Smearing
    for (y = 0; y < NRows; ++y) {
        for (x = 0; x < NCols; ++x) {
            if (x == 0) { // On new line
                Ctemp = 0;
            }
            if (is_white(image.get(Point(x, y)))) { //White
                Ctemp += 1;
            } else {                                //Black
                if ((0 != Ctemp) && (Ctemp <= Cx)){
                    for (int z = 0; z < Ctemp; z++) {
                        X_View->set(Point(x - z - 1, y), Black);
                    }
                }
                Ctemp = 0;
            }
        }
    }

    // Vertical Smearing
    for (x = 0; x < NCols; ++x) {
        for (y = 0; y < NRows; ++y) {
            if (y == 0) { // New line
                Ctemp = 0;
            }
            if (is_white(image.get(Point(x, y)))) { // White
                Ctemp += 1;
            } else {                                // Black
                if ((0 != Ctemp) && (Ctemp <= Cy)) {
                    for (int z = 0; z < Ctemp; z++)
                        Y_View->set(Point(x, y - z - 1), Black);
                }
                Ctemp = 0;
            }
        }
    }

        // Segmentation from Horizontal and Vertical Smearing
        for(y = 0; y < NRows; ++y) {
                for(x = 0; x < NCols; ++x) {
            if ((is_black(X_View->get(Point(x, y))))
                    && (is_black(Y_View->get(Point(x, y))))){
                AND_View->set(Point(x, y), Black);
            }
                }
        }

    // Removing spots from the rows less than Csm pixels
    for (y = 0; y < NRows; ++y) {
        for ( x = 0; x < NCols; ++x) {
            if (x == 0) { // New line
                Ctemp = 0;
            }
            if (is_white(AND_View->get(Point(x, y)))) { // White
                Ctemp += 1;
            } else {                                    // Black
                if ((0 != Ctemp) && (Ctemp <= Csm)){
                    for (int z = 0; z < Ctemp; z++)
                        AND_View->set(Point(x - z - 1, y), Black);
                }
                Ctemp = 0;
            }
        }
    }

    ImageList* ccs_AND = cc_analysis(*AND_View);
    ImageList* return_ccs = new ImageList();

    // Create CCs 
    ImageList::iterator i;
    for (i = ccs_AND->begin(); i != ccs_AND->end(); ++i) {  
        Cc* cc = dynamic_cast<Cc*>(*i);
        int label = cc->label();

                // Methods "get" and "set" operates relative to the image view
                // but the offset of the connected components is not relative
                // to the view. (here: (*i)->offset_x() and (*i)->offset_y())
                //
                // This means that these values must be adjusted for labeling
                // the image view.
        for (y = 0; y < (*i)->nrows(); ++y) {
            for (x = 0; x < (*i)->ncols(); ++x) {
                if( is_black(image.get(Point(x + (*i)->offset_x() - image.offset_x() , y + (*i)->offset_y() - image.offset_y()))) ) {
                    image.set(Point(x + (*i)->offset_x() - image.offset_x() , y + (*i)->offset_y() - image.offset_y()), label);
                }
            }
        }

        // Makes a new CC with the dimensions, offset and label from the
        // smeared image and the content of the original image.
        return_ccs->push_back(new ConnectedComponent<DATA>(
                *((DATA*)image.data()),                     // Data
                label,                                      // Label
                Point((*i)->offset_x(), (*i)->offset_y()),  // Point
                (*i)->dim())                                // Dim
                );

        delete *i;
    }
    
    // Free the memory
    delete ccs_AND;
    delete X_View->data();
    delete X_View;
    delete Y_View->data();
    delete Y_View;
    delete AND_View->data();
    delete AND_View;

    return return_ccs;
}


/*-------------------------------------------------------------------------
 * Functions for projection_cutting:
 * Interne_RXY_Cut(image, Tx, Ty, ccs, noise, label):recursively splits 
 * the image, sets the label and creates the CCs.
 * Start_point(image, ul, lr):search the upper_left point of the sub-image.
 * End_point(image,ul,lr):search the lower_right point of the sub-image.
 * Split_point:searchs the split point of the image
 * rxy_cut(image,Tx,Ty,noise,label):returns the ccs-list
 *-------------------------------------------------------------------------*/


/* Function: Start_Point
 * This funktion is used to search the first black pixel:
 * calculates the coordinates of the begin of the cc
 * returns the coordinates of the upper-left point of subimage
 */



template<class T>
Point proj_cut_Start_Point(T& image, Point ul, Point lr) {
    Point Start;

    for (size_t y = ul.y(); y <= lr.y(); y++) {
    for (size_t x = ul.x(); x <= lr.x(); x++) {
        if ((image.get(Point(x, y))) != 0) {
            Start.x(x);
            Start.y(y);
            goto endLoop1; // unfortunately there is no break(2) in gorgeous C++
        }
    }
    } 
    endLoop1:

    for (size_t x = ul.x(); x <= lr.x(); x++) {
    for (size_t y = ul.y(); y <= lr.y(); y++) {
        if ((image.get(Point(x, y))) != 0) {
            if (Start.x() > x)
                Start.x(x);
            goto endLoop2; // unfortunately there is no break(2) in gorgeous C++
            }
        }
    }
    endLoop2:
    return Start;
}

/* Function: End_Point
 * This funktion is used to search the last black pixel:the lower-right point
 * of subimage calculates the coordinates of the end of the CC.
 */
template<class T>
Point proj_cut_End_Point(T& image, Point ul, Point lr) {
    Point End;
    size_t x, y;

    for (y = lr.y(); y+1 >= ul.y()+1; y--) {
        for (x = lr.x(); x+1 >= ul.x()+1; x--) {
            if ((image.get(Point(x, y))) != 0) {
                End.x(x);
                End.y(y);
                goto endLoop1;
            }
        }
    }
    endLoop1:
    
    for (x = lr.x(); x+1 > ul.x()+1; x--) {
        for (y = lr.y(); y+1 > ul.y()+1; y--) {
            if ((image.get(Point(x,y))) != 0){
                if (End.x()<x)
                    End.x(x);
                goto endLoop2;
            }
        }
    }
    endLoop2:

    return End;
}

/* Function: Split_Point
 * calculates the coordinates of the split_point.
 * The split point is determined
 * by finding the largest possible gaps in the X and Y projection of the image.
 */
template<class T>
IntVector * proj_cut_Split_Point(T& image, Point ul, Point lr, int Tx, int Ty, int noise, int gap_treatment, char direction ) {
    IntVector * SplitPoints = new IntVector(); //empty IntVector
    size_t size;
    lr.x()-ul.x()>lr.y()-ul.y()?size=lr.x()-ul.x():size=lr.y()-ul.y();

    int SplitPoints_Min[size]; // probably no need for such big mem-alloc, but necessary in certain situations
    int SplitPoints_Max[size]; 
    int gap_width = 0; // width of the gap
    int gap_counter = 0; //number of gaps

    if (direction == 'x'){
        // Correct Points for Rect() with offset
        Point a( ul.x() + image.offset_x(), ul.y() + image.offset_y() );
        Point b( lr.x() + image.offset_x(), lr.y() + image.offset_y() );
        IntVector *proj_x = projection_rows(image, Rect(a, b));
        SplitPoints->push_back(ul.y()); // starting point
        
        for (size_t i = 1; i < proj_x->size(); i++) {
            if ((*proj_x)[i] <= noise) {
                gap_width++;
                if (Ty <= gap_width) {// min-gap <= act-gap?
                SplitPoints_Min[gap_counter] = (i + ul.y() - gap_width+1);
                SplitPoints_Max[gap_counter] = (i + ul.y()); // finally set to last point of gap
                }
            } 
            else {
                if (Ty <= gap_width)
                    gap_counter++;
                gap_width = 0;
            }
        }
    delete proj_x;
    }
    else{ // y-direction
        // Correct Points for Rect() with offset
        Point a( ul.x() + image.offset_x(), ul.y() + image.offset_y() );
        Point b( lr.x() + image.offset_x(), lr.y() + image.offset_y() );
        IntVector *proj_y = projection_cols(image, Rect(a, b));
        SplitPoints->push_back(ul.x()); // starting point
        
        for (size_t i = 1; i < proj_y->size(); i++) {
            if ((*proj_y)[i] <= noise) {
                gap_width++;
                if (Tx <= gap_width) {// min-gap <= act-gap?
                SplitPoints_Min[gap_counter] = (i + ul.x() - gap_width+1);
                SplitPoints_Max[gap_counter] = (i + ul.x()); // finally set to last point of gap
                }
            } 
            else {
                if (Tx <= gap_width) 
                    gap_counter++;
                gap_width = 0;
            }
        }
        delete proj_y;
    }
    
    for (int i=0; i<gap_counter; i++){
        if (0==gap_treatment){ // cut exactly in the middle of the gap -> no unlabeled noise pixels
            int mid = (SplitPoints_Min[i] + SplitPoints_Max[i]) / 2;
            SplitPoints_Min[i] = mid;
            SplitPoints_Max[i] = mid;
        }
        SplitPoints->push_back(SplitPoints_Min[i]);
        SplitPoints->push_back(SplitPoints_Max[i]);
    }   
    direction=='x'? SplitPoints->push_back(lr.y()): SplitPoints->push_back(lr.x()); // ending point
    
    return SplitPoints;
}



/* Function: Interne_RXY_Cut
 * This function recursively splits the image in horizontal or 
 * vertical direction.
 * The original image will have all of its pixels "labeled" with a number
 * representing each connected component.
 */
template<class T>
void projection_cutting_intern(T& image, Point ul, Point lr, ImageList* ccs, 
        int Tx, int Ty, int noise, int gap_treatment, char direction, int& label) {
    
    Point Start = proj_cut_Start_Point(image, ul, lr);
    Point End = proj_cut_End_Point(image, ul, lr);
    IntVector * SplitPoints = proj_cut_Split_Point(image, Start, End, Tx, Ty, noise, gap_treatment, direction);
    IntVector::iterator It;
    
    ul.x(Start.x());
    ul.y(Start.y());
    lr.x(End.x());
    lr.y(End.y());
    
        

    if (!(direction=='y' && SplitPoints->size() == 2)){ // ending condition, SplitPoints==2 => only Start- and Endpoint no gaps
        if (direction=='x'){
            direction = 'y';
            for(It = SplitPoints->begin(); It != SplitPoints->end(); It++){
                Point begin, end; // note the lowercase of end, which is not End
                begin.x(Start.x());
                begin.y(*It);
                It++;
                end.x(End.x());
                end.y(*It);
                projection_cutting_intern(image, begin, end, ccs, Tx, Ty, noise, gap_treatment, direction, label);
            }
        }
        else { // direction==y
            direction = 'x';
            for(It = SplitPoints->begin(); It != SplitPoints->end(); ++It){
                Point begin, end; // note the lowercase of end, which is not End
                begin.x(*It);
                begin.y(Start.y());
                It++;
                end.x(*It);
                end.y(End.y());
                projection_cutting_intern(image, begin, end, ccs, Tx, Ty, noise, gap_treatment, direction, label);
            }
        }
    } else {
        label++;
        for (size_t y = ul.y(); y <= lr.y(); y++) {
            for (size_t x = ul.x(); x <= lr.x(); x++) {
                if((image.get(Point(x, y))) != 0){
                    image.set(Point(x, y), label);
                }
            }
        }

        Point cc(Start.x() + image.offset_x(), Start.y() + image.offset_y());
        ccs->push_back(
                new ConnectedComponent<typename T::data_type>(
                    *((typename T::data_type*)image.data()),
                    OneBitPixel(label),
                    cc,
                    Dim((End.x() - Start.x() + 1), (End.y() - Start.y() + 1))
                )
            );
    }
    delete SplitPoints;
}

/*
 * Function: rxy_cut 
 * Returns a list of ccs found in the image.
 */
template<class T>
ImageList* projection_cutting(T& image, int Tx, int Ty, int noise, int gap_treatment) {
    int Label = 1;
    char direction = 'x';

    if (noise < 0) {
        noise = 0;
    }

    // set default values
    if (Tx < 1 || Ty < 1) {
        ImageList* ccs_temp = cc_analysis(image);
        int Median = pagesegmentation_median_height(ccs_temp);
        for (ImageList::iterator i = ccs_temp->begin(); 
                i != ccs_temp->end(); i++) {
            delete *i;
        }
        delete ccs_temp;
        if (Tx < 1) {
          Tx = Median * 7;
        }
        if (Ty < 1) {
          if (Median > 1) Ty = Median / 2;
          else Ty = 1;
        }
    }
    // set minimal gap_width
    /*if (Tx <= 2){
        if (gap_treatment)
            Tx=2;
        else
            Tx=3;
    }
    if (Ty <= 2){
        if (gap_treatment)
            Ty=2;
        else
            Ty=3;
    }*/

    ImageList* ccs = new ImageList();
    Point ul, lr;

    ul.x(0);
    ul.y(0);
    lr.x(image.ncols() - 1);
    lr.y(image.nrows() - 1);
    projection_cutting_intern(image, ul, lr, ccs, Tx, Ty, noise, gap_treatment, direction, Label);
    
    return ccs;
}




/*
sub_cc_analysis
@param cclist The list of CCs inside the image

@return A tuple with two values
    1. the image with the new labels from the new CCs
    2. a list of ImageLists
        a list-entry is a cc_analysis of a cclist from the argument
*/
template<class T>
PyObject* sub_cc_analysis(T& image, ImageVector &cclist) {
    unsigned int pos, max;
    unsigned int max_last = 1;
    int x, y;
    int label;
    int count;
    OneBitImageData *ret_image;
    OneBitImageView *ret_view;
    ImageVector::iterator iv;
    ImageList::iterator il;
    typename T::value_type Black = black(image);

    ret_image = new OneBitImageData(image.size(), image.origin());
    ret_view = new OneBitImageView(*ret_image, image.origin(), image.size());

    // Generate a list to store the CCs of all lines
    PyObject *return_cclist = PyList_New(cclist.size());
    
    for (iv = cclist.begin(), pos = 0; iv != cclist.end(); iv++, pos++) {
        Cc* cc = static_cast<Cc*>((*iv).first);

        // copy the needed CC from the original image(image) 
        // to the new image(ret_view)
        for (size_t i = 0; i < cc->ncols(); i++) {
            for (size_t j = 0; j < cc->nrows(); j++) {
                x = i + cc->offset_x();
                y = j + cc->offset_y();
                if (!is_white(image.get(Point(x, y)))) {
                    ret_view->set(Point(x, y), Black);
                }
            }
        }
        
        // generate a CC for the cc_analysis, it's simply a copy 
        // of one cclist entry
        ConnectedComponent<typename T::data_type> *cc_new = 
                new ConnectedComponent<typename T::data_type>(
                    *((typename T::data_type*)ret_view->data()),
                    OneBitPixel(cc->label()),
                    Point((*cc).offset_x(), (*cc).offset_y()),
                    (*cc).dim()
                );

        // cc_analysis of one list entry
        ImageList* ccs_orig = cc_analysis(*cc_new);
        delete cc_new;

        // Query the greatest label, or count only the list values
        max = ccs_orig->size();
        
        ImageList* return_ccs = new ImageList();
        il = ccs_orig->begin();
        count = 0;
        while (il != ccs_orig->end()) {
            Cc* cc = static_cast<Cc*>(*il);

            // Calculate new labels for the CCs.
            // cc_analysis makes no sequenced lables, so i make it
            label = max_last + count;
            
            return_ccs->push_back(
                    new ConnectedComponent<typename T::data_type>(
                        *((typename T::data_type*)ret_view->data()),
                        OneBitPixel(label),
                        cc->origin(),
                        cc->size()
                    )
                );
    
            // Renumbering the old labels to the new one
            for (int i = 0; i < (int)cc->ncols(); i++) {
                for (int j = 0; j < (int)cc->nrows(); j++) {
                    x = i + cc->offset_x();
                    y = j + cc->offset_y();
                    if (!is_white(ret_view->get(Point(x, y)))) {
                        ret_view->set(Point(x, y), label);
                    }
                }
            }

            // delete the temporary uses CCs from the cc_analysis
            delete *il;

            il++;
            count++;
        }
        // delete the ImageList
        delete ccs_orig;

        max_last += max;

        // Set the Imagelist into the PyList
        // ImageList must be converted to be a valid datatype for the PyList
        PyList_SetItem(return_cclist, pos, ImageList_to_python(return_ccs));
        delete return_ccs;
    }

    // Finaly create the return type, a tuple with a image 
    // and a list of ImageLists
    PyObject *return_values = PyTuple_New(2);
    PyTuple_SetItem(return_values, 0, create_ImageObject(ret_view));
    PyTuple_SetItem(return_values, 1, return_cclist);

    return return_values;
}

} // end of namespace Gamera

#endif

