namespace cradle {

template<class Pixel, class SrcSP>
void shrink_image(image<2,Pixel,unique>& result,
    image<2,Pixel,SrcSP> const& src, unsigned factor)
{
    vector2u result_size;
    for (unsigned i = 0; i < 2; ++i)
        result_size[i] = src.size[i] / factor;
    create_image(result, result_size);
    copy_value_mapping(result, src);
    copy_spatial_mapping(result, src);
    for (unsigned i = 0; i < 2; ++i)
        result.axes[i] *= factor;
    Pixel* r = get_iterator(result.pixels);
    double one_over_factor_squared = 1. / (factor * factor);
    for (int i = 0; i != result_size[1]; ++i)
    {
        typename image<2,Pixel,SrcSP>::iterator_type src_p =
            get_iterator(src.pixels) + i * factor * src.step[1];
        for (int j = 0; j != result_size[0]; ++j, ++r,
            src_p += factor * src.step[0])
        {
            double sum = 0;
            for (unsigned k = 0; k != factor; ++k)
            {
                typename image<2,Pixel,SrcSP>::iterator_type s =
                    src_p + k * src.step[1];
                typename image<2,Pixel,SrcSP>::iterator_type s_end =
                    s + factor * src.step[0];
                for (; s != s_end; s += src.step[0])
                    sum += *s;
            }
            // TODO: round integer pixel types
            *r = Pixel(sum * one_over_factor_squared);
        }
    }
}

}
