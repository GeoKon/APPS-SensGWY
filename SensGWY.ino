/* 
    ## Sensor Gateway  

Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
You may use this code as you like, as long as you attribute credit to the author.

This .ino includes commonCLI.cpp and .h from TempHumdSens directory

```
---------------------------------------------------------------------------------
*/
    #define INC_THINGER     // select this to enable THINGER interface
//  #undef  INC_THINGER

//    #define INC_SERVER    // select this to enable WEB server
  #undef  INC_SERVER

// ------------------ CONSTANTS & DECLARATIONS ------------------------------
    
    #define _DISABLE_TLS_           // very important for the Thinger.io 
    #define USERNAME "GeoKon"
    #define DEVICE_ID "THERMOSTATS"
    #define DEVICE_CREDENTIAL "success"
    
    #define MYLED 16              // 16 for NodeCMCU, 13 for SONOFF

// ----------------- extra includes ------------------------------------------------

    #include "ESP8266WebServer.h"

    #include <FS.h>
    #include <bufClass.h>       // in GKE-L1
    #include <ticClass.h>       // in GKE-L1
    
    #include "SimpleSRV.h"      // in GKE-Lw
    #include "SimpleSTA.h"      // in GKE-Lw
    
    #include "myGlobals.h"          // in this project. This includes externIO.h
    #include "myEndPoints.h"        // only used if INC_SERVER
    #include "commonCLI.h"
    #include "myCliHandlers.h"
    #include "mySupport.h"          // Handles remote access to thermostats
    
    #include <ThingerESP8266.h>

//------------------ Class Allocations ---------------------------------------

    CPU cpu;                    // declared by externIO.h
    CLI cli;
    EXE exe;
    EEP eep;
    
    ESP8266WebServer server( 80 );
    
    BUF buffer( 1512 );             // buffer to hold the response from the local or remote CLI commands.
//  BUF megbuf( 256  );             // used by loop() to prepare the scan report
    
    ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
    
#define SCAN_INTERVAL 5
#define THNG_INTERVAL 5
    TICsec  tic( SCAN_INTERVAL );               // how often to poll remote thermostats (http GET)
    TICsec  tak( THNG_INTERVAL );               // how often to send data to Thinger
    
// --------------------- Forward references ---------------------------------
    void sendtoDropbox();
    void setupThinger();
// -------------------------------- setup() ---------------------------------

void setup() 
{
    cpu.init( 115200, MYLED+NEGATIVE_LOGIC /* LED */ );
    ASSERT( SPIFFS.begin() );                           // start the filesystem

    myp.initAllParms( MAGIC_CODE );                     // initialize volatile & user EEPROM parameters
    
    exe.registerTable( cmnTable  );                      // register common CLI table
    exe.registerTable( mypTable );                      // register tables to CLI
    
    startCLIAfter( 10/*sec*/, &buffer );                // this also initializes cli(). See SimpleSTA.cpp   
       
    setupSTA();                                         // Start WiFi and obtain an IP address
    // thing.add_wifi( eep.wifi.ssid, eep.wifi.pwd );   // alternative?
    setupThinger();

/*  Alternative way:
      #include <ESP8266WiFiMulti.h>
      Allocate: ESP8266WiFiMulti WiFiMulti;
      In setup(): WiFi.mode(WIFI_STA); WiFiMulti.addAP( eep.wifi.ssid, eep.wifi.pwd );
      In loop(): if ((WiFi.status() == WL_CONNECTED) && !(WiFi.localIP()==INADDR_NONE) )...else cpu.blink(200)
*/
    #ifdef INC_SERVER
        srvCallbacks( server, Landing_STA_Page );           // standard WEB callbacks. "staLanding" is /. HTML page
        cliCallbacks( server, buffer );                     // enable WEB CLI with buffer specified
        //gwyCallbacks();
        
        setTrace( T_REQUEST | T_JSON );                     // default trace    
        server.begin( eep.wifi.port );                      // start the server
        PRN("HTTP server started.");    
    #endif

    cli.prompt();
}  

void setupThinger()
{   
#ifdef INC_THINGER
    // ------------ CLI with Thinker.io --------------------------------------

    thing["rpcCMD"]    << [](pson& cmd){ 
                            strncpy( myp.rpc.command, (const char *)cmd, sizeof( myp.rpc.command ) );      
                            myp.rpc.ready=true; 
                        };
    thing["rpcRSP"]    >> [](pson& ot){ ot = (const char *)myp.rpc.response; };

   // ------------ FROM THE Thinker.io TO THE DEVICE -------------------------

    thing["trace"]    << inputValue( myp.gp.trace  );

    // To Do: Optime this with LOOPs
    
    thing["relayS0"]   << [](pson& in){  static int RL = 0;
                                        if(in.is_empty()) in = RL; //S_RELAY(0);
                                        else
                                        {
                                            RL=in; sens.fetchSensor( NULL, 0, RL ? "/cli?cmd=!relay+1" : "/cli?cmd=!relay+0" ); 
                                        }
                                      };

    thing["relayS1"]   << [](pson& in){  static int RL = 0;
                                        if(in.is_empty()) in = RL; //S_RELAY(1);
                                        else
                                        {
                                            RL=in; sens.fetchSensor( NULL, 1, RL ? "/cli?cmd=!relay+1" : "/cli?cmd=!relay+0" ); 
                                        }
                                      };
    thing["relayS2"]   << [](pson& in){  static int RL = 0;
                                        if(in.is_empty()) in = RL; //S_RELAY(2);
                                        else
                                        {
                                            RL=in; sens.fetchSensor( NULL, 2, RL ? "/cli?cmd=!relay+1" : "/cli?cmd=!relay+0" ); 
                                        }
                                      };
    
    ASSERT( SCAN_COUNT >= 3 );  // number of sensors actually installed
                                    
    thing["simulS0"]   << [](pson& in){  static int V=-10;
                                        if(in.is_empty()) in = V;       //(int) S_TARGET(0);
                                        else
                                        {
                                            V = in;
                                            char url[80]; sprintf( url, (V>-10) ? "/cli?cmd=!simul+%d" : "/cli?cmd=!simul+OFF" , V );
                                            sens.fetchSensor( NULL, 0, url ); 
                                        }
                                     };
    thing["simulS1"]   << [](pson& in){  static int V=-10;
                                        if(in.is_empty()) in = V;       //(int) S_TARGET(0);
                                        else
                                        {
                                            V = in;
                                            char url[80]; sprintf( url, (V>-10) ? "/cli?cmd=!simul+%d" : "/cli?cmd=!simul+OFF" , V );
                                            sens.fetchSensor( NULL, 1, url );  
                                        }
                                     };
    thing["simulS2"]   << [](pson& in){  static int V=-10;
                                        if(in.is_empty()) in = V;       //(int) S_TARGET(0);
                                        else
                                        {
                                            V = in;
                                            char url[80]; sprintf( url, (V>-10) ? "/cli?cmd=!simul+%d" : "/cli?cmd=!simul+OFF" , V );
                                            sens.fetchSensor( NULL, 2, url ); 
                                        }
                                     };
    
    thing["dboxTm"]   << [](pson& in){          // replaced: inputValue( myp.gp.dboxTm );
                                if(in.is_empty())
                                    in = myp.gp.dboxTm;
                                else
                                {
                                    myp.gp.dboxTm = in;
                                    myp.saveMyEEParms();
                                }
                            };
    thing["scanCntl"] << [](pson& in){
                                if(in.is_empty())
                                    in = (bool) (myp.gp.scan > 0 ? true : false);
                                else
                                    myp.gp.scan = in ? 1 : 0;
                            };
                            // was inputValue( myp.gp.scan );
    
    // ------------ FROM THE DEVICE TO Thinker.io -----------------------------
   
    thing["reading"] >> [](pson& ot )
    {
        ot[ "TempS0" ] = myp.thingsens[0].units1;    // temp
        ot[ "TempS1" ] = myp.thingsens[1].units1;    // temp
        ot[ "TempS2" ] = myp.thingsens[2].units1;    // temp
        ot[ "TempS3" ] = myp.thingsens[3].units1;    // temp
        
        ASSERT( SCAN_COUNT>=4 );
        
        ot[ "TorH_S0" ] = myp.thingsens[0].units2;    // temp
        ot[ "TorH_S1" ] = myp.thingsens[1].units2;    // temp
        ot[ "TorH_S2" ] = myp.thingsens[2].units2;    // temp
        ot[ "TorH_S3" ] = myp.thingsens[3].units2;    // temp

        ot[ "targS0" ] = (const char *)myp.thingsens[0].targS;
        ot[ "targS1" ] = (const char *)myp.thingsens[1].targS;
        ot[ "targS2" ] = (const char *)myp.thingsens[2].targS;
        ot[ "targS3" ] = (const char *)myp.thingsens[3].targS;

        ot[ "infoS0" ] = (const char *)myp.thingsens[0].label;
        ot[ "infoS1" ] = (const char *)myp.thingsens[1].label;
        ot[ "infoS2" ] = (const char *)myp.thingsens[2].label;
        ot[ "infoS3" ] = (const char *)myp.thingsens[3].label;

        ot[ "scanCount" ]  = (int) sens.getScanCount();
        ot[ "scanStatus" ] = (const char *)myp.gp.scan?"ON":"OFF";
        ot[ "sensIndex" ]  = (int) sens.getIndex();              
    };
#endif
}

void loop() 
{
    if( cli.ready() )                                   // handle serial interactions
    {
        exe.dispatchBuf( cli.gets(), buffer );          // required by Meguno
        buffer.print();
        cli.prompt();
        if( myp.gp.scan )
             CRLF();
    }
    if( tic.ready() && myp.gp.scan )
    {
        int e, i;
        sens.readNextSensor();          // Advance index and fetch data from next sensor
                                        // This also parses the string and fills the myp.sensdat. structure
        i = sens.getIndex();            // this is the index of this sensor
        e = F_ERROR( i );               // this is the error fetching data from sensor
        
        PFN("Fetched sensor %d. Error: %d", i, e );
        
        sens.fillThinger();            // this converts the data structure to Thinger strings and objects
        
        if( myp.gp.scan & 2 )          // if the volatile flag is set, data are sent to MegunoLink
            showMgnSensor( i ); 
    }
#ifdef INC_THINGER    

    if( tak.ready() )
    {
        thing.stream( thing["reading"] );   // report to Thinker.io every SCAN_INTERVAL
        thing.stream( thing["rpcRSP"] );
        sendtoDropbox();
    }
    
    if( myp.rpc.ready )                 // if command is pending
    {
        PFN( "Executing [%s]", myp.rpc.command );
        exe.dispatchBuf( myp.rpc.command, buffer );          
        
        buffer.print();                 // show to the console & copy to response buffer
        strncpy( myp.rpc.response, buffer.c_str(), sizeof( myp.rpc.response ) );
        myp.rpc.ready = false;
    }
    thing.handle();
#endif
    
#ifdef INC_SERVER
    server.handleClient();
#endif
    yield();
}

// ------------------------- encaptulate Dropbox handling ----------------------------
#ifdef INC_THINGER  
void sendtoDropbox()
{
    static int count = 0; 
    pson data;
    if( myp.gp.dboxTm == 0  )               // do nothing if is zero
    {
        count = 0;
        return;                             // do nothing if not enabled
    }
    if( count == 0 )
    {
        BUF buf(200);                       // Create string for Dropbox

        buf.set( "S0:%5.1f,%5.1f,\tS1:%5.1f,%5.1f,\tS2:%5.1f,%5.1f", 
            myp.thingsens[0].units1,
            myp.thingsens[0].units2,
            myp.thingsens[1].units1,
            myp.thingsens[1].units2,
            myp.thingsens[2].units1,
            myp.thingsens[2].units2 );
        
        data["value1"] = (const char *) buf.c_str();
        PF("Dropbox %s\r\n", buf.c_str() );
      
        thing.call_endpoint( "IFTTT_TEMPS", data );
    }
    count++;    // increments by one, every time a measurement is taken, METER_READING_PERIOD
    if( count > ((myp.gp.dboxTm*60)/THNG_INTERVAL) )
        count = 0;
}
#endif
/*
```
*/
