#!/bin/sh
java_home=$JAVA_HOME
androidSDK=$ANDROID_HOME
build_tools_version=34.0.0
tmp_dir=/dex

rm -rf ./app/src/main/assets/ScrCtl.dex
mkdir -p ../build/$tmp_dir/output ../build/$tmp_dir/input
cd ../build/$tmp_dir
cp ../../app/src/main/java/com/jingrong/scrctl/ScrCtl.java ./

$java_home/bin/javac -d ./input \
    -source 8 \
    -target 8 \
    -bootclasspath "$ANDROID_HOME/platforms/android-34/android.jar" \
    ./ScrCtl.java

$java_home/bin/jar cf ScrCtl.jar -C ./input .

$androidSDK/build-tools/$build_tools_version/d8 \
    --min-api 24 \
    --release \
    --output ./output \
    ScrCtl.jar

cp ./output/classes.dex ../../app/src/main/assets/ScrCtl.dex
cd - && rm -rf ../build/$tmp_dir
