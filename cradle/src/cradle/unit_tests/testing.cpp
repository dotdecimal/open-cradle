/*
 * Author(s):  Daniel Bottomley <dbottomley@dotdecimal.com>, Kevin Erhart
 * Date:       09/15/2014
 *
 */

#include <cradle/unit_tests/testing.hpp>
 #ifdef _WIN32
#include <windows.h> // WinApi header
 #endif

using namespace cradle;

void cradle::write_test_header(unit_test &ut)
{
        ut.stream << "#########################################################\n";
        ut.stream << "App Name: " << ut.appName << "\n";
        ut.stream << "Run Date: " << ut.startTime << "\n";
        ut.stream << "#########################################################\n";
}

void cradle::write_section_header(unit_test &ut, string sectionName)
{
        ut.stream << "\n#########################################################\n";
        ut.stream << "Section: " << sectionName << "\n";
        ut.stream << "#########################################################\n\n";
}

void cradle::write_result(unit_test &ut, string functionName, bool result)
{
        #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        #endif

        ut.stream << "---------------------------------------------------------\n";
        ut.stream << "Function Name: " << functionName << "\n";
        ut.stream << "  Test Result: " << (result ? "Passed" : "Failed") << "\n";

        #ifdef _WIN32
        SetConsoleTextAttribute(hConsole, 11);
        #endif
        std::cout << "  Function: " << functionName << std::endl;
        if (result)
        {
                #ifdef _WIN32
                SetConsoleTextAttribute(hConsole, 10);
                #endif
                std::cout << "  Result:   Passed" << std::endl;
        }
        else
        {
                #ifdef _WIN32
                SetConsoleTextAttribute(hConsole, 12);
                #endif
                std::cout << "  Result:   Failed" << std::endl;
        }
        //std::cout << "  Result: " << (result ? "Passed" : "Failed") << ",  Function: " << functionName << std::endl;

        if (result)        { ++ut.passedCount;}
        else{ ++ut.failedCount; }
}

void cradle::write_result_direct(unit_test &ut, string functionName, bool is_first)
{
        #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7);
        #endif
        if (is_first) { ut.stream << "     Directly tests the following:\n"; }
        ut.stream << "        " << functionName << "\n";

        if (is_first) { std::cout << "     Directly tests the following:\n"; }
        std::cout << "        " << functionName << std::endl;
}

void cradle::write_result_indirect(unit_test &ut, string functionName, bool is_first)
{
        #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7);
        #endif
        if (is_first) { ut.stream << "     Indirectly tests the following:\n"; }
        ut.stream << "        " << functionName << "\n";

        if (is_first) { std::cout << "     Indirectly tests the following:\n"; }
        std::cout << "        " << functionName << std::endl;
}

void cradle::write_test_summary(unit_test &ut, cradle::time endTime)
{
        ut.stream << "#########################################################\n";
        ut.stream << "Total Tests: " << ut.passedCount + ut.failedCount << ",  Passed: " << ut.passedCount << ",  Failed: " << ut.failedCount << "\n";
        ut.stream << "Time Taken: " << endTime - ut.startTime << "\n";

        #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7);
        #endif
        std::cout << "---------------------------------------------------------\n";
        std::cout << "  Total Tests: " << ut.passedCount + ut.failedCount << ", Passed: " << ut.passedCount << ", Failed:  " << ut.failedCount << std::endl;
}
