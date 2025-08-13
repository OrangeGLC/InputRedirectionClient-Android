package com.jingrong.inputredirectionclient_android;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;

import com.google.androidgamesdk.GameActivity;

import java.io.File;

public class MainActivity extends GameActivity {
    static {
        System.loadLibrary("inputredirectionclient_android");
    }

    private EditText et_ip;
    private Button bt_save;
    private Button bt_offScr;
    public native void initNative(String path);
    public native void cleanNative();
    public native String saveIPAddress(String input);
    public native String getCfgIP();
    public native void handleKeyEvent(int keyCode, int action, int source);
    public void updateUI()
    {
        runOnUiThread(()->
        {
                setContentView(R.layout.activity_main);
        //Read IP from config file
        et_ip = findViewById(R.id.et_ip);
        et_ip.setText(getCfgIP());

        bt_save = findViewById(R.id.bt_save);
        bt_offScr = findViewById(R.id.bt_offScr);
        et_ip.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int i, KeyEvent keyEvent) {
                handleKeyEvent(keyEvent.getKeyCode(), keyEvent.getAction(), keyEvent.getSource());
                return false;
            }
        });
        //Check the validity of the IP address.
        bt_save.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String input = et_ip.getText().toString();
                if (!isValidIP(input)) {
                    showErrorDialog();
                    return;
                }
                hideKeyboard(MainActivity.this);
                //Save to file by ndk API
                Toast.makeText(MainActivity.this, saveIPAddress(input), Toast.LENGTH_SHORT).show();
            }
        });
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //Read IP from config file
        File inPath = getFilesDir();
        initNative(inPath.getAbsolutePath());
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
                .setMessage("Correct IP address consists of numbers and periods, for example, 192.168.10.20")
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
}