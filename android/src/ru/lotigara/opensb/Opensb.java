package ru.lotigara.opensb;

import android.content.ContextWrapper;
import android.content.Context;
import android.util.Log;
import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDL;

public class Opensb extends SDLActivity {
    @Override
    protected String[] getArguments() {
        Context context = SDL.getContext();
	String[] arguments = {"-bootconfig", context.getFilesDir().getAbsolutePath() + "/sbinit.config"};
        return arguments;
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "main"
        };
    }
}
