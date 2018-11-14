package com.rubick;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Build;

import java.util.concurrent.atomic.AtomicBoolean;

public class Rubick {

    private static final int COMPARE_WIDTH = 17,COMPARE_HEIGHT = 47;
    private static boolean init = false;
    private static boolean available = false;
    private static ANDROID_VERSION version;

    enum ANDROID_VERSION {
        ANDROID_5X,ANDROID_6X
    }

    static {
        System.loadLibrary("rubick");
    }

    private static ThreadLocal<AtomicBoolean> threadLocal = new ThreadLocal<AtomicBoolean>() {
        @Override
        protected AtomicBoolean initialValue() {
            return new AtomicBoolean();
        }
    };

    public static Bitmap createBitmap(int width, int height, Bitmap.Config config) {

        if (!init) {
            throw new IllegalStateException("Rubick has not init");
        } else if (!available) {
            return Bitmap.createBitmap(width, height, config);
        }

        threadLocal.get().set(true);
        Bitmap bitmap = Bitmap.createBitmap(width, height, config);
        threadLocal.get().set(false);
        if(version == ANDROID_VERSION.ANDROID_5X) {
            BitmapUtil.setBuffer(bitmap,null);
        }

        return bitmap;
    }

    public static boolean needHook() {
        return threadLocal.get().get();
    }

    public synchronized static boolean init(Context context) {
        if (!init) {
            if (Build.VERSION_CODES.LOLLIPOP == Build.VERSION.SDK_INT || Build.VERSION_CODES.LOLLIPOP_MR1 == Build.VERSION.SDK_INT) {
                version = ANDROID_VERSION.ANDROID_5X;
                available = initRubickL();
                if(available){
                    threadLocal.get().set(true);
                    Bitmap.createBitmap(COMPARE_WIDTH, COMPARE_HEIGHT, Bitmap.Config.ARGB_8888);
                    threadLocal.get().set(false);
                }
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && Build.VERSION.SDK_INT <= Build.VERSION_CODES.N_MR1) {
                version = ANDROID_VERSION.ANDROID_6X;
                available = initRubickM();
            }
            init = true;
        }
        return available;
    }

    private native static boolean initRubickL();

    private native static boolean initRubickM();
}
