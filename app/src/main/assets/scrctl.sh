pm grant com.jingrong.inputredirectionclient_android android.permission.WRITE_SECURE_SETTINGS

file_name="scrctl.dex"
origin_path="$(cd "$(dirname "$0")" && pwd)/$file_name"
cache_dir="/data/local/tmp"
target_path="$cache_dir/$file_name"

if [[ -e $origin_path ]]; then
    cp -rf "$origin_path" $target_path
    export CLASSPATH="$target_path"
    nohup app_process /system/bin com.jingrong.scrctl.ScrCtl > /dev/null 2>&1 &
    echo "ScrCtl started"
else
    echo "ERROR: $origin_path not found"
fi
