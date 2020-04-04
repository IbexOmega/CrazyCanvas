#pragma once

#if defined(LAMBDA_DEBUG) && defined(LAMBDA_VISUAL_STUDIO)
    #include <stdlib.h>
    #include <crtdbg.h>

    #define DBG_NEW                 new (_NORMAL_BLOCK , __FILE__ ,__LINE__)
    #define SET_DEBUG_FLAGS(...)    _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF)
#else
    #define DBG_NEW                 new
    #define SET_DEBUG_FLAGS(...)    (void)0
#endif