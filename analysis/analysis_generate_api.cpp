#include <analysis/api_index.hpp>
#include <dosimetry/api_index.hpp>
#include <cradle/io/generic_io.hpp>
#include <cradle/imaging/api.hpp>

#include <visualization/api_index.hpp>

using namespace cradle;
using namespace dosimetry;
using namespace analysis;

int main(int argc, char const* const* argv)
{
    try
    {
        api_implementation analysis_api;
        ANALYSIS_REGISTER_APIS(analysis_api);

        auto json = get_manifest_json(analysis_api);
        FILE* f = fopen("analysis_api.json", "wb");
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
