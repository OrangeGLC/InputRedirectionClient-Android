#!/bin/sh

LOG_PREFIX="[ScrCtl]"
log() { echo "$LOG_PREFIX $1"; }

file_name="scrctl.dex"
origin_path="$(cd "$(dirname "$0")" && pwd)/$file_name"
cache_dir="/data/local/tmp"
target_path="$cache_dir/$file_name"
log_path="$cache_dir/scrctl_startup.log"

log "=== ScrCtl service startup ==="

# Step 1: Grant permission
log "Granting WRITE_SECURE_SETTINGS permission..."
pm grant com.jingrong.inputredirectionclient_android android.permission.WRITE_SECURE_SETTINGS 2>/dev/null
log "Permission OK."

# Step 2: Copy dex to cache
log "Copying $file_name to $target_path..."
if [ -e "$origin_path" ]; then
    cp -f "$origin_path" "$target_path"
    log "Copy done."
else
    log "ERROR: $origin_path not found."
    exit 1
fi

# Step 3: Launch app_process
log "Launching app_process..."
export CLASSPATH="$target_path"
> "$log_path"
nohup app_process /system/bin com.jingrong.scrctl.ScrCtl >> "$log_path" 2>&1 &

# Wait for service to initialize (reflection + bind socket)
sleep 2

# Check result
if grep -qE "ScrCtl started|already running" "$log_path" 2>/dev/null; then
    log "SUCCESS: ScrCtl service is running."
else
    log "FAILED: ScrCtl service startup failed."
    log "--- Java crash log ($log_path) ---"
    cat "$log_path"
    log "--- End of log ---"
    exit 1
fi
