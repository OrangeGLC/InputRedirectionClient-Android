package com.jingrong.inputredirectionclient_android;

import android.Manifest;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.text.InputType;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Switch;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;

import com.google.androidgamesdk.GameActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

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
    public void updateUI()
    {
        runOnUiThread(()-> {
            etIP.setText(getCfgIP());
            etIP.setEnabled(false);
            btSaveIP.setText(R.string.bt_txt_edit);
            swInvertAB.setChecked(getInvertAB());
            swInvertXY.setChecked(getInvertXY());
            swTurboA.setChecked(getTurbo(0));
            swTurboB.setChecked(getTurbo(1));
            swTurboX.setChecked(getTurbo(2));
            swTurboY.setChecked(getTurbo(3));
            swTurboL.setChecked(getTurbo(4));
            swTurboR.setChecked(getTurbo(5));
            swTurboZL.setChecked(getTurbo(6));
            swTurboZR.setChecked(getTurbo(7));
            swHomeMap.setChecked(getHomeMapEnable());
            swPowerMap.setChecked(getPowerMapEnable());
            swPowerOffMap.setChecked(getPowerOffMapEnable());
            int interval = getTurboInterval();
            sbTurboInterval.setProgress(interval);
            tvTurboInterval.setText(getString(R.string.turbo_interval_label, interval));
            etHome.setEnabled(getHomeMapEnable());
            etPower.setEnabled(getPowerMapEnable());
            etPowerOff.setEnabled(getPowerOffMapEnable());
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String btPerm = Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
                ? Manifest.permission.BLUETOOTH_CONNECT : Manifest.permission.BLUETOOTH;
        if (checkSelfPermission(btPerm) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{btPerm}, 0);
        }

        //Read IP from config file
        File inPath = getFilesDir();
        initNative(inPath.getAbsolutePath());
        setContentView(R.layout.activity_main);

        etIP = findViewById(R.id.et_ip);
        btSaveIP = findViewById(R.id.bt_save);
        btDisableTurbo = findViewById(R.id.bt_disableturbo);
        sbTurboInterval = findViewById(R.id.sb_turbo_interval);
        tvTurboInterval = findViewById(R.id.tv_turbo_interval);
        btOffScr = findViewById(R.id.bt_offScr);
        swInvertAB = findViewById(R.id.switch_invertAB);
        swInvertXY = findViewById(R.id.switch_invertXY);
        swTurboA = findViewById(R.id.switch_turbo_A);
        swTurboB = findViewById(R.id.switch_turbo_B);
        swTurboX = findViewById(R.id.switch_turbo_X);
        swTurboY = findViewById(R.id.switch_turbo_Y);
        swTurboR = findViewById(R.id.switch_turbo_R);
        swTurboL = findViewById(R.id.switch_turbo_L);
        swTurboZR = findViewById(R.id.switch_turbo_ZR);
        swTurboZL = findViewById(R.id.switch_turbo_ZL);
        swHomeMap = findViewById(R.id.switch_home);
        swPowerMap = findViewById(R.id.switch_power);
        swPowerOffMap = findViewById(R.id.switch_shut);
        etHome = findViewById(R.id.et_home);
        etPower = findViewById(R.id.et_power);
        etPowerOff = findViewById(R.id.et_shut);

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
                for (int i = 0; i < 8; i++) {
                    setTurbo(i,false);
                }
                updateUI();
            }
        });

        sbTurboInterval.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvTurboInterval.setText(getString(R.string.turbo_interval_label, progress));
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {
                setTurboInterval(seekBar.getProgress());
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

        swTurboA.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(0, b);
            }
        });

        swTurboB.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(1, b);
            }
        });

        swTurboX.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(2, b);
            }
        });

        swTurboY.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(3, b);
            }
        });

        swTurboL.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(4, b);
            }
        });

        swTurboR.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(5, b);
            }
        });

        swTurboZL.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(6, b);
            }
        });

        swTurboZR.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(@NonNull CompoundButton compoundButton, boolean b)
            {
                setTurbo(7, b);
            }
        });

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
        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.about_title))
                .setMessage(Html.fromHtml(getString(R.string.about_msg)))
                .setPositiveButton(android.R.string.ok, null)
                .show();
        TextView msg = dlg.findViewById(android.R.id.message);
        if (msg != null) msg.setMovementMethod(LinkMovementMethod.getInstance());
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
    private TextView tvTurboInterval;
    private Button btDisableTurbo;
    private Switch swInvertAB;
    private Switch swInvertXY;
    private Switch swTurboA;
    private Switch swTurboB;
    private Switch swTurboX;
    private Switch swTurboY;
    private Switch swTurboR;
    private Switch swTurboL;
    private Switch swTurboZL;
    private Switch swTurboZR;
    private Switch swHomeMap;
    private Switch swPowerMap;
    private Switch swPowerOffMap;
    private EditText etHome;
    private EditText etPower;
    private EditText etPowerOff;
}