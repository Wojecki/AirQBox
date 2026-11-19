#pragma once

#define GxEPD2_420_GDEY042T81_IS_BW true

#if defined(GxEPD2_DISPLAY_CLASS) && defined(GxEPD2_DRIVER_CLASS)
#define IS_GxEPD2_DRIVER(c, x) (c##x)
#define IS_GxEPD2_DRIVER_BW(x) IS_GxEPD2_DRIVER(x, _IS_BW)

#if !IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS) || !IS_GxEPD2_DRIVER_BW(GxEPD2_DRIVER_CLASS)
#error "Invalid display or driver class configuration"
#endif
#endif