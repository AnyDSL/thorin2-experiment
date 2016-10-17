#pragma once

#include <string>
#include <iostream>

void prepareUtf8Console();
void printUtf8(const std::string& input);

#ifdef WIN32
#define cout std::wcout
#else
#define cout std::cout
#endif
