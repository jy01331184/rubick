package com.rubick;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.widget.ImageView;

public class MainActivity extends AppCompatActivity {

    private Bitmap back;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        back = BitmapFactory.decodeResource(getResources(), R.drawable.ic_launcher);
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, Integer.toHexString(R.layout.activity_main), Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
                System.out.println(Rubick.init(getApplicationContext()));
            }
        });

        findViewById(R.id.toolbar).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                ImageView imageView = (ImageView) findViewById(R.id.img);
                Bitmap bitmap = Rubick.createBitmap(1024, 1024, Bitmap.Config.ARGB_8888);

                Canvas canvas = new Canvas(bitmap);
                canvas.drawColor(Color.YELLOW);
                canvas.drawBitmap(back, 0, 0, new Paint());
                imageView.setImageBitmap(bitmap);
            }
        });

    }

}
