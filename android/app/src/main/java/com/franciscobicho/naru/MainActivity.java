package com.franciscobicho.naru;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[]{
                "SDL2",
                "Naru"
        };
    }
}