#pragma once

#ifdef ELEPHANT_ENGINE_EXPORTS
#define ELEPHANT_API __declspec(dllexport)
#else  
#define ELEPHANT_API
#endif