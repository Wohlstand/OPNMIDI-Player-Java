package ru.wohlsoft.opnmidiplayer;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.IBinder;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Button;
import android.view.View;
import android.Manifest;
import android.content.pm.PackageManager;
import androidx.core.content.ContextCompat;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.core.app.ActivityCompat;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.util.Locale;

public class Player extends AppCompatActivity
{
    final String LOG_TAG = "OPNMIDI";

    public static final int READ_PERMISSION_FOR_BANK = 1;
    public static final int READ_PERMISSION_FOR_MUSIC = 2;
    public static final int READ_PERMISSION_FOR_INTENT = 3;

    private PlayerService m_service = null;
    private volatile boolean m_bound = false;
    private volatile boolean m_uiLoaded = false;

    private SharedPreferences   m_setup = null;

    private String              m_lastPath = "";
    private String              m_lastBankPath = "";
    private String              m_lastMusicPath = "";

    private int                 m_chipsCount = 2;

    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive(Context context, Intent intent) {
            String intentType = intent.getStringExtra("INTENT_TYPE");
            assert intentType != null;
            if(intentType.equalsIgnoreCase("SEEKBAR_RESULT")) {
                int percentage = intent.getIntExtra("PERCENTAGE", -1);
                SeekBar musPos = findViewById(R.id.musPos);
                if(percentage >= 0)
                    musPos.setProgress(percentage);
            }
        }
    };

    public static double round(double value, int places)
    {
        if (places < 0) throw new IllegalArgumentException();

        BigDecimal bd = new BigDecimal(value);
        bd = bd.setScale(places, RoundingMode.HALF_UP);
        return bd.doubleValue();
    }

    public void seekerStart()
    {
        //Register Broadcast receiver
        LocalBroadcastManager.getInstance(this).registerReceiver(mBroadcastReceiver, new IntentFilter("OPNMIDI_Broadcast"));
    }

    public void seekerStop()
    {
        //Unregister Broadcast receiver
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mBroadcastReceiver);
    }

    /** Defines callbacks for service binding, passed to bindService() */
    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className,
                                       IBinder service) {
            // We've bound to LocalService, cast the IBinder and get LocalService instance
            PlayerService.LocalBinder binder = (PlayerService.LocalBinder) service;
            m_service = binder.getService();
            m_bound = true;
            Log.d(LOG_TAG, "mConnection: Connected");
            initUiSetup(true);
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0)
        {
            m_bound = false;
            Log.d(LOG_TAG, "mConnection: Disconnected");
        }
    };

    @SuppressLint("SetTextI18n")
    private void initUiSetup(boolean fromConnect)
    {
        boolean isPlaying = false;

        if(!fromConnect)
        {
            AppSettings.loadSetup(m_setup);
            if(reconnectPlayerService())
                return;
        }

        if (m_bound)
        {
            Log.d(LOG_TAG, "\"bound\" set to true");
            isPlaying = m_service.isPlaying();

            if(isPlaying)
            {
                Log.d(LOG_TAG, "Player works, make a toast");
                seekerStart();
                Toast toast = Toast.makeText(getApplicationContext(), "Already playing", Toast.LENGTH_SHORT);
                toast.show();
            }
            else
                Log.d(LOG_TAG, "Player doesn NOT works");
        }
        else
            Log.d(LOG_TAG, "\"bound\" set to false");

        if(m_uiLoaded)
        {
            Log.d(LOG_TAG, "UI already loaded, do nothing");
            return;
        }

        Log.d(LOG_TAG, "UI is not loaded, do load");

        /*
         * Music position seeker
         */
        SeekBar musPos = (SeekBar) findViewById(R.id.musPos);
        if(m_bound)
        {
            musPos.setMax(m_service.getSongLength());
            musPos.setProgress(m_service.getPosition());
        }
        musPos.setProgress(0);
        musPos.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
        {
            //private double dstPos = 0;
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
            {
                if(fromUser && m_bound)
                    m_service.setPosition(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        /*
         * Filename title
         */
        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.currentFileName);
        tv.setText(PlayerService.stringFromJNI());
        if(isPlaying) {
            tv.setText(m_service.getCurrentMusicPath());
        }

        /*
         * Bank name title
         */
        TextView cbl = (TextView) findViewById(R.id.bankFileName);
        m_lastBankPath = AppSettings.getBankPath();
        if(!m_lastBankPath.isEmpty()) {
            File f = new File(m_lastBankPath);
            cbl.setText(f.getName());
        } else {
            cbl.setText(R.string.noCustomBankLabel);
        }

        /*
         * Use custom bank checkbox
         */
        CheckBox useCustomBank = (CheckBox)findViewById(R.id.useCustom);
        useCustomBank.setChecked(AppSettings.getUseCustomBank());
        useCustomBank.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                AppSettings.setUseCustomBank(isChecked);
                if(m_bound)
                    m_service.setUseCustomBank(isChecked);
            }
        });


        /*
         * Emulator model combo-box
         */
        Spinner sEmulator = (Spinner) findViewById(R.id.emulatorType);
        final String[] emulatorItems =
        {
            "Mame YM2612 OPN2 (accurate and fast)",
            "Nuked OPN2 (very accurate and !HEAVY!)",
            "GENS OPN2 (broken SSG-EG and envelopes)",
            "Genesis Plus GX OPN2 (EXPERIMENTAL)",
            "Neko Project II OPNA (semi-accurate and fast)",
            "Mame YM2608 OPNA (accurate and fast)",
            "PMDWin OPNA (EXPERIMENTAL)"
        };

        ArrayAdapter<String> adapterEMU = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, emulatorItems);
        adapterEMU.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        sEmulator.setAdapter(adapterEMU);
        sEmulator.setSelection(AppSettings.getEmulator());

        sEmulator.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parent,
                                       View itemSelected, int selectedItemPosition, long selectedId) {
                if(m_bound)
                    m_service.setEmulator(selectedItemPosition);
                // Unlike other options, this should be set after an engine-side update
                AppSettings.setEmulator(selectedItemPosition);
            }

            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        /*
         * Volume model combo-box
         */
        Spinner sVolModel = (Spinner) findViewById(R.id.volumeRangesModel);
        final String[] volumeModelItems = {"[Auto]", "Generic", "CMF", "DMX", "Apogee", "9X" };

        ArrayAdapter<String> adapterVM = new ArrayAdapter<>(
                this, android.R.layout.simple_spinner_item, volumeModelItems);
        adapterVM.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        sVolModel.setAdapter(adapterVM);
        sVolModel.setSelection(AppSettings.getVolumeModel());

        sVolModel.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parent,
                                       View itemSelected, int selectedItemPosition, long selectedId)
            {
                AppSettings.setVolumeModel(selectedItemPosition);
                if(m_bound)
                    m_service.setVolumeModel(selectedItemPosition);
            }

            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        /*
         * Channel allocation mode combo-box
         */
        Spinner sChanMode = (Spinner) findViewById(R.id.channelAllocationMode);
        final String[] chanAllocModeItems = {"[Auto]", "Releasing delay", "Released with same instrument", "Any released" };

        ArrayAdapter<String> adapterCA = new ArrayAdapter<>(
                this, android.R.layout.simple_spinner_item, chanAllocModeItems);
        adapterCA.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        sChanMode.setAdapter(adapterCA);
        sChanMode.setSelection(AppSettings.getChanAlocMode());

        sChanMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parent,
                                       View itemSelected, int selectedItemPosition, long selectedId)
            {
                AppSettings.setChanAllocMode(selectedItemPosition);
                if(m_bound)
                    m_service.setChanAllocMode(selectedItemPosition);
            }

            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        /*
         * Scalable Modulators checkbox
         */
        CheckBox scalableMod = (CheckBox)findViewById(R.id.scalableModulation);
        scalableMod.setChecked(AppSettings.getScalableModulation());
        scalableMod.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                AppSettings.setScalableModulators(isChecked);
                if(m_bound)
                    m_service.setScalableModulators(isChecked);
                Toast toast = Toast.makeText(getApplicationContext(),
                        "Scalable modulation toggled!", Toast.LENGTH_SHORT);
                toast.show();
            }
        });

        /*
         * Run at PCM Rate checkbox
         */
        CheckBox runAtPcmRate = (CheckBox)findViewById(R.id.runAtPcmRate);
        runAtPcmRate.setChecked(AppSettings.getRunAtPcmRate());
        runAtPcmRate.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                AppSettings.setRunAtPcmRate(isChecked);
                if(m_bound)
                    m_service.setRunAtPcmRate(isChecked);
                Toast toast = Toast.makeText(getApplicationContext(),
                        "Run at PCM Rate has toggled!", Toast.LENGTH_SHORT);
                toast.show();
            }
        });

        /*
         * Full-Panning Stereo checkbox
         */
        CheckBox fullPanningStereo = (CheckBox)findViewById(R.id.fullPanningStereo);
        fullPanningStereo.setChecked(AppSettings.getFullPanningStereo());
        fullPanningStereo.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                AppSettings.setFullPanningStereo(isChecked);
                if(m_bound)
                    m_service.setFullPanningStereo(isChecked);
                Toast toast = Toast.makeText(getApplicationContext(),
                        "Full-Panning toggled!", Toast.LENGTH_SHORT);
                toast.show();
            }
        });

        /*
         * Automatic arpeggio
         */
        CheckBox autoArpeggio = (CheckBox)findViewById(R.id.autoArpeggio);
        autoArpeggio.setChecked(AppSettings.getAutoArpeggio());
        autoArpeggio.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                AppSettings.setAutoArpeggio(isChecked);
                if(m_bound)
                    m_service.setAutoArpeggio(isChecked);
                Toast toast = Toast.makeText(getApplicationContext(),
                        "Auto Arpeggio toggled!", Toast.LENGTH_SHORT);
                toast.show();
            }
        });

        /*
         * Chips count
         */
        Button numChipsMinus = (Button) findViewById(R.id.numChipsMinus);
        numChipsMinus.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                int g = m_chipsCount;
                g--;
                if(g < 1) {
                    return;
                }
                onChipsCountUpdate(g, false);
            }
        });

        Button numChipsPlus = (Button) findViewById(R.id.numChipsPlus);
        numChipsPlus.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view) {
                int g = m_chipsCount;
                g++;
                if(g > 100) {
                    return;
                }
                onChipsCountUpdate(g, false);
            }
        });

        onChipsCountUpdate(AppSettings.getChipsCount(), true);

        /*
         * Gain level
         */
        Button gainMinusMinus = (Button) findViewById(R.id.gainFactorMinusMinus);
        gainMinusMinus.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                if(m_bound)
                {
                    double gain = AppSettings.getGaining();
                    gain -= 1.0;
                    if(gain < 0.1)
                        gain += 1.0;
                    onGainUpdate(gain, false);
                }
            }
        });

        Button gainMinus = (Button) findViewById(R.id.gainFactorMinus);
        gainMinus.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                if(m_bound)
                {
                    double gain = AppSettings.getGaining();
                    gain -= 0.1;
                    if(gain < 0.1)
                        gain += 0.1;
                    onGainUpdate(gain, false);
                }
            }
        });

        Button gainPlus = (Button) findViewById(R.id.gainFactorPlus);
        gainPlus.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                if(m_bound)
                {
                    double gain = AppSettings.getGaining();
                    gain += 0.1;
                    onGainUpdate(gain, false);
                }
            }
        });

        Button gainPlusPlus = (Button) findViewById(R.id.gainFactorPlusPlus);
        gainPlusPlus.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                if(m_bound)
                {
                    double gain = AppSettings.getGaining();
                    gain += 1.0;
                    onGainUpdate(gain, false);
                }
            }
        });

        onGainUpdate(AppSettings.getGaining(), true);



        /* *******Everything UI related has been initialized!****** */
        m_uiLoaded = true;

        // Try to load external file if requested
        handleFileIntent();

        // TODO: Make the PROPER settings loading without service bootstrapping and remove this mess as fast as possible!!!!
        // WORKAROUND: stop the service
        if(!isPlaying)
            playerServiceStop();
    }

    private void playerServiceStart()
    {
        bindPlayerService();
        Intent intent = new Intent(this, PlayerService.class);
        intent.setAction(PlayerService.ACTION_START_FOREGROUND_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }

    private void playerServiceStop()
    {
        Intent intent = new Intent(this, PlayerService.class);
        intent.setAction(PlayerService.ACTION_CLOSE_FOREGROUND_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_player);

        if(android.os.Build.VERSION.SDK_INT <= android.os.Build.VERSION_CODES.P)
            m_lastPath = Environment.getExternalStorageDirectory().getPath();
        else
            m_lastPath = "/storage/emulated/0";

        m_setup = getPreferences(Context.MODE_PRIVATE);

        m_lastPath = m_setup.getString("lastPath", m_lastPath);
        m_lastMusicPath = m_setup.getString("lastMusicPath", m_lastMusicPath);

        Button quitBut = (Button) findViewById(R.id.quitapp);
        quitBut.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                Log.d(LOG_TAG, "Quit: Trying to stop seeker");
                seekerStop();
                if(m_bound) {
                    Log.d(LOG_TAG, "Quit: Stopping player");
                    m_service.playerStop();
                    Log.d(LOG_TAG, "Quit: De-Initializing player");
                    m_service.unInitPlayer();
                }
                Log.d(LOG_TAG, "Quit: Stopping player service");
                playerServiceStop();

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
                    Log.d(LOG_TAG, "Quit: Finish Affinity");
                    Player.this.finishAffinity();
                } else {
                    Log.d(LOG_TAG, "Quit: Just finish");
                    Player.this.finish();
                }
                Log.d(LOG_TAG, "Quit: Collect garbage");
                System.gc();
            }
        });

        Button openfb = (Button) findViewById(R.id.openFile);
        openfb.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view) {
                OnOpenFileClick(view);
            }
        });

        Button playPause = (Button) findViewById(R.id.playPause);
        playPause.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view) {
                OnPlayClick(view);
            }
        });

        Button restartBtn = (Button) findViewById(R.id.restart);
        restartBtn.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view) {
                OnRestartClick(view);
            }
        });

        Button openBankFileButton = (Button) findViewById(R.id.customBank);
        openBankFileButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view) {
                OnOpenBankFileClick(view);
            }
        });
    }


    @Override
    protected void onStart()
    {
        super.onStart();
        initUiSetup(false);
    }

    private boolean isPlayerRunning()
    {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (PlayerService.class.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }

    /**!
     * Reconnect running player
     */
    private boolean reconnectPlayerService()
    {
        if(isPlayerRunning())
        {
            Log.d(LOG_TAG, "Player is running, reconnect");
            bindPlayerService();
            return true;
        }
        else
            Log.d(LOG_TAG, "Player is NOT running, do nothing");

        return false;
    }

    private void bindPlayerService()
    {
        if(!m_bound)
        {
            Log.d(LOG_TAG, "bind is not exist, making a bind");
            // Bind to LocalService
            Intent intent = new Intent(this, PlayerService.class);
            bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        // Unbind from the service
        if (m_bound) {
            unbindService(mConnection);
            m_bound = false;
        }
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {

            /***
             * TODO: Rpleace this crap with properly made settings box
             * (this one can't receive changed value for "input" EditText field)
             */

            AlertDialog.Builder alert = new AlertDialog.Builder(this);

            alert.setTitle("Setup");
            alert.setMessage("Gaining level");

            // Set an EditText view to get user input
            final EditText input = new EditText(this);
            input.setRawInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
            alert.setView(input);

            if(m_bound) {
                input.setText(String.format(Locale.getDefault(), "%g", AppSettings.getGaining()));
            }

            alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                    String txtvalue = input.getText().toString();
                    onGainUpdate(Double.parseDouble(txtvalue), false);
                }
            });

            alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                    // Canceled.
                }
            });

            alert.show();
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }


    public void OnPlayClick(View view)
    {
        if(m_bound)
        {
            if(!m_service.hasLoadedMusic())
            {
                if(!m_service.isReady())
                    m_service.initPlayer();
                processMusicFileLoadMusic(false);
            }

            if(!m_service.hasLoadedMusic())
                return;

            if(!m_service.isPlaying()) {
                playerServiceStart();
                seekerStart();
            }

            m_service.togglePlayPause();
            if(!m_service.isPlaying()) {
                seekerStop();
                playerServiceStop();
            }
        }
    }

    public void OnRestartClick(View view)
    {
        if(m_bound) {
            if(!m_service.hasLoadedMusic())
                return;
            if(!m_service.isPlaying()) {
                playerServiceStart();
                seekerStart();
            }
            m_service.playerRestart();
        }
    }

    private boolean checkFilePermissions(int requestCode)
    {
        final int grant = PackageManager.PERMISSION_GRANTED;

        if (Build.VERSION.SDK_INT >= 23)
        {
            final String exStorage = Manifest.permission.READ_EXTERNAL_STORAGE;
            if (ContextCompat.checkSelfPermission(this, exStorage) == grant) {
                Log.d(LOG_TAG, "File permission is granted");
            } else {
                Log.d(LOG_TAG, "File permission is revoked");
            }
        }

        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
        {
            final String exStorage = Manifest.permission.READ_EXTERNAL_STORAGE;
            if((ContextCompat.checkSelfPermission(this, exStorage) == grant))
                return false;

            // Should we show an explanation?
            if(ActivityCompat.shouldShowRequestPermissionRationale(this, exStorage))
            {
                // Show an expanation to the user *asynchronously* -- don't block
                // this thread waiting for the user's response! After the user
                // sees the explanation, try again to request the permission.
                AlertDialog.Builder b = new AlertDialog.Builder(this);
                b.setTitle("Permission denied");
                b.setMessage("Sorry, but permission is denied!\n"+
                        "Please, check the Read Extrnal Storage permission to application!");
                b.setNegativeButton(android.R.string.ok, null);
                b.show();
                return true;
            }
            else
            {
                // No explanation needed, we can request the permission.
                ActivityCompat.requestPermissions(this, new String[] { exStorage }, requestCode);
                //MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE
                // MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE is an
                // app-defined int constant. The callback method gets the
                // result of the request.
            }
            return true;
        }

        return false;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (grantResults.length > 0 &&
            permissions[0].equals(Manifest.permission.READ_EXTERNAL_STORAGE) &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED
        ) {
            if (requestCode == READ_PERMISSION_FOR_BANK) {
                openBankDialog();
            } else if (requestCode == READ_PERMISSION_FOR_MUSIC) {
                openMusicFileDialog();
            } else if (requestCode == READ_PERMISSION_FOR_INTENT) {
                handleFileIntent();
            }
        }
    }


    public void OnOpenBankFileClick(View view)
    {
        // Here, thisActivity is the current activity
        if(checkFilePermissions(READ_PERMISSION_FOR_BANK))
            return;
        openBankDialog();
    }

    private final int REQ_OPEN_BANK = 42;

    public void openBankDialog()
    {
        File file = new File(m_lastBankPath);
        OpenFileDialog fileDialog = new OpenFileDialog(this)
                .setFilter(".*\\.wopn")
                .setCurrentDirectory(m_lastBankPath.isEmpty() ?
                        Environment.getExternalStorageDirectory().getPath() :
                        file.getParent())
                .setOpenDialogListener(new OpenFileDialog.OpenDialogListener()
                {
                    @Override
                    public void OnSelectedFile(Context ctx, String fileName, String lastPath)
                    {
                        m_lastBankPath = fileName;
                        AppSettings.setBankPath(m_lastBankPath);

                        TextView cbl = (TextView) findViewById(R.id.bankFileName);
                        if(!m_lastBankPath.isEmpty())
                        {
                            File f = new File(m_lastBankPath);
                            cbl.setText(f.getName());
                        }
                        else
                        {
                            cbl.setText(R.string.noCustomBankLabel);
                        }

                        if(m_bound)
                            m_service.openBank(m_lastBankPath);
                    }

                    @Override
                    public void OnSelectedDirectory(Context ctx, String lastPath) {}
                });
        fileDialog.show();
//        }
    }

//    public static void copy(File src, File dst) throws IOException
//    {
//        InputStream in = new FileInputStream(src);
//        try
//        {
//            OutputStream out = new FileOutputStream(dst);
//            try
//            {
//                // Transfer bytes from in to out
//                byte[] buf = new byte[1024];
//                int len;
//                while ((len = in.read(buf)) > 0) {
//                    out.write(buf, 0, len);
//                }
//            } finally {
//                out.close();
//            }
//        } finally {
//            in.close();
//        }
//    }
//
//    @Override
//    public void onActivityResult(int requestCode, int resultCode,Intent resultData)
//    {
//        super.onActivityResult(requestCode, resultCode, resultData);
//
//        if (requestCode == REQ_OPEN_BANK && resultCode == Activity.RESULT_OK)
//        {
//            Uri uri = null;
//            if (resultData != null)
//            {
//                uri = resultData.getData();
//                String rPath = RealPathUtil.getRealPath(this, uri);
//                File in = new File(rPath);
//                File outFile = new File(getExternalFilesDir(null), in.getName());
//
//                try
//                {
//                    copy(in, outFile);
//                }
//                catch (IOException e)
//                {
//                    e.printStackTrace();
//                }
//
//                m_lastBankPath = outFile.getPath();
//
//                TextView cbl = (TextView) findViewById(R.id.bankFileName);
//                if (!m_lastBankPath.isEmpty())
//                {
//                    File f = new File(m_lastBankPath);
//                    cbl.setText(f.getName());
//                }
//                else
//                {
//                    cbl.setText(R.string.noCustomBankLabel);
//                }
//
//                if (m_bound)
//                    m_service.openBank(m_lastBankPath);
//            }
//        }
//    }

    public void OnOpenFileClick(View view) {
        // Here, thisActivity is the current activity
        if(checkFilePermissions(READ_PERMISSION_FOR_MUSIC))
            return;
        openMusicFileDialog();
    }

    public void openMusicFileDialog()
    {
        OpenFileDialog fileDialog = new OpenFileDialog(this)
            .setFilter(".*\\.mid|.*\\.midi|.*\\.kar|.*\\.rmi|.*\\.mus|.*\\.xmi")
            .setCurrentDirectory(m_lastPath)
            .setOpenDialogListener(new OpenFileDialog.OpenDialogListener()
            {
                @Override
                public void OnSelectedFile(Context ctx, String fileName, String lastPath) {
                    processMusicFile(fileName, lastPath);
                }

                @Override
                public void OnSelectedDirectory(Context ctx, String lastPath) {}
            });
        fileDialog.show();
    }

    private void handleFileIntent()
    {
        Intent intent = getIntent();
        String scheme = intent.getScheme();
        if(scheme != null)
        {
            if(checkFilePermissions(READ_PERMISSION_FOR_INTENT))
                return;
            if(scheme.equals(ContentResolver.SCHEME_FILE))
            {
                Uri url = intent.getData();
                if(url != null)
                {
                    Log.d(LOG_TAG, "Got a file: " + url + ";");
                    String fileName = url.getPath();
                    processMusicFile(fileName, m_lastPath);
                }
            }
            else if(scheme.equals(ContentResolver.SCHEME_CONTENT))
            {
                Uri url = intent.getData();
                if(url != null)
                {
                    Log.d(LOG_TAG, "Got a content: " + url + ";");
                    String fileName = url.getPath();
                    processMusicFile(fileName, m_lastPath);
                }
            }
        }
    }

    private void processMusicFile(String fileName, String lastPath)
    {
        boolean wasPlay = false;
        Toast.makeText(getApplicationContext(), fileName, Toast.LENGTH_LONG).show();
        TextView tv = (TextView) findViewById(R.id.currentFileName);
        tv.setText(fileName);

        m_lastPath = lastPath;
        m_lastMusicPath = fileName;

        if(m_bound)
        {
            //Abort previously playing state
            wasPlay = m_service.isPlaying();
            if (m_service.isPlaying())
                m_service.playerStop();

            if (!m_service.isReady())
            {
                if (!m_service.initPlayer())
                {
                    m_service.playerStop();
                    m_service.unInitPlayer();
                    AlertDialog.Builder b = new AlertDialog.Builder(Player.this);
                    b.setTitle("Failed to initialize player");
                    b.setMessage("Can't initialize player because of " + m_service.getLastError());
                    b.setNegativeButton(android.R.string.ok, null);
                    b.show();
                    return;
                }
            }
        }

        m_setup.edit().putString("lastPath", m_lastPath).apply();
        m_setup.edit().putString("lastMusicPath", m_lastMusicPath).apply();
        processMusicFileLoadMusic(wasPlay);

        bindPlayerService();
    }

    private void processMusicFileLoadMusic(boolean wasPlay)
    {
        if(m_bound)
        {
            //Reload bank for a case if CMF file was passed that cleans custom bank
            m_service.reloadBank();
            if (!m_service.openMusic(m_lastMusicPath)) {
                AlertDialog.Builder b = new AlertDialog.Builder(Player.this);
                b.setTitle("Failed to open file");
                b.setMessage("Can't open music file because of " + m_service.getLastError());
                b.setNegativeButton(android.R.string.ok, null);
                b.show();
            } else {
                SeekBar musPos = (SeekBar) findViewById(R.id.musPos);
                musPos.setMax(m_service.getSongLength());
                musPos.setProgress(0);
                if (wasPlay)
                    m_service.playerStart();
            }
        }
    }

    void onChipsCountUpdate(int chipsCount, boolean silent)
    {
        if(chipsCount < 1) {
            chipsCount = 1;
        } else if(chipsCount > 100) {
            chipsCount = 100;
        }

        m_chipsCount = chipsCount;
        AppSettings.setChipsCount(m_chipsCount);
        if(m_bound && !silent) {
            m_service.applySetup();
            Log.d(LOG_TAG, String.format(Locale.getDefault(), "Chips: Written=%d", m_chipsCount));
        }

        TextView numChipsCounter = (TextView)findViewById(R.id.numChipsCount);
        numChipsCounter.setText(String.format(Locale.getDefault(), "%d", m_chipsCount));
    }

    void onGainUpdate(double gainLevel, boolean silent)
    {
        gainLevel = round(gainLevel, 1);
        AppSettings.setGaining(gainLevel);
        if(m_bound && !silent) {
            m_service.gainingSet(gainLevel);
        }
        TextView gainFactor = (TextView)findViewById(R.id.gainFactor);
        gainFactor.setText(String.format(Locale.getDefault(), "%.1f", gainLevel));
    }
}

