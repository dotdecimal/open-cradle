#ifndef CRADLE_IMAGING_ITERATOR_HPP
#define CRADLE_IMAGING_ITERATOR_HPP

#include <cradle/imaging/image.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace cradle {

template<unsigned N, class Pixel, class Storage>
class image_iterator
  : public boost::iterator_facade<
        image_iterator<N,Pixel,Storage>
      , typename boost::remove_reference<
            typename image<N,Pixel,Storage>::reference_type>::type
      , boost::forward_traversal_tag
    >
{
 private:
  struct enabler {};

 public:
    image_iterator()
      : img_(0), pixel_(0) {}

    image_iterator(image<N,Pixel,Storage> const& img,
        vector<N,unsigned> const& index)
      : img_(&img), index_(index), pixel_(get_pixel_iterator(img, index))
    {}

 private:
    friend class boost::iterator_core_access;

    bool equal(image_iterator const& other) const
    { return this->pixel_ == other.pixel_; }

    void increment()
    {
        for (unsigned i = 0; i < N; ++i)
        {
            ++index_[i];
            pixel_ += img_->step[i];
            if (index_[i] < img_->size[i] || i == N - 1)
                break;
            index_[i] = 0;
            pixel_ -= img_->size[i] * img_->step[i];
        }
    }

    typename image<N,Pixel,Storage>::reference_type dereference() const
    { return *pixel_; }

    image<N,Pixel,Storage> const* img_;
    vector<N,unsigned> index_;
    typename image<N,Pixel,Storage>::iterator_type pixel_;
};

template<unsigned N, class Pixel, class Storage>
image_iterator<N,Pixel,Storage> get_begin(image<N,Pixel,Storage> const& img)
{
    return image_iterator<N,Pixel,Storage>(
        img, uniform_vector<N,unsigned>(0));
}

template<unsigned N, class Pixel, class Storage>
image_iterator<N,Pixel,Storage> get_end(image<N,Pixel,Storage> const& img)
{
    vector<N,unsigned> index;
    for (unsigned i = 0; i < N - 1; ++i)
        index[i] = 0;
    index[N - 1] = img.size[N - 1];
    return image_iterator<N,Pixel,Storage>(img, index);
}

template<unsigned N, class Pixel, class Storage>
class span_iterator
  : public boost::iterator_facade<
        span_iterator<N,Pixel,Storage>
      , typename boost::remove_reference<
            typename image<N,Pixel,Storage>::reference_type>::type
      , boost::forward_traversal_tag
    >
{
 private:
  struct enabler {};

 public:
    span_iterator()
      : pixel_(0) {}

    span_iterator(image<N,Pixel,Storage> const& img,
        unsigned axis,
        typename Storage::template iterator_type<N,Pixel>::type pixel)
      : pixel_(pixel), step_(img.step[axis])
    {}

 private:
    friend class boost::iterator_core_access;

    bool equal(span_iterator const& other) const
    { return this->pixel_ == other.pixel_; }

    void increment() { pixel_ += step_; }
    typename Storage::template reference_type<N,Pixel>::type
    dereference() const
    { return *pixel_; }

    typename Storage::template iterator_type<N,Pixel>::type pixel_;
    typename Storage::template step_type<N,Pixel>::type step_;
};

template<class Pixel, class Storage>
span_iterator<2,Pixel,Storage> get_row_begin(
    image<2,Pixel,Storage> const& img, unsigned y)
{
    return span_iterator<2,Pixel,Storage>(
        img, 0, get_iterator(img.pixels) + y * img.step[1]);
}
template<class Pixel, class Storage>
span_iterator<2,Pixel,Storage> get_row_end(
    image<2,Pixel,Storage> const& img, int y)
{
    return span_iterator<2,Pixel,Storage>(
        img, 0, get_iterator(img.pixels) + y * img.step[1] +
        img.size[0] * img.step[0]);
}

template<unsigned N, class Pixel, class Storage>
span_iterator<N,Pixel,Storage> get_axis_iterator(
    image<N,Pixel,Storage> const& img,
    unsigned axis, vector<N,unsigned> const& index)
{
    return span_iterator<N,Pixel,Storage>(
        img, axis, get_pixel_iterator(img, index));
}

}

#endif
