#include <cradle/io/standard_image_io.hpp>
#include <cradle/imaging/contiguous.hpp>
#include <boost/noncopyable.hpp>
#include <boost/format.hpp>
#include <cradle/io/file.hpp>
#include <cradle/external/devil.hpp>
#include <alia/color.hpp>

namespace cradle {

// Check for pending DevIL errors.
// If one is found, a corresponding exception is thrown.
// All errors are cleared out after a call to this function.
static void check_devil_errors(file_path const& file)
{
    ILenum error = ilGetError();
    while (ilGetError() != IL_NO_ERROR)
        ;
    if (error != IL_NO_ERROR)
    {
        throw image_io_error(
            file,
            str(boost::format("DevIL error: %s (0x%x)")
                % string(iluErrorString(error))
                % static_cast<unsigned>(error)));
    }
}

static void initialize_devil(file_path const& file)
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
    {
        throw image_io_error(
            file, "DevIL DLL version is older than compiled version.");
    }
    ilInit();
    ilEnable(IL_FILE_OVERWRITE);
    iluInit();
    check_devil_errors(file);
}

ILenum get_devil_file_format(image_file_format format)
{
    switch (format)
    {
        case image_file_format::AUTO:
            return IL_TYPE_UNKNOWN;
        case image_file_format::PNG:
            return IL_PNG;
        case image_file_format::TIFF:
            return IL_TIF;
        case image_file_format::BMP:
            return IL_BMP;
        case image_file_format::JPEG:
            return IL_JPG;
        case image_file_format::TGA:
            return IL_TGA;
        case image_file_format::RAW:
            return IL_RAW;
        case image_file_format::PCX:
            return IL_PCX;
        case image_file_format::PNM:
            return IL_PNM;
        default:
            return IL_TYPE_UNKNOWN;
    }
}

struct create_variant_image_fn
{
    image<2,variant,shared> result;
    vector2u size;
    variant_type_info type_info;
    void const* pixels;
    template<class Pixel>
    void operator()(Pixel)
    {
        image<2,Pixel,unique> tmp;
        create_image(tmp, size);
        size_t image_size = product(size) * sizeof(Pixel);
        memcpy(tmp.pixels.ptr, pixels, image_size);
        image<2,Pixel,shared> img;
        img = share(tmp);
        result = as_variant(img);
    }
};

void interpret_devil_type_info(
    variant_type_info& type_info,
    ILenum image_format, ILenum image_type,
    file_path const& file)
{
    switch (image_format)
    {
      case IL_RGB:
        type_info.format = pixel_format::RGB;
        break;
      case IL_RGBA:
        type_info.format = pixel_format::RGBA;
        break;
      case IL_LUMINANCE:
        type_info.format = pixel_format::GRAY;
        break;
      default:
        throw image_io_error(file, "unsupported pixel format");
    }

    switch (image_type)
    {
      case IL_BYTE:
        type_info.type = channel_type::INT8;
        break;
      case IL_UNSIGNED_BYTE:
        type_info.type = channel_type::UINT8;
        break;
      case IL_SHORT:
        type_info.type = channel_type::INT16;
        break;
      case IL_UNSIGNED_SHORT:
        type_info.type = channel_type::UINT16;
        break;
      case IL_INT:
        type_info.type = channel_type::INT32;
        break;
      case IL_UNSIGNED_INT:
        type_info.type = channel_type::UINT32;
        break;
      case IL_FLOAT:
        type_info.type = channel_type::FLOAT;
        break;
      case IL_DOUBLE:
        type_info.type = channel_type::DOUBLE;
        break;
      default:
        throw image_io_error(file, "unsupported channel type");
    }
}

void read_image_file(
    image<2,variant,shared>& img,
    file_path const& file,
    image_file_format format)
{
    initialize_devil(file);

    ILenum devil_file_format = get_devil_file_format(format);

    auto file_as_string = file.string<string>();
    ilLoad(devil_file_format,
        const_cast<ILstring>(file_as_string.c_str()));
    check_devil_errors(file);

    vector2u size;
    size[0] = ilGetInteger(IL_IMAGE_WIDTH);
    size[1] = ilGetInteger(IL_IMAGE_HEIGHT);
    if (ilGetInteger(IL_IMAGE_DEPTH) > 1)
        throw image_io_error(file, "wrong dimensionality");

    variant_type_info type_info;
    interpret_devil_type_info(type_info, ilGetInteger(IL_IMAGE_FORMAT),
        ilGetInteger(IL_IMAGE_TYPE), file);

    create_variant_image_fn fn;
    fn.type_info = type_info;
    fn.size = size;
    fn.pixels = ilGetData();
    dispatch_variant(type_info, fn);

    swap(fn.result, img);
}

class devil_image : boost::noncopyable
{
 public:
    devil_image() { ilGenImages(1, &name_); }
    ~devil_image() { ilDeleteImages(1, &name_); }
    void bind() const { ilBindImage(name_); }
 private:
    ILuint name_;
};

void write_image_file(
    file_path const& file,
    image<2,variant,const_view> const& img,
    image_file_format format)
{
    assert(is_contiguous(img));

    initialize_devil(file);

    ILenum devil_file_format = get_devil_file_format(format);

    ILenum devil_format;
    switch (img.pixels.type_info.format)
    {
     case pixel_format::RGB:
        devil_format = IL_RGB;
        break;
     case pixel_format::RGBA:
        devil_format = IL_RGBA;
        break;
     case pixel_format::GRAY:
        devil_format = IL_LUMINANCE;
        break;
     default:
        throw image_io_error(file, "unsupported pixel format");
    }

    ILenum devil_type;
    switch (img.pixels.type_info.type)
    {
     case channel_type::INT8:
        devil_type = IL_BYTE;
        break;
     case channel_type::UINT8:
        devil_type = IL_UNSIGNED_BYTE;
        break;
     case channel_type::INT16:
        devil_type = IL_SHORT;
        break;
     case channel_type::UINT16:
        devil_type = IL_UNSIGNED_SHORT;
        break;
     case channel_type::INT32:
        devil_type = IL_INT;
        break;
     case channel_type::UINT32:
        devil_type = IL_UNSIGNED_INT;
        break;
     case channel_type::FLOAT:
        devil_type = IL_FLOAT;
        break;
     case channel_type::DOUBLE:
        devil_type = IL_DOUBLE;
        break;
     default:
        throw image_io_error(file, "unsupported channel type");
    }

    contiguous_view<2,variant> cv(img);

    devil_image di;
    di.bind();
    ilTexImage(img.size[0], img.size[1], 1,
        get_channel_count(img.pixels.type_info.format),
        devil_format, devil_type, const_cast<void*>(cv.get().pixels.view));
    check_devil_errors(file);

    // DevIL's convention for row ordering seems to be upside-down with
    // respect to ours, so we need to flip the image to correct it.
    iluFlipImage();
    check_devil_errors(file);

    auto file_as_string = file.string<string>();
    //ilSave(devil_file_format,
    auto devil_file = const_cast<ILstring>(file_as_string.c_str());
    ilSaveImage(devil_file);
    check_devil_errors(file);
}

void read_image_file(
    image<2,int16_t,shared>& img,
    file_path const& file,
    image_file_format format)
{
    initialize_devil(file);

    ILenum devil_file_format = get_devil_file_format(format);

    auto file_as_string = file.string<string>();
    ilLoad(devil_file_format,
        const_cast<ILstring>(file_as_string.c_str()));
    check_devil_errors(file);

    vector2u size;
    size[0] = ilGetInteger(IL_IMAGE_WIDTH);
    size[1] = ilGetInteger(IL_IMAGE_HEIGHT);
    if (ilGetInteger(IL_IMAGE_DEPTH) > 1)
        throw image_io_error(file, "wrong dimensionality");

    variant_type_info type_info;
    interpret_devil_type_info(type_info, ilGetInteger(IL_IMAGE_FORMAT),
        ilGetInteger(IL_IMAGE_TYPE), file);

    image<2,int16_t,unique> tmp;
    create_image(tmp, size);
    size_t image_size = product(size) * sizeof(int16_t);
    memcpy(tmp.pixels.ptr, ilGetData(), image_size);
    img = share(tmp);
}

image2 read_standard_image_file(file_path const& file)
{
    image2 img;
    read_image_file(img, file);
    return img;
}

void write_standard_image_file(file_path const& file, image2 const& img)
{
    write_image_file(file, img);
}

}
