package com.liangfu.pwptracker;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Window;
import android.content.pm.ActivityInfo;

// Sample0Base
public class CvTracker extends Activity {
  private static final String TAG            = "Sample::Activity";

  public static final int     VIEW_MODE_RGBA = 0;
  public static final int     VIEW_MODE_GRAY = 1;

  private MenuItem            mItemPreviewRGBA;
  private MenuItem            mItemPreviewGray;

  public static int           viewMode       = VIEW_MODE_RGBA;

  public CvTracker() {
    Log.i(TAG, "Instantiated new " + this.getClass());
  }

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.i(TAG, "onCreate");
    super.onCreate(savedInstanceState);
    requestWindowFeature(Window.FEATURE_NO_TITLE);

    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

    setContentView(new CvTrackerView(this));
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    Log.i(TAG, "onCreateOptionsMenu");
    mItemPreviewRGBA = menu.add("Preview RGBA");
    mItemPreviewGray = menu.add("Preview GRAY");
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    Log.i(TAG, "Menu Item selected " + item);
    if (item == mItemPreviewRGBA)
      viewMode = VIEW_MODE_RGBA;
    else if (item == mItemPreviewGray)
      viewMode = VIEW_MODE_GRAY;
    return true;
  }
}
