#include <cradle_test_utilities.hpp>

#include <cradle/common.hpp>
#include <cradle/io/generic_io.hpp>
#include <cradle/io/vtk_io.hpp>

using namespace cradle;

namespace unittests
{

double
distance(vector3d p1, vector3d p2)
{
    double d = 0;
    double xSqr = (p2[0] - p1[0]) * (p2[0] - p1[0]);
    double ySqr = (p2[1] - p1[1]) * (p2[1] - p1[1]);
    double zSqr = (p2[2] - p1[2]) * (p2[2] - p1[2]);
    double mySqr = xSqr + ySqr + zSqr;
    d = sqrt(mySqr);

    return d;
}

blob
readFileToBlob(string fileName)
{
    blob dicomBlob;

    std::ifstream is(fileName, std::ifstream::binary);
    if (is) {
        is.seekg(0, is.end);
        dicomBlob.size = is.tellg();
        is.seekg(0, is.beg);
        char * buffer = new char[dicomBlob.size];
        is.read(buffer, dicomBlob.size);
        if (is)
        {
            dicomBlob.data = buffer;
            is.close();
        }
        else { is.close(); }
    }
    return dicomBlob;
}

}