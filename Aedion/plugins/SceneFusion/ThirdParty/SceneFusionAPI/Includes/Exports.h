#pragma once

#ifndef KS_DLL
    #define EXTERNAL
    #define INTERNAL
#else
    #ifdef _WIN32    //Windows
        #ifdef SF_EXPORTS
            #ifdef SF_TEST
                #define INTERNAL __declspec(dllexport)
            #else
                #define INTERNAL
            #endif
    
            #define EXTERNAL __declspec(dllexport)
        #else
            #ifdef SF_TEST
                #define INTERNAL __declspec(dllimport)
            #else
                #define INTERNAL
            #endif
    
            #define EXTERNAL __declspec(dllimport)
        #endif 
    #else       //Non windows
        #ifdef SF_EXPORTS
            #ifdef SF_TEST
                #define INTERNAL
            #else
                #define INTERNAL __attribute__((__visibility__("hidden")))
            #endif
        #else
            #define INTERNAL
        #endif 

        #define EXTERNAL
    #endif
#endif