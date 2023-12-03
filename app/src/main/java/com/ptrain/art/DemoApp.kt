package com.ptrain.art

import android.app.Application
import android.content.Context
import com.ptrain.artimple.ARTHook

class DemoApp : Application() {
    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        System.loadLibrary("art")
        val hook: ARTHook = ARTHook()
        hook.initLibs()
        hook.init()
    }

    override fun onCreate() {
        super.onCreate()
    }
}