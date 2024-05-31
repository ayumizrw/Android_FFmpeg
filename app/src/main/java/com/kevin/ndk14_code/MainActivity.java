package com.kevin.ndk14_code;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    private zrwPlayer zPlayer;
    private TextView tv_state;
    private SurfaceView surfaceView;
    private static final int PERMISSION_REQUEST_CODE = 1;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        TextView tv = findViewById(R.id.sample_text);
        tv_state = findViewById(R.id.tv_state);
        surfaceView =findViewById(R.id.surfaceView);

        tv.setText(stringFromJNI());
        // 检查权限
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            // 请求权限
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CODE);
        } else {
            // 权限已授予
            createDemoFile();
        }

    }

    private void createDemoFile() {
        Log.e("地址","地址"+Environment.getExternalStorageDirectory()+File.separator+"demo.mp4");
        zPlayer =new zrwPlayer();
        zPlayer.setSurfaceView(surfaceView);
        zPlayer.setDataSource(new File(Environment.getExternalStorageDirectory()+File.separator+"demo.mp4").getAbsolutePath());
        zPlayer.setOnPreparedListener(new zrwPlayer.OnPreparedListener() {
            @Override
            public void onPrepared() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        tv_state.setTextColor(Color.GREEN); // 绿色
                        tv_state.setText("恭喜init初始化成功");
                        Toast.makeText(MainActivity.this, "准备成功，即将开始播放", Toast.LENGTH_SHORT).show();
                    }
                });
                zPlayer.start(); // 调用 C++ 开始播放
            }
        });

        zPlayer.setOnErrorListener(new zrwPlayer.OnErrorListener() {
            @Override
            public void onError(final String errorInfo) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "准备成功，即将开始播放", Toast.LENGTH_SHORT).show();
                        tv_state.setTextColor(Color.RED); // 红色
                        tv_state.setText("哎呀,错误啦，错误:" + errorInfo);
                    }
                });
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        zPlayer.prepare();
    }

    @Override
    protected void onStop() {
        super.onStop();
        zPlayer.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        zPlayer.release();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // 权限授予
                createDemoFile();
            } else {
                // 权限被拒绝
                // 处理权限被拒绝的情况
                Log.e("","权限被拒绝");
            }
        }
    }

    // 用于显示错误信息的方法
    public void showError(String message) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }

    public native String stringFromJNI();
}
