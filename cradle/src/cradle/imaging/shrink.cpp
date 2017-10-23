#include <cradle/imaging/shrink.hpp>

namespace cradle {

namespace {

    struct shrink_fn
    {
        image<2,variant,shared>* result;
        unsigned factor;
        template<class Pixel>
        void operator()(image<2,Pixel,const_view> const& img)
        {
            image<2,Pixel,unique> a;
            shrink_image(a, img, factor);
            image<2,Pixel,shared> b;
            b = share(a);
            *result = as_variant(b);
        }
    };

}

void shrink_image(image<2,variant,shared>& result,
    image<2,variant,const_view> const& src, unsigned factor)
{
    shrink_fn fn;
    fn.result = &result;
    fn.factor = factor;
    apply_fn_to_gray_variant(fn, src);
}

}
