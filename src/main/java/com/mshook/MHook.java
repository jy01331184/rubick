package com.mshook;

public class MHook {

    static {
        System.loadLibrary("mshook");
    }

    public static native Object test();
}
