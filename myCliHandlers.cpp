/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <FS.h>
#include <externIO.h>               // IO includes and cpu...exe extern declarations
#include "mgnClass.h"
#include "oledClass.h"

#include "myGlobals.h"
#include "mySupport.h"
#include "commonCLI.h"
#include "myDisplay.h"
#include "myCliHandlers.h"

MGN mgn;                            // allocate Meguno Link class

static char *mychn = "CONFIG";

#define RESPONSE( A, ... ) if( bp )                     \
                            bp->add(A, ##__VA_ARGS__ ); \
                           else                         \
                            PF(A, ##__VA_ARGS__);

 
// ------------------------------ Streaming & Show measurements -----------------------------

    void showMgnSensor( int i /* sensor id */ )
    {
        B64( tmp );                             // temporary stack buffer 
        mgn.init( mychn );                      // to print to console immediately

        if( F_ERROR(i) )                        // if a fetch() error...
        {
            tmp.set( "Sensor:%d Read error:%d Delay:%d", i, F_ERROR(i), F_DELAY(i) );
            mgn.controlSetText("stream1", tmp.c_str() );
            mgn.controlSetText("stream2", " ");
        }
        else                                    // fetch() is done correctly
        {            
            if( P_ERROR(i) )                    // if a parsing error...
            {
                tmp.set( "Sensor:%d, Read delay:%d, Parse error:%d", i, F_DELAY(i), P_ERROR(i) );
                mgn.controlSetText("stream1", tmp.c_str() );
                mgn.controlSetText("stream2", " ");
            }
            else                                // no parsing errors...
            {
                tmp.set( "Sensor:%d, Type:%d (%s), Read delay:%d", i, S_TYPE(i), S_LABEL(i), F_DELAY(i) );
                mgn.controlSetText("stream1", tmp.c_str() );
                
                switch( S_TYPE(i) )             // depanding on sensor type, show results
                {
                    case OWN_DS18x2:    tmp.set( "Temp1=%.1fC (%.1fF)\r\nTemp2=%.1fC (%.1fF)",
                                            FTOC(S_TEMP1(i)), S_TEMP1(i), FTOC(S_TEMP2(i)), S_TEMP2(i) );
                                        break;
                    default:
                    case OWN_DS18:      tmp.set( "Temp1=%.1fC (%.1fF)",
                                            FTOC(S_TEMP1(i)), S_TEMP1(i) );
                                        break;
                    case OWN_HTU:
                    case OWN_DHT:       tmp.set( "Temp1=%.1fC (%.1fF)\r\nHumidity=%.0f%%",
                                            FTOC(S_TEMP1(i)), S_TEMP1(i), S_HUMID(i) );
                                        break;
                }
                mgn.controlSetText("stream2", tmp.c_str() );
            }
        }
    }

// ============================== ACCESS REMOTE SENSORS ===================================

    void cliFetchSensor( int n, char **arg )                     // fetch [i]. Reads sensor and displays eng units
    {
        BINIT( bp, arg );
        int err;
        
        int i = (n>=1) ? atoi( arg[1] ): 0 ;

        sens.fetchSensor( bp, i, "/tstat" );                    // save response into cli buffer
        
        if( F_ERROR(i) )
        {
            RESPONSE("FETCH Error:%d Delay:%d", F_ERROR(i), F_DELAY(i) );
        }
        else
        {            
//          PFN("Delay:%dms %s", F_DELAY(i), bp->c_str() );     // Print this first
            sens.parseResponse( bp );            

            RESPONSE( "Index:%d, Err:%d, Typ:%d %s ", i, P_ERROR(i), S_TYPE(i), S_LABEL(i) );
            RESPONSE( "T1:%.1fF, T2:%.1fF, Tgt:%.1fF, RL:%d, Humid:%.0f%%", S_TEMP1(i), S_TEMP2(i), S_TARGET(i), S_RELAY(i), S_HUMID(i) );
        }
    }
    void mgnFetchSensor( int n, char **arg )                     // fetch [i]. Reads sensor and displays eng units
    {
        cliFetchSensor( n, arg );
        showMgnSensor( sens.getIndex() );
    }

// ============================== GENERIC REMOTE ACCESS by Thinker ===================================    

    void cliSendRPC( int n, char **arg )                      // uses Meguno
    {
        BINIT( bp, arg );
        bool err = false;
        if( n<=2 )
        {
            bp->set("Use send <sindex> <text>\r\n");
            err = true;
        }
        int idx = atoi( arg[1] );
        if( idx >= SCAN_COUNT )
        {
            bp->set("Unvalid index\r\n");
            err = true;
        }
        if( !err )
        {
            BUF url(80);
            url.set( "/cli?cmd=%s", arg[2] );                       // form the URL
            if( n>3 )
                url.add( "+%s", arg[3] );
            if( n>4 )
                url.add( "+%s", arg[4] );
            if( n>5 )
                url.add( "+%s", arg[5] );

            sens.fetchSensor( bp, idx, url.c_str() );               // send RPC to remote. bp is set with the response
        }
        strncpy( myp.rpc.response, bp->c_str(), sizeof( myp.rpc.response ) );   // copy response to Thinger CLI
        // return & print the same
    }
    void cliLastResult( int n, char **arg )
    {
        BINIT( bp, arg );
        int i = (n>=1) ? atoi( arg[1] ): sens.getIndex() ;
        bp->set( "Sensor:%d (%s), IP=%d, Fetch Err:%d, Dly:%d\r\n", i,S_LABEL(i), myp.gp.ip[i], F_ERROR( i ), F_DELAY( i ) );
    }
    void cliAskSensor( int n, char **arg )                     // ask i /url. OR ask i cmd p1 p2 ...
    {
        BINIT( bp, arg );
        bp->init();                                             // clear this buffer
        
        if( n<=2 )
            bp->set("Syntax error\r\n");
        else
        {
            int i = atoi( arg[1] );                                 // index of the sensor
            char *cmd = arg[2];                                     // command
    
            B80( urlbf );
            if( *cmd == '/' )
                urlbf.set( arg[2] );
            else
            {
                urlbf.set( "/cli?cmd=%s", arg[2] );
                for( int i=3; i<n; i++ )
                    urlbf.add( "+%s", arg[i] );
            }                   
            PFN( "Fetching from %d with URL %s\r\n", i, urlbf.c_str() );        // use TRACE to see the flow
            sens.fetchSensor( bp, i, urlbf.c_str() );                           // save response into cli buffer    
 
            // at this point, bp has been filled either with an error or with remote data
        }
//        if( ok )        
//            strncpy( myp.rpc.response, bp->c_str(), sizeof( myp.rpc.response ) );   // copy response to Thinger CLI
    }
        
// ================================== JUST MODIFY/DISPLAY PARAMETERS ================================

    void cliSetScan( int n, char **arg )
    {
        BINIT( bp, arg );
        
        int doi = (n>=1) ? atoi( arg[1] ) : 0;
        
        myp.gp.scan = doi;
             
        myp.saveMyEEParms();            // save scan to EEPROM       
        RESPONSE("ok(%d)", doi );
    }
    void mgnSetScan( int n, char **arg )
    {
        cliSetScan( n, arg );

        nmp.printMgnInfo( mychn, "scan", "updated" );             // update named parameters
        nmp.printMgnParm( mychn, "scan" );            
        
        mgn.init( NULL, mychn );                                  // update remaining UI
        mgn.controlSetText( "stream1", " ");
        mgn.controlSetText( "stream2", " ");
    }
    void mgnShowState( int n, char **arg )                         // show current measurement
    {
        showMgnSensor( sens.getIndex() );
    }
    void cliSetOLED( int n, char **arg )
    {
        BINIT( bp, arg );
        
        int value = (n>=1) ? atoi( arg[1] ) : 1;
        brightOLED( value );
        
        if( value )                     // activate OLED
            showOLED("INIT");           // initialize
        else
            showOLED("CLEAR");

        myp.saveMyEEParms();            // save oledbr to EEPROM    
        RESPONSE("ok(%d)", value );
    }
    void mgnSetOLED( int n, char **arg )
    {
        BINIT( bp, arg );
        cliSetOLED( n, arg );
        nmp.printMgnInfo( mychn, "oledbr", "updated" );             // update named parameters
        nmp.printMgnParm( mychn, "oledbr" );            
    }

    void cliSwapPins( int n, char **arg )
    {
        BINIT( bp, arg );
        char *p;        
        int pins = (n>=1) ? atoi( arg[1] ) : 0;
        swapSerial( pins );
//        if( myp.swapon )
//            p = "Secondary Serial pins active\r\n";
//        else
//            p = "Primary Serial pins active\r\n";
//        RESPONSE( p );
    }
    void cliDummy( int n, char **arg )
    {
        ;   // do nothing. This is intercepted by main loop. Only used for help
    }
    // ============================== CLI COMMAND TABLE =======================================

    CMDTABLE mypTable[]= 
    {
        {"send",     "sindex c1..c4. Send /cli?cmd=c1+..+c4",               cliSendRPC },      
        {"ask",      "si </cmd>|<cmd p1..pn>. Do an HTTP/GET from sensor",  cliAskSensor }, 

        {"oled",     "brightness. Initializes OLED; use 0 to clear OLED",   cliSetOLED}, 
        {"!oled",    "",                                                    mgnSetOLED}, 

        {"swap",     "0=primary 1=secondary set of pins",                   cliSwapPins}, 
  
        {"result",   "[si]. Shows last result of this sensor",              cliLastResult}, 
        {"fetch",    "[sensor] Fetch sensor data and decode measurements",  cliFetchSensor }, 
        {"!fetch",   "Same as above for Meguno",                            mgnFetchSensor }, 

        {"scan",     "mask: 1=scan sensors 2:show Meguno, 4:show OLED",     cliSetScan },
        {"!scan",    "",                                                    mgnSetScan },
        {"!state",   "Shows measurement state",                             mgnShowState },

        {"restart",  "Restart main program",                                cliDummy },
        
        {NULL, NULL, NULL}
    };
    #undef RESPONSE
