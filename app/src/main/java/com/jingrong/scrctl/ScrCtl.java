package com.jingrong.scrctl;

import android.os.Build;
import android.os.IBinder;

import java.io.File;
import java.lang.reflect.Method;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.file.Files;
import java.nio.file.Paths;

public final class ScrCtl {
    private static final int PORT = 64531;
    private static final File LOCK = new File("/data/local/tmp/scrctl.lock");

    public static void main(String[] args) throws Exception {
        String myPid = String.valueOf(android.os.Process.myPid());
        if (LOCK.exists()) {
            String pidStr = new String(Files.readAllBytes(Paths.get(LOCK.getPath()))).trim();
            if (!pidStr.isEmpty() && pidStr.matches("\\d+")
                    && new File("/proc/" + pidStr).exists()) {
                System.out.println("already running (pid " + pidStr + ")");
                System.exit(0);
            }
            LOCK.delete();
        }
        Files.write(Paths.get(LOCK.getPath()), myPid.getBytes());
        LOCK.deleteOnExit();

        Method getDisplay = getDisplayMethod();
        Method setPower   = getSetPowerMethod();
        Object display;
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            display = getDisplay.invoke(null, 0);
        } else {
            display = getDisplay.invoke(null);
        }
        System.out.println("ScrCtl started, listening on port " + PORT);

        byte[] buf = new byte[1];
        DatagramSocket socket = new DatagramSocket(PORT, InetAddress.getLoopbackAddress());
        while (true) {
            DatagramPacket pkt = new DatagramPacket(buf, 1);
            socket.receive(pkt);
            byte cmd = buf[0];
            if (cmd == (byte) 0xAA) {
                byte[] ack = {0x55};
                DatagramPacket resp = new DatagramPacket(ack, 1, pkt.getAddress(), pkt.getPort());
                socket.send(resp);
            }
            else if (cmd == 0x00 || cmd == 0x02) {
                setPower.invoke(null, display, cmd & 0xFF);
                System.out.println("Screen power mode: " + cmd);
            }
        }
    }

    private static Method getDisplayMethod() throws Exception {
        Class<?> cls = Class.forName("android.view.SurfaceControl");
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            return cls.getMethod("getBuiltInDisplay", int.class);
        } else {
            return cls.getMethod("getInternalDisplayToken");
        }
    }

    private static Method getSetPowerMethod() throws Exception {
        Class<?> cls = Class.forName("android.view.SurfaceControl");
        return cls.getMethod("setDisplayPowerMode", IBinder.class, int.class);
    }
}
