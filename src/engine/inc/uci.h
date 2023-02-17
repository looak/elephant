#pragma once

// Define the function to be exported
__declspec(dllexport) double my_function(int arg1, double arg2) {
    // Function implementation goes here
    return arg1 + arg2;
}