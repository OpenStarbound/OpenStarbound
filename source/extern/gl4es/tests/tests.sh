#!/bin/bash

function clean_tests {
    #glxgears
    if [ -e glxgears.0000008203.png ];then
        rm glxgears.0000008203.png
    fi
    if [ -e glxgears.trace ];then
        rm glxgears.trace
    fi
    #stunt car racer
    if [ -e stuntcarracer.0000118817.png ];then
        rm stuntcarracer.0000118817.png
    fi
    if [ -e stuntcarracer.trace ];then
        rm stuntcarracer.trace
    fi
    #neverball
    if [ -e neverball.0000078750.png ];then
        rm neverball.0000078750.png
    fi
    if [ -e neverball.trace ];then
        rm neverball.trace
    fi
    #foobillardplus
    if [ -e foobillardplus.0000014748.png ];then
        rm foobillardplus.0000014748.png
    fi
    if [ -e foobillardplus.trace ];then
        rm foobillardplus.trace
    fi
    #openra
    if [ -e openra.0000031249.png ];then
        rm openra.0000031249.png
    fi
    if [ -e openra.trace ];then
        rm openra.trace
    fi
    #glsl_lighting
    if [ -e glsl_lighting.0000505393.png ];then
        rm glsl_lighting.0000505393.png
    fi
    if [ -e glsl_lighting.trace ];then
        rm glsl_lighting.trace
    fi
    #pointsprite
    if [ -e pointsprite.0000248810.png ];then
        rm pointsprite.0000248810.png
    fi
    if [ -e pointsprite.trace ];then
        rm pointsprite.trace
    fi
    #diff result
    if [ -e diff.png ];then
        rm diff.png
    fi
}

export OK=1
function launch_test {
    tar xf ../traces/$1.tgz
    if [ "$BENCH" = "1" ];then
        glretrace -b $1.trace    
    else
        apitrace dump-images --calls="$2" $1.trace
        EXTRACT=""
        if [ ! -z "$4" ];then
        EXTRACT="-extract $4"
        fi
        result=$(compare -metric AE -fuzz 20% $EXTRACT ../refs/$1.$2.png $1.$2.png diff.png 2>&1)
        if [ ! "$result" -lt "$3" ];then
            popd >/dev/null
            echo "error, $result pixels diff"
            export OK=0
        fi
    fi
}

function banner {
    echo
    echo " ******************* "
    if [ "$BENCH" = "1" ];then
    echo "    Bench $1 "
    else
    echo "    $1 "
    fi
    echo " ------------------- "
}

export BENCH=0
if [ "$1" = "-b" ];then
 BENCH=1
 shift
fi
if [ "$2" = "-b" ];then
 BENCH=1
fi

if [ ! -z "$1" ];then
 export LD_LIBRARY_PATH=$1:$LD_LIBRARY_PATH
fi

if [ ! "$BENCH" = "1" ];then
    export LIBGL_FB=3
fi
export LIBGL_SILENTSTUB=1
export LIBGL_NOBANNER=1

TESTS=`dirname "$0"`

pushd "$TESTS" >/dev/null

clean_tests

export LIBGL_ES=1

banner "GLES1.1: glxgears"
launch_test glxgears 0000008203 25
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES1.1: StuntCarRacer"
launch_test stuntcarracer 0000118817 20 638x478+1+1
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES1.1: Neverball"
launch_test neverball 0000078750 20 798x478+1+1
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES1.1: Foobillard Plus"
launch_test foobillardplus 0000014748 20 798x478+1+1
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES1.1: Point Sprite"
launch_test pointsprite 0000248810 20
if [ $OK = 0 ];then
    exit 1
fi

export LIBGL_ES=2

banner "GLES2.0: OpenRA"
launch_test openra 0000031249 20 638x478+1+1
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES2.0: glsl_lighting"
launch_test glsl_lighting 0000505393 20
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES2.0: glxgears"
# Because FlatShaded mode is not supported, the image is more different.
launch_test glxgears 0000008203 700
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES2.0: StuntCarRacer"
launch_test stuntcarracer 0000118817 20 638x478+1+1
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES2.0: Neverball"
launch_test neverball 0000078750 200 798x478+1+1
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES2.0: Foobillard Plus"
launch_test foobillardplus 0000014748 50 798x478+1+1
if [ $OK = 0 ];then
    exit 1
fi

banner "GLES2.0: Point Sprite"
launch_test pointsprite 0000248810 20
if [ $OK = 0 ];then
    exit 1
fi

# cleanup
clean_tests

popd >/dev/null
echo " ================= "
echo "All done"
exit 0
