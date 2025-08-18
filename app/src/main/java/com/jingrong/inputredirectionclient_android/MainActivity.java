package com.jingrong.inputredirectionclient_android;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Switch;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;

import com.google.androidgamesdk.GameActivity;

import java.io.File;

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
            etHome.setEnabled(getHomeMapEnable());
            etPower.setEnabled(getPowerMapEnable());
            etPowerOff.setEnabled(getPowerOffMapEnable());
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //Read IP from config file
        File inPath = getFilesDir();
        initNative(inPath.getAbsolutePath());
        setContentView(R.layout.activity_main);

        etIP = findViewById(R.id.et_ip);
        btSaveIP = findViewById(R.id.bt_save);
        btDisableTurbo = findViewById(R.id.bt_disableturbo);
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
    private EditText etIP;
    private Button btSaveIP;
    private Button btOffScr;
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