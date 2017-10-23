#include <cradle/io/calc_provider.hpp>
#include <cradle/api_index.hpp>
#include <cradle/api.hpp>
#include <cradle/io/generic_io.hpp>
#include <cradle/imaging/api.hpp>
#include <cradle/imaging/variant.hpp>

using namespace cradle;

int main(int argc, char const* const* argv)
{
    try
    {
        api_implementation cradle_api;
        CRADLE_REGISTER_APIS(cradle_api);
        register_image_types(cradle_api);

        auto json = get_manifest_json(cradle_api);
        FILE* f = fopen("cradle_api.json", "wb");
        fprintf(f, "%s\n", json.c_str());
        fclose(f);

        return 0;
    }
    catch (std::exception& e)
    {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
