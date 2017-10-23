#include <cradle/io/standard_image_io.hpp>
#include <cradle/imaging/iterator.hpp>
#include <cradle/imaging/channel.hpp>
#include <boost/filesystem/operations.hpp>
#include <cmath>
#include <cradle/io/file.hpp>

#define BOOST_TEST_MODULE file_io
#include <cradle/imaging/test.hpp>

using namespace cradle;

template<class SP>
void make_color_square(image<2,alia::rgb8,SP> const& img)
{
    alia::rgb<double> p(0, 0, 0),
        x_inc(0, 0, 255.0 / (img.size[0] - 1)),
        y_inc(255.0 / (img.size[1] - 1), 0, 0);
    for (unsigned y = 0; y < img.size[1]; ++y)
    {
        auto row_start = p;
        for (auto i = get_row_begin(img, y); i != get_row_end(img, y); ++i)
        {
            p.g = std::pow(p.r * p.b, 0.75);
            i->r = cradle::uint8_t(p.r);
            p.r += x_inc.r;
            i->g = cradle::uint8_t(p.g);
            p.g += x_inc.g;
            i->b = cradle::uint8_t(p.b);
            p.b += x_inc.b;
        }
        p.r = row_start.r + y_inc.r;
        p.g = row_start.g + y_inc.g;
        p.b = row_start.b + y_inc.b;
    }
}

file_path const image_path = "img.png";

BOOST_AUTO_TEST_CASE(color_image_test)
{
    image<2,alia::rgb8,unique> img;
    create_image(img, make_vector<unsigned>(24, 24));
    make_color_square(img);
    write_image_file(image_path, img);

    image<2,alia::rgb8,shared> img2;
    read_image_file(img2, image_path);
    CRADLE_CHECK_IMAGE(img, img2.pixels.view,
        img2.pixels.view + product(img2.size))

    remove(image_path);
}

BOOST_AUTO_TEST_CASE(try_gray_load_test)
{
    image<2,alia::rgb8,unique> img;
    create_image(img, make_vector<unsigned>(24, 24));
    make_color_square(img);
    write_image_file(image_path, img);

    image<2,alia::rgb<cradle::uint16_t>,shared> img2;
    BOOST_CHECK_THROW(read_image_file(img2, image_path), image_type_mismatch);

    remove(image_path);
}
