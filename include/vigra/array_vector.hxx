/************************************************************************/
/*                                                                      */
/*               Copyright 2002-2004 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.3.0, Sep 10 2004 )                                    */
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

#ifndef VIGRA_ARRAY_VECTOR_HXX
#define VIGRA_ARRAY_VECTOR_HXX

#include <memory>
#include <algorithm>
#include <vigra/memory.hxx>

namespace vigra
{

/** Replacement for <tt>std::vector</tt>.
    
    This template implements the same functionality as <tt>std::vector</tt>.
    However, it gives two usful guarantees, that <tt>std::vector</tt> fails 
    to provide:
    
    <ul>
    <li>The memory is always allocated as one contigous piece</li>
    <li>The iterator is always a <TT>T *</TT> </li>
    </ul>
    
    This means that memory managed by <tt>ArrayVector</tt> can be passed
    to algorithms that expect raw memory. This is especially important
    when lagacy or C code has to be called, but it is also useful for certain
    optimizations.
    
    Refer to the documentation of <tt>std::vector</tt> for a detailed 
    description of <tt>ArrayVector</tt> functionality.

    <b>\#include</b> "<a href="array_vector_8hxx-source.html">vigra/array_vector.hxx</a>"<br>
    Namespace: vigra
*/
template <class T, class Alloc = std::allocator<T> >
class ArrayVector
{
    typedef ArrayVector<T, Alloc> this_type;

public:
    typedef T value_type;
    typedef value_type & reference;
    typedef value_type const & const_reference;
    typedef value_type * pointer;
    typedef value_type const * const_pointer;
    typedef value_type * iterator;
    typedef value_type const * const_iterator;
    typedef unsigned int size_type;
    typedef int          difference_type;
    typedef Alloc        allocator_type;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

public:
    ArrayVector();

    explicit ArrayVector(Alloc const & alloc);

    explicit ArrayVector( size_type size, Alloc const & alloc = Alloc());

    ArrayVector( size_type size, value_type const & initial, Alloc const & alloc = Alloc());

    ArrayVector( this_type const & rhs );

    template <class InputIterator>
    ArrayVector(InputIterator i, InputIterator end);

    template <class InputIterator>
    ArrayVector(InputIterator i, InputIterator end, Alloc const & alloc);

    this_type & operator=( this_type const & rhs );

    ~ArrayVector();

    inline const_pointer data() const
    {
        return data_;
    }

    inline pointer data()
    {
        return data_;
    }

    inline const_iterator begin() const
    {
        return data();
    }

    inline iterator begin()
    {
        return data();
    }

    inline const_iterator end() const
    {
        return data() + size();
    }

    inline iterator end()
    {
        return data() + size();
    }

    inline reverse_iterator rbegin()
    {
        return (reverse_iterator(end()));
    }

    inline const_reverse_iterator rbegin() const
    {
        return (const_reverse_iterator(end()));
    }

    inline reverse_iterator rend()
    {    
        return (reverse_iterator(begin()));
    }

    inline const_reverse_iterator rend() const
    {    
        return (const_reverse_iterator(begin()));
    }

    reference front()
    {
        return *data_;
    }

    const_reference front() const
    {
        return *data_;
    }

    reference back()
    {
        return data_[size_-1];
    }

    const_reference back() const
    {
        return data_[size_-1];
    }

    reference operator[]( size_type i )
    {
        return data()[i];
    }

    const_reference operator[]( size_type i ) const
    {
        return data()[i];
    }

    void pop_back();

    void push_back( value_type const & t );

    iterator insert(iterator p, value_type const & v);

    iterator insert(iterator p, size_type n, value_type const & v);

    template <class InputIterator>
    iterator insert(iterator p, InputIterator i, InputIterator iend);

    iterator erase(iterator p);

    iterator erase(iterator p, iterator q);

    void clear();

    void reserve( size_type new_capacity );

    void reserve();

    void resize( size_type new_size, value_type const & initial );

    void resize( size_type new_size )
    {
        resize(new_size, value_type());
    }

    bool empty() const
    {
        return size_ == 0;
    }

    size_type size() const
    {
        return size_;
    }

    size_type capacity() const
    {
        return capacity_;
    }

    void swap(this_type & rhs);

  private:

    void deallocate(pointer data, size_type size);

    pointer reserve_raw(size_type capacity);

    Alloc alloc_;
    size_type size_, capacity_;
    pointer data_;
};

template <class T, class Alloc>
ArrayVector<T, Alloc>::ArrayVector()
: alloc_(Alloc()),
  size_(0),
  capacity_(5),
  data_(reserve_raw(5))
{}

template <class T, class Alloc>
ArrayVector<T, Alloc>::ArrayVector(Alloc const & alloc)
: alloc_(alloc),
  size_(0),
  capacity_(5),
  data_(reserve_raw(5))
{}

template <class T, class Alloc>
ArrayVector<T, Alloc>::ArrayVector( size_type size, Alloc const & alloc)
: alloc_(alloc),
  size_(size),
  capacity_(size),
  data_(reserve_raw(size))
{
    if(size_ > 0)
        std::uninitialized_fill(data_, data_+size_, value_type());
}

template <class T, class Alloc>
ArrayVector<T, Alloc>::ArrayVector( size_type size, 
                         value_type const & initial, Alloc const & alloc)
: alloc_(alloc),
  size_(size),
  capacity_(size),
  data_(reserve_raw(size))
{
    if(size_ > 0)
        std::uninitialized_fill(data_, data_+size_, initial);
}

template <class T, class Alloc>
ArrayVector<T, Alloc>::ArrayVector( this_type const & rhs )
: alloc_(rhs.alloc_),
  size_(rhs.size_),
  capacity_(rhs.capacity_),
  data_(reserve_raw(rhs.capacity_))
{
    if(size_ > 0)
        std::uninitialized_copy(rhs.data_, rhs.data_+size_, data_);
}

template <class T, class Alloc>
template <class InputIterator>
ArrayVector<T, Alloc>::ArrayVector(InputIterator i, InputIterator end)
: alloc_(),
  size_(std::distance(i, end)),
  capacity_(size_),
  data_(reserve_raw(size_))
{
    std::uninitialized_copy(i, end, data_);
}

template <class T, class Alloc>
template <class InputIterator>
ArrayVector<T, Alloc>::ArrayVector(InputIterator i, InputIterator end, Alloc const & alloc)
: alloc_(alloc),
  size_(std::distance(i, end)),
  capacity_(size_),
  data_(reserve_raw(size_))
{
    std::uninitialized_copy(i, end, data_);
}


template <class T, class Alloc>
ArrayVector<T, Alloc> & ArrayVector<T, Alloc>::operator=( this_type const & rhs )
{
    if(this == &rhs)
        return *this;
    ArrayVector new_vector(rhs);
    swap(new_vector);
    return *this;
}

template <class T, class Alloc>
ArrayVector<T, Alloc>::~ArrayVector()
{
    deallocate(data_, size_);
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::pop_back()
{
    --size_;
    alloc_.destroy(data_ + size_);
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::push_back( value_type const & t )
{
    reserve();
    alloc_.construct(data_ + size_, t);
    ++size_;
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::clear()
{
    detail::destroy_n(data_, size_);
    size_ = 0;
}

template <class T, class Alloc>
typename ArrayVector<T, Alloc>::iterator
ArrayVector<T, Alloc>::insert(iterator p, value_type const & v)
{
    difference_type pos = p - begin();
    if(p == end())
    {
        push_back(v);
        p = begin() + pos;
    }
    else
    {
        push_back(back());
        p = begin() + pos;
        std::copy_backward(p, end() - 2, end() - 1);
        *p = v;
    }
    return p;
}

template <class T, class Alloc>
typename ArrayVector<T, Alloc>::iterator
ArrayVector<T, Alloc>::insert(iterator p, size_type n, value_type const & v)
{
    difference_type pos = p - begin();
    size_type new_size = size() + n;
    if(new_size >= capacity_)
    {
        pointer new_data = reserve_raw(new_size);
        std::uninitialized_copy(begin(), p, new_data);
        std::uninitialized_fill(new_data + pos, new_data + pos + n, v);
        std::uninitialized_copy(p, end(), new_data + pos + n);
        deallocate(data_, size_);
        capacity_ = new_size;
        data_ = new_data;
    }
    else if(pos + n >= size_)
    {
        size_type diff = pos + n - size_;
        std::uninitialized_copy(p, end(), end() + diff);
        std::uninitialized_fill(end(), end() + diff, v);
        std::fill(p, end(), v);
    }
    else
    {
        size_type diff = size_ - (pos + n);
        std::uninitialized_copy(end() - n, end(), end());
        std::copy_backward(p, p + diff, end());
        std::fill(p, p + n, v);
    }
    size_ = new_size;
    return begin() + pos;
}

template <class T, class Alloc>
template <class InputIterator>
typename ArrayVector<T, Alloc>::iterator
ArrayVector<T, Alloc>::insert(iterator p, InputIterator i, InputIterator iend)
{
    difference_type n = iend - i;
    difference_type pos = p - begin();
    size_type new_size = size() + n;
    if(new_size >= capacity_)
    {
        pointer new_data = reserve_raw(new_size);
        std::uninitialized_copy(begin(), p, new_data);
        std::uninitialized_copy(i, iend, new_data + pos);
        std::uninitialized_copy(p, end(), new_data + pos + n);
        deallocate(data_, size_); // removed std:: MGD
        capacity_ = new_size;
        data_ = new_data;
    }
    else if(pos + n >= size_)
    {
        size_type diff = pos + n - size_;
        std::uninitialized_copy(p, end(), end() + diff);
        std::uninitialized_copy(iend - diff, iend, end());
        std::copy(i, iend - diff, p);
    }
    else
    {
        size_type diff = size_ - (pos + n);
        std::uninitialized_copy(end() - n, end(), end());
        std::copy_backward(p, p + diff, end());
        std::copy(i, iend, p);
    }
    size_ = new_size;
    return begin() + pos;
}

template <class T, class Alloc>
typename ArrayVector<T, Alloc>::iterator
ArrayVector<T, Alloc>::erase(iterator p)
{
    std::copy(p+1, end(), p);
    pop_back();
    return p;
}

template <class T, class Alloc>
typename ArrayVector<T, Alloc>::iterator
ArrayVector<T, Alloc>::erase(iterator p, iterator q)
{
    std::copy(q, end(), p);
    size_type eraseCount = q - p;
    detail::destroy_n(end() - eraseCount, eraseCount);
    size_ -= eraseCount;
    return p;
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::reserve( size_type new_capacity )
{
    if(new_capacity <= capacity_)
        return;
    pointer new_data = reserve_raw(new_capacity);
    if(size_ > 0)
        std::uninitialized_copy(data_, data_+size_, new_data);
    deallocate(data_, size_);
    data_ = new_data;
    capacity_ = new_capacity;
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::reserve()
{
    if(size_ == capacity_)
        reserve(2*capacity_);
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::resize( size_type new_size, value_type const & initial)
{
    if(new_size < size_)
        erase(begin() + new_size, end());
    else if(size_ < new_size)
    {
        insert(end(), new_size - size(), initial);
    }
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::swap(this_type & rhs)
{
    std::swap(size_, rhs.size_);
    std::swap(capacity_, rhs.capacity_);
    std::swap(data_, rhs.data_);
}

template <class T, class Alloc>
void ArrayVector<T, Alloc>::deallocate(pointer data, size_type size)
{
    if(data)
    {
        detail::destroy_n(data, size);
        alloc_.deallocate(data, size);
    }
}

template <class T, class Alloc>
typename ArrayVector<T, Alloc>::pointer
ArrayVector<T, Alloc>::reserve_raw(size_type capacity)
{
    pointer data = 0;
    if(capacity)
    {
        data = alloc_.allocate(capacity);
    }
    return data;
}

} // namespace vigra


#endif /* VIGRA_ARRAY_VECTOR_HXX */
