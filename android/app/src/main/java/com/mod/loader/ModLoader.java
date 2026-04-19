package com.mod.loader;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.text.InputType;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

public class ModLoader {
    static {
        System.loadLibrary("subway_mod");
    }

    private static volatile String sLastSeedInput = "";
    private static volatile boolean sHasNewSeedInput = false;

    public native void initMod();
    public native void toggleMenu();
    public native boolean isMenuVisible();

    public static void promptSeedInput(final String current) {
        final Activity activity = findUnityActivity();
        if (activity == null) {
            return;
        }
        activity.runOnUiThread(new Runnable() {
            @Override public void run() {
                try {
                    Context ctx = activity;
                    LinearLayout container = new LinearLayout(ctx);
                    container.setOrientation(LinearLayout.VERTICAL);
                    int pad = dp(ctx, 16);
                    container.setPadding(pad, pad, pad, pad);

                    TextView hint = new TextView(ctx);
                    hint.setText("enter seed (digits, empty = random)");
                    hint.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
                    container.addView(hint);

                    final EditText edit = new EditText(ctx);
                    edit.setInputType(InputType.TYPE_CLASS_NUMBER);
                    edit.setText(current != null ? current : "");
                    edit.setSelection(edit.getText().length());
                    edit.setHint("0 .. 4294967295");
                    LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT);
                    lp.topMargin = dp(ctx, 8);
                    container.addView(edit, lp);

                    AlertDialog dialog = new AlertDialog.Builder(activity)
                        .setTitle("SEED_VALUE")
                        .setView(container)
                        .setPositiveButton("OK", new android.content.DialogInterface.OnClickListener() {
                            @Override public void onClick(android.content.DialogInterface d, int w) {
                                sLastSeedInput = edit.getText() != null ? edit.getText().toString() : "";
                                sHasNewSeedInput = true;
                            }
                        })
                        .setNeutralButton("RANDOM", new android.content.DialogInterface.OnClickListener() {
                            @Override public void onClick(android.content.DialogInterface d, int w) {
                                sLastSeedInput = "";
                                sHasNewSeedInput = true;
                            }
                        })
                        .setNegativeButton("CANCEL", null)
                        .create();
                    dialog.show();
                    edit.requestFocus();
                } catch (Throwable t) {
                }
            }
        });
    }

    public static String pollSeedInput() {
        if (!sHasNewSeedInput) return null;
        sHasNewSeedInput = false;
        return sLastSeedInput;
    }

    private static Activity findUnityActivity() {
        try {
            Class<?> up = Class.forName("com.unity3d.player.UnityPlayer");
            java.lang.reflect.Field f = up.getField("currentActivity");
            Object o = f.get(null);
            if (o instanceof Activity) return (Activity) o;
        } catch (Throwable ignored) {}
        try {
            Class<?> th = Class.forName("android.app.ActivityThread");
            Object cur = th.getMethod("currentActivityThread").invoke(null);
            java.util.Map<?, ?> records =
                (java.util.Map<?, ?>) th.getDeclaredField("mActivities").get(cur);
            if (records != null) {
                for (Object rec : records.values()) {
                    java.lang.reflect.Field pausedField = rec.getClass().getDeclaredField("paused");
                    pausedField.setAccessible(true);
                    boolean paused = pausedField.getBoolean(rec);
                    if (paused) continue;
                    java.lang.reflect.Field actField = rec.getClass().getDeclaredField("activity");
                    actField.setAccessible(true);
                    Object act = actField.get(rec);
                    if (act instanceof Activity) return (Activity) act;
                }
            }
        } catch (Throwable ignored) {}
        return null;
    }

    private static int dp(Context ctx, int v) {
        float d = ctx.getResources().getDisplayMetrics().density;
        return (int) (d * v + 0.5f);
    }
}
