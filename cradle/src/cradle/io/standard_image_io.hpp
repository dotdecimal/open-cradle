#ifndef CRADLE_IO_STANDARD_IMAGE_IO_HPP
#define CRADLE_IO_STANDARD_IMAGE_IO_HPP

#include <cradle/imaging/image.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/io/file.hpp>
#include <cradle/imaging/contiguous.hpp>

namespace cradle {

class image_io_error : public file_error
{
 public:
    image_io_error(
        file_path const& file, string const& msg)
      : file_error(file, msg)
    {}
    ~image_io_error() throw() {}
};

// supported file formats
enum class image_file_format
{
    AUTO,
    PNG,
    TIFF,
    BMP,
    JPEG,
    TGA,
    RAW,
    PCX,
    PNM
};

// Read an image file into a variant image.
// If the file format parameter is omitted, the function will attempt to
// determine the format itself by looking at the file header.
void read_image_file(
    image<2,variant,shared>& img,
    file_path const& file,
    image_file_format format = image_file_format::AUTO);

template<class Pixel>
void read_image_file(
    image<2,Pixel,shared>& img,
    file_path const& file,
    image_file_format format = image_file_format::AUTO)
{
    image<2,variant,shared> tmp;
    read_image_file(tmp, file, format);
    try
    {
        img = cast_variant<Pixel>(tmp);
    }
    catch (cradle::exception& e)
    {
        e.add_context("while attempting to read: " + file.string());
        throw;
    }
}

// Read a 2D image stored in one of the standard image file formats.
//
// Supported formats include PNG, TIFF, BMP, JPEG, TGA, RAW, PCX, and PNM.
//
image2 read_standard_image_file(file_path const& file);

// DevIL seems to read signed 16-bit images as unsigned, so we need a special
// overload for them.
void read_image_file(
    image<2,int16_t,shared>& img,
    file_path const& file,
    image_file_format format = image_file_format::AUTO);

// Write an image to disk.
// If the file format parameter is omitted, the library will attempt to
// determine the format itself by looking at the file name.
void write_image_file(
    file_path const& file,
    image<2,variant,const_view> const& img,
    image_file_format format = image_file_format::AUTO);

template<class Pixel, class Storage>
void write_image_file(
    file_path const& file,
    image<2,Pixel,Storage> const& img,
    image_file_format format = image_file_format::AUTO)
{
    write_image_file(file, cast_image<image_view2>(img), format);
}

// Write a 2D image to a file in one of the standard image formats.
//
// Supported formats include PNG, TIFF, BMP, JPEG, TGA, RAW, PCX, and PNM.
//
void write_standard_image_file(file_path const& file, image2 const& img);

}

#endif
