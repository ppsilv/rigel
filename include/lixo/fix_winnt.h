 #ifndef FIX_WINT_H
 #define FIX_WINT_H

 /* Garante que wint_t e wchar_t estejam visíveis para a newlib */
 #ifndef _WINT_T
 #define _WINT_T
 typedef unsigned int wint_t;
 #endif

 #ifndef _WCHAR_T
 #define _WCHAR_T
 typedef unsigned int wchar_t;
 #endif

 #endif
