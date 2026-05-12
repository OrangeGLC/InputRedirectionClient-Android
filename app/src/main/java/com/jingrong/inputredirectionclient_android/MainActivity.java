package com.jingrong.inputredirectionclient_android;

import android.Manifest;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.text.Html;
import java.util.Locale;
import android.text.method.LinkMovementMethod;
import android.text.InputType;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Switch;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;

import com.google.androidgamesdk.GameActivity;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import org.json.JSONObject;

public class MainActivity extends GameActivity {
    static {
        System.loadLibrary("inputredirectionclient_android");
    }
    public native void initNative(String path);
    public native void cleanNative();
    public native String saveIPAddress(String input);
    public native String getCfgIP();
    public native void setInvertAB(boolean flg);
    public native boolean getInvertAB();
    public native void setInvertXY(boolean flg);
    public native boolean getInvertXY();
    public native void setTurbo(int index, boolean flg);
    public native boolean getTurbo(int index);
    public native void setHomeMapEnable(boolean flg);
    public native boolean getHomeMapEnable();
    public native void setPowerMapEnable(boolean flg);
    public native boolean getPowerMapEnable();
    public native void setPowerOffMapEnable(boolean flg);
    public native boolean getPowerOffMapEnable();
    public native void setTurboInterval(int ms);
    public native int getTurboInterval();
    public native void setTurboMode(int index, boolean fullAuto);
    public native boolean getTurboMode(int index);
    public native void setKeyMapMode(int mode);
    public native int getKeyMapMode();
    public native void setSwapJoysticks(boolean flg);
    public native boolean getSwapJoysticks();
    public native void setKeyMapping(int inputIdx, int targetIdx);
    public native int getKeyMapping(int inputIdx);
    public native void enterKeyCapture(int n3dsKeyIndex);
    public native void exitKeyCapture();
    public native void resolveKeyConflict(boolean accept);
    public native String getInputKeyName(int inputIdx);
    public void updateUI()
    {
        runOnUiThread(()-> {
            etIP.setText(getCfgIP());
            etIP.setEnabled(false);
            btSaveIP.setText(R.string.bt_txt_edit);
            swInvertAB.setChecked(getInvertAB());
            swInvertXY.setChecked(getInvertXY());
            updateTurboButtons();
            swHomeMap.setChecked(getHomeMapEnable());
            swPowerMap.setChecked(getPowerMapEnable());
            swPowerOffMap.setChecked(getPowerOffMapEnable());
            int interval = getTurboInterval();
            sbTurboInterval.setProgress(interval);
            etTurboInterval.setText(String.valueOf(interval));
            etHome.setEnabled(getHomeMapEnable());
            etPower.setEnabled(getPowerMapEnable());
            etPowerOff.setEnabled(getPowerOffMapEnable());
            updateKeyMappingUI();
        });
    }

    // Called from C++ after key capture
    public void onCaptureResult(String n3dsName, String physName,
                                boolean conflict, String conflictN3dsName) {
        runOnUiThread(() -> {
            if (!conflict) {
                // Success: mapping applied, just refresh
                mCapturingN3dsIdx = -1;
                updateKeyMappingUI();
            } else {
                // Conflict: show dialog
                showConflictDialog(n3dsName, physName, conflictN3dsName);
            }
        });
    }

    private void showConflictDialog(String n3dsName, String physName, String conflictN3dsName) {
        new AlertDialog.Builder(this)
            .setTitle(R.string.conflict_title)
            .setMessage(getString(R.string.conflict_msg, physName, conflictN3dsName))
            .setNegativeButton(R.string.conflict_cancel, (d, w) -> {
                resolveKeyConflict(false);
                mCapturingN3dsIdx = -1;
                updateKeyMappingUI();
            })
            .setPositiveButton(R.string.conflict_continue, (d, w) -> {
                resolveKeyConflict(true);
                mCapturingN3dsIdx = -1;
                updateKeyMappingUI();
            })
            .setCancelable(false)
            .show();
    }

    private void switchKeyMapMode(int mode) {
        mUpdatingKeyMapUI = true;
        setKeyMapMode(mode);
        updateKeyMappingUI();
        mUpdatingKeyMapUI = false;
    }

    private void updateKeyMappingUI() {
        mUpdatingKeyMapUI = true;

        int mode = getKeyMapMode();
        // Mode tabs: TabLayout handles visual state automatically
        com.google.android.material.tabs.TabLayout.Tab tab = tabMode.getTabAt(mode);
        if (tab != null) tab.select();

        // Simple mode layout
        layoutSimpleMode.setVisibility(mode == KEYMAP_MODE_SIMPLE ? View.VISIBLE : View.GONE);

        // Custom mode layout
        layoutCustomMode.setVisibility(mode == KEYMAP_MODE_CUSTOM ? View.VISIBLE : View.GONE);

        // Swap joysticks (sync both switches, show correct one per mode)
        boolean swap = getSwapJoysticks();
        swSwapSticks.setChecked(swap);
        swSwapSticks.setVisibility(mode == KEYMAP_MODE_CUSTOM ? View.VISIBLE : View.GONE);
        swSwapSticksSimple.setChecked(swap);

        // Update custom mode key mapping entries
        if (mode == KEYMAP_MODE_CUSTOM) {
            for (int n3dsIdx = 0; n3dsIdx < mKeyMapEdits.length; n3dsIdx++) {
                if (mKeyMapEdits[n3dsIdx] == null) continue;
                // Find which physical key maps to this N3DS key
                String physName = N3DS_KEY_NAMES[n3dsIdx]; // default
                for (int physIdx = 0; physIdx < MAX_INPUT_KEY_INDEX; physIdx++) {
                    if (getKeyMapping(physIdx) == n3dsIdx) {
                        physName = getInputKeyName(physIdx);
                        break;
                    }
                }
                if (mCapturingN3dsIdx == n3dsIdx) {
                    mKeyMapEdits[n3dsIdx].setText("");
                } else {
                    mKeyMapEdits[n3dsIdx].setText(physName);
                }
            }
        }

        // Simple mode switches
        swInvertAB.setChecked(getInvertAB());
        swInvertXY.setChecked(getInvertXY());

        mUpdatingKeyMapUI = false;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String btPerm = Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
                ? Manifest.permission.BLUETOOTH_CONNECT : Manifest.permission.BLUETOOTH;
        if (checkSelfPermission(btPerm) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{btPerm}, 0);
        }

        WifiManager wm = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wm != null) {
            wifiLock = wm.createWifiLock(
                    Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
                            ? WifiManager.WIFI_MODE_FULL : 0x3 /* WIFI_MODE_FULL_LOCK */,
                    "IRC:WifiLock");
            wifiLock.acquire();
        }
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (pm != null) {
            wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "IRC:WakeLock");
            wakeLock.acquire();
        }

        //Read IP from config file
        File inPath = getFilesDir();
        initNative(inPath.getAbsolutePath());

        // 允许内容延伸到系统栏下方，并手动处理底部insets
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);

        // 屏幕常亮
        getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_main);

        etIP = findViewById(R.id.et_ip);
        btSaveIP = findViewById(R.id.bt_save);
        btDisableTurbo = findViewById(R.id.bt_disableturbo);
        sbTurboInterval = findViewById(R.id.sb_turbo_interval);
        scrollMain = findViewById(R.id.scroll_main);
        ViewCompat.setOnApplyWindowInsetsListener(scrollMain, (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPaddingRelative(v.getPaddingStart(), v.getPaddingTop(), v.getPaddingEnd(), systemBars.bottom);
            return insets;
        });
        etTurboInterval = findViewById(R.id.et_turbo_interval);
        btOffScr = findViewById(R.id.bt_offScr);
        swInvertAB = findViewById(R.id.switch_invertAB);
        swInvertXY = findViewById(R.id.switch_invertXY);
        swTurboA = findViewById(R.id.switch_turbo_A);
        swTurboB = findViewById(R.id.switch_turbo_B);
        swTurboX = findViewById(R.id.switch_turbo_X);
        swTurboY = findViewById(R.id.switch_turbo_Y);
        swTurboL = findViewById(R.id.switch_turbo_L);
        swTurboR = findViewById(R.id.switch_turbo_R);
        swTurboZL = findViewById(R.id.switch_turbo_ZL);
        swTurboZR = findViewById(R.id.switch_turbo_ZR);
        cbTurboModeA = findViewById(R.id.cb_turbo_mode_A);
        cbTurboModeB = findViewById(R.id.cb_turbo_mode_B);
        cbTurboModeX = findViewById(R.id.cb_turbo_mode_X);
        cbTurboModeY = findViewById(R.id.cb_turbo_mode_Y);
        cbTurboModeL = findViewById(R.id.cb_turbo_mode_L);
        cbTurboModeR = findViewById(R.id.cb_turbo_mode_R);
        cbTurboModeZL = findViewById(R.id.cb_turbo_mode_ZL);
        cbTurboModeZR = findViewById(R.id.cb_turbo_mode_ZR);
        swHomeMap = findViewById(R.id.switch_home);
        swPowerMap = findViewById(R.id.switch_power);
        swPowerOffMap = findViewById(R.id.switch_shut);
        etHome = findViewById(R.id.et_home);
        etPower = findViewById(R.id.et_power);
        etPowerOff = findViewById(R.id.et_shut);

        // Key mapping views
        tabMode = findViewById(R.id.tab_mode);
        layoutSimpleMode = findViewById(R.id.layout_simple_mode);
        layoutCustomMode = findViewById(R.id.layout_custom_mode);
        swSwapSticks = findViewById(R.id.switch_swap_sticks);
        swSwapSticksSimple = findViewById(R.id.switch_swap_sticks_simple);

        mKeyMapEdits[N3DS_KEY_INDEX_A] = findViewById(R.id.et_map_A);
        mKeyMapEdits[N3DS_KEY_INDEX_B] = findViewById(R.id.et_map_B);
        mKeyMapEdits[N3DS_KEY_INDEX_X] = findViewById(R.id.et_map_X);
        mKeyMapEdits[N3DS_KEY_INDEX_Y] = findViewById(R.id.et_map_Y);
        mKeyMapEdits[N3DS_KEY_INDEX_L] = findViewById(R.id.et_map_L);
        mKeyMapEdits[N3DS_KEY_INDEX_R] = findViewById(R.id.et_map_R);
        mKeyMapEdits[N3DS_KEY_INDEX_ZL] = findViewById(R.id.et_map_ZL);
        mKeyMapEdits[N3DS_KEY_INDEX_ZR] = findViewById(R.id.et_map_ZR);
        mKeyMapEdits[N3DS_KEY_INDEX_SELECT] = findViewById(R.id.et_map_SELECT);
        mKeyMapEdits[N3DS_KEY_INDEX_START] = findViewById(R.id.et_map_START);
        // HOME/POWER are handled in the special buttons section, not in key mapping grid
        mKeyMapEdits[N3DS_KEY_INDEX_UP] = findViewById(R.id.et_map_UP);
        mKeyMapEdits[N3DS_KEY_INDEX_DOWN] = findViewById(R.id.et_map_DOWN);
        mKeyMapEdits[N3DS_KEY_INDEX_LEFT] = findViewById(R.id.et_map_LEFT);
        mKeyMapEdits[N3DS_KEY_INDEX_RIGHT] = findViewById(R.id.et_map_RIGHT);
        // N3DS_KEY_INDEX_SHUTDOWN has no entry

        disableFocus(findViewById(android.R.id.content));

        btSaveIP.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String txtSave = getString(R.string.bt_txt_save);
                if(btSaveIP.getText().toString().equals(txtSave)){
                    String input = etIP.getText().toString();
                    if (!isValidIP(input)) {
                        showErrorDialog();
                        return;
                    }
                    hideKeyboard(MainActivity.this);
                    etIP.setEnabled(false);
                    //Save to file by ndk API
                    Toast.makeText(MainActivity.this, saveIPAddress(input), Toast.LENGTH_SHORT).show();
                    btSaveIP.setText(R.string.bt_txt_edit);
                }else {
                    etIP.setEnabled(true);
                    btSaveIP.setText(R.string.bt_txt_save);
                }

            }
        });
        btDisableTurbo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                for (int i = 0; i < MAX_N3DS_KEY_TURBO_INDEX; i++) {
                    setTurbo(i, false);
                }
                updateUI();
            }
        });

        sbTurboInterval.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) etTurboInterval.setText(String.valueOf(progress));
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {
                setTurboInterval(seekBar.getProgress());
            }
        });

        etTurboInterval.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                applyTurboIntervalInput();
                return true;
            }
            return false;
        });
        etTurboInterval.setOnFocusChangeListener((v, hasFocus) -> {
            if (hasFocus) {
                scrollMain.postDelayed(() -> scrollMain.smoothScrollTo(0, etTurboInterval.getBottom()), 200);
            } else {
                applyTurboIntervalInput();
            }
        });

        btOffScr.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(() -> sendScrCmd(true)).start();
            }
        });

        swInvertAB.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b) {
                setInvertAB(b);
            }
        });

        swInvertXY.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b) {
                setInvertXY(b);
            }
        });

        setupTurboButtons();

        swHomeMap.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b) {
                setHomeMapEnable(b);
                etHome.setEnabled(b);
            }
        });

        swPowerMap.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b) {
                setPowerMapEnable(b);
                etPower.setEnabled(b);
            }
        });

        swPowerOffMap.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b) {
                setPowerOffMapEnable(b);
                etPowerOff.setEnabled(b);
            }
        });

        // Key mapping mode toggle
        tabMode.addOnTabSelectedListener(new com.google.android.material.tabs.TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(com.google.android.material.tabs.TabLayout.Tab tab) {
                if (!mUpdatingKeyMapUI) switchKeyMapMode(tab.getPosition());
            }
            @Override public void onTabUnselected(com.google.android.material.tabs.TabLayout.Tab tab) {}
            @Override public void onTabReselected(com.google.android.material.tabs.TabLayout.Tab tab) {}
        });

        // Swap joysticks (both simple and custom mode)
        CompoundButton.OnCheckedChangeListener swapListener = (btn, on) -> {
            if (mUpdatingKeyMapUI) return;
            setSwapJoysticks(on);
        };
        swSwapSticks.setOnCheckedChangeListener(swapListener);
        swSwapSticksSimple.setOnCheckedChangeListener(swapListener);

        // Key mapping entry click listeners (on EditText)
        for (int i = 0; i < mKeyMapEdits.length; i++) {
            if (mKeyMapEdits[i] == null) continue;
            final int n3dsIdx = i;
            mKeyMapEdits[i].setOnClickListener(v -> {
                if (mCapturingN3dsIdx == n3dsIdx) {
                    // Cancel capture
                    mCapturingN3dsIdx = -1;
                    exitKeyCapture();
                    updateKeyMappingUI();
                } else {
                    // Start capture for this N3DS key
                    if (mCapturingN3dsIdx >= 0) {
                        exitKeyCapture();
                    }
                    mCapturingN3dsIdx = n3dsIdx;
                    enterKeyCapture(n3dsIdx);
                    updateKeyMappingUI();
                }
            });
        }

        etHome.setInputType(InputType.TYPE_NULL);
        etHome.setFocusable(false);
        etHome.setClickable(false);

        etPower.setInputType(InputType.TYPE_NULL);
        etPower.setFocusable(false);
        etPower.setClickable(false);


        etPowerOff.setInputType(InputType.TYPE_NULL);
        etPowerOff.setFocusable(false);
        etPowerOff.setClickable(false);

        unzipFiles();
        checkUpdate();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.action_about) {
            showAbout();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void showAbout() {
        String aboutMsg = getString(R.string.about_msg)
                + "<br><br>" + getString(R.string.about_version, BuildConfig.VERSION_NAME);
        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.about_title))
                .setMessage(Html.fromHtml(aboutMsg))
                .setPositiveButton(android.R.string.ok, null)
                .show();
        TextView tv = dlg.findViewById(android.R.id.message);
        if (tv != null) tv.setMovementMethod(LinkMovementMethod.getInstance());
    }

    private void unzipFiles() {
        File dir = getExternalFilesDir(null);
        if (dir == null) return;
        String sh = dir.getPath() + "/scrctl.sh";
        String dexPath = dir.getPath() + "/scrctl.dex";
        try {
            InputStream is = getAssets().open("scrctl.sh");
            FileOutputStream fos = new FileOutputStream(sh);
            byte[] buffer = new byte[1024];
            int len;
            while ((len = is.read(buffer)) > 0) fos.write(buffer, 0, len);
            is.close();
            fos.flush();
            fos.close();
        } catch (IOException ignored) {}
        try {
            InputStream is = getAssets().open("ScrCtl.dex");
            FileOutputStream fos = new FileOutputStream(dexPath);
            byte[] buffer = new byte[1024];
            int len;
            while ((len = is.read(buffer)) > 0) fos.write(buffer, 0, len);
            is.close();
            fos.flush();
            fos.close();
        } catch (IOException ignored) {}
    }

    @Override
    protected void onDestroy()
    {
        if (wifiLock != null && wifiLock.isHeld()) wifiLock.release();
        if (wakeLock != null && wakeLock.isHeld()) wakeLock.release();
        super.onDestroy();
    }

    public static boolean isValidIP(String ip) {
        try {
            String[] parts = ip.split("\\.");
            if (parts.length != 4) return false;

            for (String segment : parts) {
                int value = Integer.parseInt(segment);
                if (value < 0 || value > 255) return false;
            }
            return true;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    private void showErrorDialog() {
        new AlertDialog.Builder(this)
                .setTitle("Error: Invalid IP Address.")
                .setMessage("A valid IP address consists of numbers less than 255 separated by dots, e.g. 192.168.10.20")
                .setPositiveButton("Close", null)
                .show();
    }

    private void disableFocus(View v) {
        if (v instanceof ViewGroup) {
            ViewGroup g = (ViewGroup) v;
            for (int i = 0; i < g.getChildCount(); i++)
                disableFocus(g.getChildAt(i));
        }
        if (v != null && v != etIP)
            v.setFocusable(false);
    }

    public void hideKeyboard(Activity activity) {
        View view = activity.getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) {
                imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
            }
        }
    }

    // --- Screen-off via ScrCtl UDP service ---

    private boolean scrOff;

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (scrOff && (keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
                keyCode == KeyEvent.KEYCODE_VOLUME_DOWN)) {
            new Thread(() -> {
                if (trySendScrCmd(false)) {
                    runOnUiThread(() -> scrOff = false);
                }
            }).start();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private static final int SCR_PORT = 64531;
    private static final InetAddress SCR_HOST = InetAddress.getLoopbackAddress();

    private static final String SCRCTL_CMD =
            "sh /sdcard/Android/data/com.jingrong.inputredirectionclient_android/files/scrctl.sh";

    private void sendScrCmd(boolean off) {
        if (trySendScrCmd(off)) {
            scrOff = off;
            return;
        }
        runOnUiThread(() -> {
            ClipboardManager cm = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
            cm.setPrimaryClip(ClipData.newPlainText("scrctl", SCRCTL_CMD));
            new AlertDialog.Builder(MainActivity.this)
                    .setMessage(getString(R.string.scrctl_fail))
                    .setPositiveButton(android.R.string.ok, null)
                    .show();
        });
    }

    private boolean trySendScrCmd(boolean off) {
        try (DatagramSocket socket = new DatagramSocket()) {
            socket.setSoTimeout(1500);

            // Ping
            byte[] ask = {(byte) 0xAA};
            DatagramPacket askPkt = new DatagramPacket(ask, 1, SCR_HOST, SCR_PORT);
            socket.send(askPkt);
            byte[] buf = new byte[1];
            DatagramPacket respPkt = new DatagramPacket(buf, 1);
            socket.receive(respPkt);

            if (buf[0] != (byte) 0x55) {
                Log.w("Main", "ScrCtl bad ack: " + buf[0]);
                return false;
            }

            // Send command: 0=off, 2=on
            byte[] cmd = {(byte) (off ? 0 : 2)};
            DatagramPacket cmdPkt = new DatagramPacket(cmd, 1, SCR_HOST, SCR_PORT);
            socket.send(cmdPkt);
            Log.d("Main", "ScrCtl screen " + (off ? "off" : "on"));
            return true;

        } catch (Exception e) {
            Log.w("Main", "ScrCtl unreachable", e);
            return false;
        }
    }

    private EditText etIP;
    private Button btSaveIP;
    private Button btOffScr;
    private SeekBar sbTurboInterval;
    private ScrollView scrollMain;
    private EditText etTurboInterval;
    private Button btDisableTurbo;
    private Switch swInvertAB;
    private Switch swInvertXY;
    private Switch swTurboA;
    private Switch swTurboB;
    private Switch swTurboX;
    private Switch swTurboY;
    private Switch swTurboL;
    private Switch swTurboR;
    private Switch swTurboZL;
    private Switch swTurboZR;
    private CheckBox cbTurboModeA;
    private CheckBox cbTurboModeB;
    private CheckBox cbTurboModeX;
    private CheckBox cbTurboModeY;
    private CheckBox cbTurboModeL;
    private CheckBox cbTurboModeR;
    private CheckBox cbTurboModeZL;
    private CheckBox cbTurboModeZR;
    private Switch swHomeMap;
    private Switch swPowerMap;
    private Switch swPowerOffMap;
    private EditText etHome;
    private EditText etPower;
    private EditText etPowerOff;
    // Key mapping UI
    private com.google.android.material.tabs.TabLayout tabMode;
    private LinearLayout layoutSimpleMode;
    private LinearLayout layoutCustomMode;
    private Switch swSwapSticks;
    private Switch swSwapSticksSimple;
    private Button[] mKeyMapEdits = new Button[17];
    private int mCapturingN3dsIdx = -1;
    // N3DS key names (matches gN3DsKeyTab order in Gamepad.h)
    private static final String[] N3DS_KEY_NAMES = {
        "A", "B", "X", "Y", "L", "R", "ZL", "ZR",
        "SELECT", "START", "HOME", "POWER",
        "UP", "DOWN", "LEFT", "RIGHT", "SHUTDOWN"
    };
    // N3DS key index constants (mirrors N3DS_KEY_INDEX in Gamepad.h)
    private static final int N3DS_KEY_INDEX_A = 0;
    private static final int N3DS_KEY_INDEX_B = 1;
    private static final int N3DS_KEY_INDEX_X = 2;
    private static final int N3DS_KEY_INDEX_Y = 3;
    private static final int N3DS_KEY_INDEX_L = 4;
    private static final int N3DS_KEY_INDEX_R = 5;
    private static final int N3DS_KEY_INDEX_ZL = 6;
    private static final int N3DS_KEY_INDEX_ZR = 7;
    private static final int N3DS_KEY_INDEX_SELECT = 8;
    private static final int N3DS_KEY_INDEX_START = 9;
    private static final int N3DS_KEY_INDEX_HOME = 10;
    private static final int N3DS_KEY_INDEX_POWER = 11;
    private static final int N3DS_KEY_INDEX_UP = 12;
    private static final int N3DS_KEY_INDEX_DOWN = 13;
    private static final int N3DS_KEY_INDEX_LEFT = 14;
    private static final int N3DS_KEY_INDEX_RIGHT = 15;
    // N3DS_KEY_INDEX_SHUTDOWN = 16 (no mapping entry in UI)

    private static final int KEYMAP_MODE_SIMPLE = 0;
    private static final int KEYMAP_MODE_CUSTOM = 1;
    private static final int MAX_INPUT_KEY_INDEX = 23; // mirrors C++ INPUT_KEY_INDEX_INVALID
    private static final int MAX_N3DS_KEY_TURBO_INDEX = 8;
    private static final int TURBO_INTERVAL_MIN = 50;
    private static final int TURBO_INTERVAL_MAX = 500;

    private WifiManager.WifiLock wifiLock;
    private PowerManager.WakeLock wakeLock;

    private boolean mUpdatingTurboUI;
    private boolean mUpdatingKeyMapUI;

    private void setupTurboButtons() {
        Switch[] sws = {swTurboA, swTurboB, swTurboX, swTurboY,
                        swTurboL, swTurboR, swTurboZL, swTurboZR};
        CheckBox[] cbs = {cbTurboModeA, cbTurboModeB, cbTurboModeX, cbTurboModeY,
                          cbTurboModeL, cbTurboModeR, cbTurboModeZL, cbTurboModeZR};
        for (int i = 0; i < MAX_N3DS_KEY_TURBO_INDEX; i++) {
            final int idx = i;
            sws[i].setOnCheckedChangeListener((btn, on) -> {
                if (mUpdatingTurboUI) return;
                setTurbo(idx, on);
                cbs[idx].setEnabled(on);
                if (!on) cbs[idx].setChecked(false);
            });
            cbs[i].setOnCheckedChangeListener((btn, on) -> {
                if (mUpdatingTurboUI) return;
                setTurboMode(idx, on);
            });
        }
    }

    private void updateTurboButtons() {
        mUpdatingTurboUI = true;
        Switch[] sws = {swTurboA, swTurboB, swTurboX, swTurboY,
                        swTurboL, swTurboR, swTurboZL, swTurboZR};
        CheckBox[] cbs = {cbTurboModeA, cbTurboModeB, cbTurboModeX, cbTurboModeY,
                          cbTurboModeL, cbTurboModeR, cbTurboModeZL, cbTurboModeZR};
        for (int i = 0; i < MAX_N3DS_KEY_TURBO_INDEX; i++) {
            boolean on = getTurbo(i);
            sws[i].setChecked(on);
            cbs[i].setChecked(getTurboMode(i));
            cbs[i].setEnabled(on);
        }
        mUpdatingTurboUI = false;
    }

    private void applyTurboIntervalInput() {
        try {
            int val = Integer.parseInt(etTurboInterval.getText().toString().trim());
            if (val < TURBO_INTERVAL_MIN) val = TURBO_INTERVAL_MIN;
            if (val > TURBO_INTERVAL_MAX) val = TURBO_INTERVAL_MAX;
            setTurboInterval(val);
            sbTurboInterval.setProgress(val);
            etTurboInterval.setText(String.valueOf(val));
        } catch (NumberFormatException e) {
            int cur = getTurboInterval();
            etTurboInterval.setText(String.valueOf(cur));
        }
    }

    private void checkUpdate() {
        new Thread(() -> {
            try {
                boolean isZh = Locale.getDefault().getLanguage().equals(
                        new Locale("zh").getLanguage());
                String api = isZh
                        ? "https://gitee.com/api/v5/repos/rojing/InputRedirectionClient-Android/releases/latest"
                        : "https://api.github.com/repos/OrangeGLC/InputRedirectionClient-Android/releases/latest";
                HttpURLConnection conn = (HttpURLConnection) new URL(api).openConnection();
                conn.setConnectTimeout(5000);
                conn.setReadTimeout(5000);
                if (conn.getResponseCode() != 200) return;
                BufferedReader reader = new BufferedReader(
                        new InputStreamReader(conn.getInputStream()));
                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) sb.append(line);
                reader.close();
                JSONObject json = new JSONObject(sb.toString());
                String latest = json.getString("tag_name").replaceFirst("^v", "");
                if (!latest.equals(BuildConfig.VERSION_NAME)) {
                    String releaseUrl = isZh
                            ? "https://gitee.com/rojing/InputRedirectionClient-Android/releases/tag/v" + latest
                            : json.getString("html_url");
                    runOnUiThread(() -> showUpdateDialog(latest, releaseUrl));
                }
            } catch (Exception e) {
                Log.w("IRC", "Update check failed", e);
            }
        }).start();
    }

    private void showUpdateDialog(String latest, String url) {
        new AlertDialog.Builder(this)
                .setTitle(R.string.update_title)
                .setMessage(getString(R.string.update_msg, latest))
                .setPositiveButton(R.string.update_go, (d, w) -> {
                    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
                })
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }
}