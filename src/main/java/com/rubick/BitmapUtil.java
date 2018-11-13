package com.rubick;

import android.graphics.Bitmap;

import java.lang.reflect.Field;

class BitmapUtil {

    private static Field mBufferField;
    private static Field mNativePtrField;

    static {
        try {
            mBufferField = Bitmap.class.getDeclaredField("mBuffer");
            mBufferField.setAccessible(true);
//            mNativePtrField = Bitmap.class.getDeclaredField("mNativePtr");
//            mNativePtrField.setAccessible(true);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    public static byte[] getBuffer(Bitmap bitmap) {
        try {
            return (byte[]) mBufferField.get(bitmap);
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
        return null;
    }

    public static void setBuffer(Bitmap bitmap, byte[] buffer) {
        try {
            mBufferField.set(bitmap, buffer);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static long getNativePtr(Bitmap bitmap) {
        try {
            return (long) mNativePtrField.get(bitmap);
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
        return 0;
    }
}
