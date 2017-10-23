#include <cradle/imaging/level_window.hpp>

namespace cradle {

uint8_t apply_level_window(double level, double window, double image_value)
{
    double x = image_value - (level - window / 2);
    if (x < 0)
        return 0;
    if (x >= window)
        return 0xff;
    int y = int(x / window * 0x100);
    return y > 0xff ? 0xff : uint8_t(y);
}

}
