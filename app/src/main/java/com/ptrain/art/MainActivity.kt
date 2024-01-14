package com.ptrain.art

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.ptrain.artimple.ARTHook


class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val hook: ARTHook = ARTHook()
        hook.malloc()
    }

    companion object {
        // Used to load the 'art' library on application startup.
        init {
            System.loadLibrary("art")
        }
    }
}