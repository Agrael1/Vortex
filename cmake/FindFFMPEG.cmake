# ==================================================================================================================== #
# Prepare                                                                                                              #
# - include dir: $HOME/ffmpeg_build/include                                                                            #
# - library dir: $HOME/ffmpeg_build/lib                                                                                #
# - binary  dir: $HOME/ffmpeg_build/bin                                                                                #
# ==================================================================================================================== #

# ==================================================================================================================== #
# Linux FFmpeg installation:
# ```
# # Install dependencies with apt:
# # sudo apt install nasm libass-dev libfdk-aac-dev libmp3lame-dev libopus-dev libvorbis-dev libvpx-dev libx264-dev libx265-dev;
# 
# Install dependiencies with pacman:
# # sudo pacman -S nasm libass libfdk-aac lame opus libvorbis libvpx libx264 x265;
# 
# cd ffmpeg &&\
# PATH="$HOME/ffmpeg_build/bin:$PATH" \
# PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig" \
# ./configure \
#   --prefix="$HOME/ffmpeg_build" \
#   --extra-cflags="-I$HOME/ffmpeg_build/include" \
#   --extra-ldflags="-L$HOME/ffmpeg_build/lib" \
#   --extra-libs="-lpthread -lm" \
#   --bindir="$HOME/ffmpeg_build/bin" \
#   --disable-static \
#   --enable-shared \
#   --enable-gpl \
#   --enable-libass \
#   --enable-libfdk-aac \
#   --enable-libfreetype \
#   --enable-libmp3lame \
#   --enable-libopus \
#   --enable-libvorbis \
#   --enable-libvpx \
#   --enable-libx264 \
#   --enable-libx265 \
#   --enable-nonfree && \
# PATH="$HOME/ffmpeg_build/bin:$PATH" make -j 8 && \
# make install
# ```
# ==================================================================================================================== #

# ==================================================================================================================== #
# MacOS X FFmpeg installation:
# ```
# # please install brew first !
# brew install automake fdk-aac git lame libass libtool libvorbis libvpx \
#              opus sdl shtool texi2html theora wget x264 x265 xvid nasm &&
# cd ffmpeg &&\
# PATH="$HOME/ffmpeg_build/bin:$PATH" \
# PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig" \
# pkg_config='pkg-config --static' \
# ./configure \
#   --prefix="$HOME/ffmpeg_build" \
#   --pkg-config-flags="--static" \
#   --extra-cflags="-I$HOME/ffmpeg_build/include" \
#   --extra-ldflags="-L$HOME/ffmpeg_build/lib" \
#   --extra-libs="-lpthread -lm" \
#   --bindir="$HOME/ffmpeg_build/bin" \
#   --cc=clang --host-cflags= --host-ldflags= \
#   --enable-static --enable-shared --enable-pthreads \
#   --enable-hardcoded-tables --enable-avresample \
#   --enable-gpl  --enable-libmp3lame --enable-libx264 --enable-libxvid --enable-opencl \
#   --enable-videotoolbox \
#   --disable-lzma &&
# PATH="$HOME/ffmpeg_build/bin:$PATH" make -j 8 && \
# make install 
# ```
# ==================================================================================================================== #

# ==================================================================================================================== #
# Windows FFmpeg installation:
# ```
# # first open a cmd (not powershell or something else !)
# # install VS2015 (VS2013 and later should be ok, but paths should be changed according to VS version)
# # install msys2 (64bit) at c:/
# # cmd -> cd c:/msys64
# # cmd -> "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/amd64/vcvars64.bat"
# # cmd -> ./msys2_shell.cmd -mingw64
# # then, msys will open, all following cmds are typed in msys shell.
#
# export PATH="/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/BIN/amd64/":$PATH &&
# which link.exe && # should be `/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/BIN/amd64/link.exe`
# which cl.exe &&   # should be `/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/BIN/amd64/cl.exe`
# # if link.exe is not right, rename /usr/bin/link.exe to /usr/bin/link_backup.exe
#
# cd ffmpeg &&
# PATH="$HOME/ffmpeg_build/bin:$PATH" PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig" ./configure \
#   --toolchain=msvc \
#   --prefix="$HOME/ffmpeg_build" \
#   --pkg-config-flags="--static" \
#   --extra-cflags="-I$HOME/ffmpeg_build/include" \
#   --extra-ldflags="-L$HOME/ffmpeg_build/lib" \
#   --bindir="$HOME/ffmpeg_build/bin" \
#   --arch=x86_64 --enable-yasm --enable-asm --enable-shared --enable-static && \
# PATH="$HOME/ffmpeg_build/bin:$PATH" make -j 8 && \
# make install
# # the ffmpeg_build should be in the msys home.
# ```
# ==================================================================================================================== #


# ==================================================================================================================== #
# Code of FindFFmpeg.cmake                                                                                             #
# ==================================================================================================================== #

# automaticly get installation path
if (NOT FFmpeg_INSTALL_PATH)
    if (UNIX)   # macos or Linux using Home
        set(FFmpeg_INSTALL_PATH "$ENV{HOME}/ffmpeg_build")
    else()      # windows using msys2_shell -mingw64
        set(FFmpeg_INSTALL_PATH "C:/msys64/home/admin/ffmpeg_build")
    endif()
endif ()

# Find all components if not specificly asked
if (NOT FFmpeg_FIND_COMPONENTS)
    set(FFmpeg_FIND_COMPONENTS AVCODEC AVFORMAT AVDEVICE AVUTIL AVFILTER SWSCALE SWRESAMPLE)
endif ()

# Function, to find component
function(_FFmpeg_find_component _component _library _header)
    # find from path
    find_path(FFmpeg_${_component}_INCLUDE_DIR
        NAMES ${_header}
        PATHS "${FFmpeg_INSTALL_PATH}/include"
        NO_DEFAULT_PATH
        NO_CMAKE_PATH
    )
    find_library(FFmpeg_${_component}_LIBRARY
        NAMES ${_library}
        PATHS "${FFmpeg_INSTALL_PATH}/lib"
        NO_DEFAULT_PATH
        NO_CMAKE_PATH
    )

    # if already found
    if(TARGET FFmpeg::${_component})
        return()
    endif()

    # Found
    if (FFmpeg_${_component}_INCLUDE_DIR AND FFmpeg_${_component}_LIBRARY)
        mark_as_advanced(FFmpeg_${_component}_INCLUDE_DIR FFmpeg_${_component}_LIBRARY)
        add_library(FFmpeg::${_component} UNKNOWN IMPORTED)
        set_target_properties(FFmpeg::${_component} PROPERTIES
            IMPORTED_LOCATION             "${FFmpeg_${_component}_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_${_component}_INCLUDE_DIR}"
        )
        if (NOT FFmpeg_FIND_QUIETLY)
            message(STATUS "Found FFmpeg::${_component}: ${FFmpeg_${_component}_INCLUDE_DIR} ${FFmpeg_${_component}_LIBRARY}")
        endif ()
    # Not Found
    else ()
        if (FFmpeg_FIND_REQUIRED)
            message(SEND_ERROR "Can't find ${_component} in FFmpeg. Check that it's installed correctly and try again.")
        endif ()
    endif ()
endfunction()

# # TBALE of argments
# AVCODEC    avcodec    libavcodec/avcodec.h
# AVFORMAT   avformat   libavformat/avformat.h
# AVDEVICE   avdevice   libavdevice/avdevice.h
# AVUTIL     avutil     libavutil/avutil.h
# AVFILTER   avfilter   libavfilter/avfilter.h
# SWSCALE    swscale    libswscale/swscale.h
# SWRESAMPLE swresample libswresample/swresample.h
# POSTPROC   postproc   libpostproc/postprocess.h  (special case!)

# loop each component
set(FFmpeg_INTERFACE_LINK_LIBRARY "")
foreach (_component ${FFmpeg_FIND_COMPONENTS})
    string(TOLOWER ${_component} _tmp_lower)
    set(_tmp_header lib${_tmp_lower}/${_tmp_lower}.h)
    # find
    _FFmpeg_find_component(${_component} ${_tmp_lower} ${_tmp_header})
    list(APPEND FFmpeg_INTERFACE_LINK_LIBRARIES FFmpeg::${_component})
endforeach ()
# Interface FFmpeg::FFmpeg
if (NOT TARGET FFmpeg::FFmpeg)
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
    set_property(TARGET FFmpeg::FFmpeg PROPERTY
        INTERFACE_LINK_LIBRARIES ${FFmpeg_INTERFACE_LINK_LIBRARIES}
    )
endif ()


# Discover FFmpeg runtime DLLs for Windows builds so we can stage them next to
# the consuming targets and package them via `cmake --install`.
if (WIN32)
    set(_ffmpeg_bin_dir "${FFmpeg_INSTALL_PATH}/bin")
    if (EXISTS "${_ffmpeg_bin_dir}")
        file(GLOB FFMPEG_WIN32_DLLS "${_ffmpeg_bin_dir}/*.dll")
        list(LENGTH FFMPEG_WIN32_DLLS _ffmpeg_dll_count)
        if (_ffmpeg_dll_count EQUAL 0)
            message(WARNING
                "FFmpeg bin directory (${_ffmpeg_bin_dir}) does not contain any DLLs."
            )
        endif()
    else()
        set(FFMPEG_WIN32_DLLS "")
        message(WARNING
            "FFmpeg bin directory not found: ${_ffmpeg_bin_dir}. Set FFmpeg_INSTALL_PATH to a valid build."
        )
    endif()
    set(FFMPEG_DLLS_INSTALL_RULE_DEFINED FALSE)
endif ()

macro(FFMPEG_COPY_DLL projectName)
    if (WIN32)
        if (FFMPEG_WIN32_DLLS)
            add_custom_command(TARGET ${projectName} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${FFMPEG_WIN32_DLLS}
                        $<TARGET_FILE_DIR:${projectName}>
                COMMAND_EXPAND_LISTS
                COMMENT "Copying FFmpeg runtime DLLs for ${projectName}"
            )
            if (NOT FFMPEG_DLLS_INSTALL_RULE_DEFINED)
                install(FILES ${FFMPEG_WIN32_DLLS} DESTINATION bin)
                set(FFMPEG_DLLS_INSTALL_RULE_DEFINED TRUE)
            endif()
        else()
            message(WARNING
                "Skipping FFmpeg DLL copy for target ${projectName}; DLL list is empty."
            )
        endif()
    endif ()
endmacro()