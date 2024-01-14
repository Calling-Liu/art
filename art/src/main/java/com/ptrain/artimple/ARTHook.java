package com.ptrain.artimple;

import com.bytedance.android.bytehook.ByteHook;

public class ARTHook {
    public void initLibs() {
        ByteHook.init();
    }

    public native void init();
    public native void malloc();
}
