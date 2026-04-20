#!/bin/bash
# https://lisyarus.github.io/blog/posts/porting-for-android.html#section-assembling

# For future support of more architectures
if [ -z $1 ] || [ -z $2 ]; then
  echo 'build.sh: Missing argument'; exit 1
fi

ARCH_ABI="$1"
SDL_JAR="$2"
echo $SDL_JAR

for i in "jar" "aapt" "javac" "d8" "zipalign" "apksigner"; do
  command -v $i >/dev/null 2>&1 
  if [ $? -ne 0 ]; then echo 'build.sh: $PATH is missing Android build-tools'; exit 1; fi
done

mkdir -p bin/ lib/$ARCH_ABI
rm -rf src/org/ && jar -xf $SDL_JAR org && mv org src/
aapt package -f -m -J ./src/ -M AndroidManifest.xml -I $PLATFORMS/android.jar -S res/
javac -d obj -classpath src:$PLATFORMS/android.jar src/ru/lotigara/opensb/* src/org/libsdl/app/*
d8 --output . obj/ru/lotigara/opensb/* obj/org/libsdl/app/*
aapt package -f -m -F ./bin/opensb.unaligned.apk -M AndroidManifest.xml -I $PLATFORMS/android.jar -S res/
aapt add ./bin/opensb.unaligned.apk classes.dex 
aapt add ./bin/opensb.unaligned.apk lib/$ARCH_ABI/*
zipalign -f 4 ./bin/opensb.unaligned.apk ./bin/opensb.apk 
apksigner sign --ks opensb.keystore --ks-pass=pass:opensb ./bin/opensb.apk 
