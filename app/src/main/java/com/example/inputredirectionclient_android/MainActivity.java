package com.example.inputredirectionclient_android;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AlertDialog;

import com.google.androidgamesdk.GameActivity;

import java.io.File;

public class MainActivity extends GameActivity {
    static {
        System.loadLibrary("inputredirectionclient_android");
    }

    // 声明UI控件变量
    private EditText et_ip;
    private Button bt_save;
    private Button bt_offScr;
    public native void saveIPAddress(String input);
    public native String getSavedIPAddress(String inPath);
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //read IP from config file
        File inPath = getFilesDir();
        et_ip = findViewById(R.id.et_ip);
        et_ip.setText(getSavedIPAddress(inPath.getAbsolutePath()));

        bt_save = findViewById(R.id.bt_save);
        bt_offScr = findViewById(R.id.bt_offScr);
        //Check the validity of the IP address.
        bt_save.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String input = et_ip.getText().toString();
                if(!isValidIP(input))
                {
                    showErrorDialog();
                    return;
                }
                // 调用NDK原生方法
                saveIPAddress(input);
            }
        });
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
                .setMessage("Correct IP address consists of numbers and periods, for example 192.168.10.20")
                .setPositiveButton("Close", null)
                .show();
    }
}