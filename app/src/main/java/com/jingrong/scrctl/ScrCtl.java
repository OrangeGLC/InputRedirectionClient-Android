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

        Method setPower = getSetPowerMethod();
        Object display = getDisplay();
        if (display == null) {
            System.err.println("Failed to get display token");
            System.exit(-1);
            return;
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
        Method m;
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            m = cls.getDeclaredMethod("getBuiltInDisplay", int.class);
        } else {
            m = cls.getDeclaredMethod("getInternalDisplayToken");
        }
        m.setAccessible(true);
        return m;
    }

    private static Method getSetPowerMethod() throws Exception {
        Class<?> cls = Class.forName("android.view.SurfaceControl");
        Method m = cls.getDeclaredMethod("setDisplayPowerMode", IBinder.class, int.class);
        m.setAccessible(true);
        return m;
    }

    private static Object getDisplay() throws Exception {
        // Android 14+: try DisplayControl for physical display tokens first
        if (Build.VERSION.SDK_INT >= 34) {
            if (DisplayControl.isAvailable()) {
                long[] ids = DisplayControl.getPhysicalDisplayIds();
                if (ids != null) {
                    for (long id : ids) {
                        IBinder token = DisplayControl.getPhysicalDisplayToken(id);
                        if (token != null) return token;
                    }
                }
            }
        }

        Method m = getDisplayMethod();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            return m.invoke(null, 0);
        }
        return m.invoke(null);
    }
}
