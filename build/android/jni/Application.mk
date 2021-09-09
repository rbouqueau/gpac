APP_ABI := armeabi-v7a x86
APP_STL := c++_static
APP_CPPFLAGS := -frtti -fexceptions -Wno-parentheses -Wno-shift-negative-value -Wno-pointer-sign
APP_CFLAGS := -Wno-parentheses -Wno-shift-negative-value -Wno-pointer-sign
LOCAL_CFLAGS := -O0 -g
APP_OPTIM := debug


APP_MODULES := gpac gm_ft_font libjavaenv gm_droid_out gm_droid_audio gm_droid_mpegv gpacWrapper mp4box

#old modules to reintegrate or drop
#APP_MODULES := gm_osd gm_droid_cam gm_widgetman
