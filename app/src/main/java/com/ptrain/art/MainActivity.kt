package com.ptrain.art

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle


class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    companion object {
        // Used to load the 'art' library on application startup.
        init {
            System.loadLibrary("art")
        }
    }
}