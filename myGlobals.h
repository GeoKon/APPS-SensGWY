/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#pragma once
    
    #include <bufClass.h>
    #define SCAN_COUNT 5        // max number of URLs to get data from. Only myp.gp.nscans actually are read
    #define MAGIC_CODE 0x1903
    
    #include "externIO.h"       // in GKE-Lw. Includes and externs of cpu,...,eep
    #include "nmpClass.h"

// ------------------ Global structures and variables -------------------

    extern NMP nmp;             // allocated in myGlobals.cpp; used by this and CLi

// ---------------- add here global parameters -----------------------

//    enum thermode_t { NO_RELAY=0, HEAT_MODE=1, COOL_MODE=2 };

extern char *senstype[];        // allocated in myGlobals.cpp

enum type_t { NONE=0, RADIOSTAT=1, OWN_DS18=2, OWN_DS18x2=3, OWN_DHT=4, OWN_HTU=5 };
//    SENSOR_NONE=0, SENSOR_DS18=1, SENSOR_DHT=2, SENSOR_HTU=3 

#define S_TYPE(A)  myp.sensdat[A].type
#define S_TEMP1(A) myp.sensdat[A].temp1
#define S_TEMP2(A) myp.sensdat[A].temp2
#define S_HUMID(A) myp.sensdat[A].humid
#define S_TARGET(A) myp.sensdat[A].target
#define S_RELAY(A) myp.sensdat[A].relay

#define S_TEMP01(A) myp.sensdat[A].temp01
#define S_TEMP02(A) myp.sensdat[A].temp02
#define S_HUMID0(A) myp.sensdat[A].humid0

#define F_ERROR(A) myp.sensdat[A].fetch_err
#define F_DELAY(A) myp.sensdat[A].fetch_dly
#define P_ERROR(A) myp.sensdat[A].parse_err
#define S_LABEL(A) senstype[ (int) myp.sensdat[A].type ]

#define TRACE(A)   (myp.gp.trace & A)
#define CTOF(A) (32.0 + (A)*9.0/5.0)
#define FTOC(A) ( ((A)-32.0)*5.0/9.0 )


    class Global
    {
      public:                                               // ======= A1. Add here all volatile parameters 

        struct sensdata_t
        {                       // set by fetchSensor()
            int fetch_dly;                                  // delay set by fetchSensor()
            int fetch_err;                                  // error code set by fetchSensor()
                                // set by parseSensor()           
            type_t type;                                    // can be used as the index of the label
            float temp1, temp01;                            // temp0x is the LAST GOOD one
            float temp2, temp02;                            // These are set by parseSensor()
            float target;                                   // target temp aka t_heat (temp1 only). Assumes heat mode
            int   relay;                                    // Relay state aka tstate.
            float humid, humid0;
            int parse_err;                                  // error code set by parseSensor()

        } sensdat[ SCAN_COUNT ];        

        #define TEMP_COUNT 5
        #define HUMD_COUNT 3
        
        struct thingsens_t                                  // set by sens.fillThinger() in mySupport.cpp
        {
            float units1;                                   // first measurement
            float units2;                                   // second measurement
            char  targS[40];                                // target temperature & relay state
            char  label[40];                                // label
               
        } thingsens[ SCAN_COUNT ];

        struct rpc_t                                        // allocated in myGlobal.cpp
        {
            char command [128];
            char response[ 512 ];
            bool ready;                                     // true=command is pending for execution
        } rpc;
        
        void initVolatile()                                 // ======= A2. Initialize here the volatile parameters
        {
            for( int i=0; i<SCAN_COUNT; i++ )
            {
                sensdat[i].type = NONE;      
                sensdat[i].temp1  = sensdat[i].temp2  = sensdat[i].humid  = 0.0;
                sensdat[i].temp01 = sensdat[i].temp02 = sensdat[i].humid0 = 0.0;
                sensdat[i].target = 0.0;
                sensdat[i].relay = 0;
                sensdat[i].fetch_dly = 0;
                sensdat[i].fetch_err = 0;
                sensdat[i].parse_err = 0;

                thingsens[i].units1 = 0.0;
                thingsens[i].units2 = 0.0;
                thingsens[i].label[0] = 0;
                thingsens[i].targS[0] = 0;
            }
            rpc.command[0]=0;
            rpc.response[0] = 0;
            rpc.ready = false;    
        }
        void printVolatile( char *prompt="", BUF *bp=NULL ) // ======= A3. Add to buffer (or print) all volatile parms
        {
            ;
        }
        struct gp_t                                         // ======= B1. Add here all non-volatile parameters into a structure
        {                           
            int scan;                   // enable/disable scanning of sensors
            int timeout;                // fetch timeout in ms
            int trace;                  // debug, serial port tracing mask
            int dboxTm;                 // how ofter to save to Dropbox 
            char mask[USER_STR_SIZE];   // mask of sensor state 0=off, 1=on
            int ip[ SCAN_COUNT ];
        } gp;
        
        void initMyEEParms()                                // ======= B2. Initialize here the non-volatile parameters
        {
            gp.scan   = 0;
            gp.timeout = 2000;
            gp.trace  = 3;
            gp.dboxTm = 0;
            strcpy( gp.mask, "X----" );
            gp.ip[0] = 115;
            gp.ip[1] = 28;
            gp.ip[2] = 29;
            gp.ip[3] = 0;            
            gp.ip[4] = 0;  
        }   
        void registerMyEEParms()                           // ======= B3. Register parameters by name
        {
            nmp.resetRegistry();
            nmp.registerParm( "scan",     'd', &gp.scan,       "0=do not scan sensors 1=scan 3=scan and display)"    );
            nmp.registerParm( "trace",    'd', &gp.trace,      "0=traceOFF, 1=Requests, 2=Responses, 4=Remote Cmd"    );
            nmp.registerParm( "timeout",  'd', &gp.timeout,    "max delay waiting to fetch sensor data"    );
            nmp.registerParm( "dbox",     'd', &gp.dboxTm,     "0=OFF, N[sec]=store in Dropbox"    );

            nmp.registerParm( "mask",         's', &gp.mask,      "X scan this sensor, - do not scan this sensor"    );            
            nmp.registerParm( "sens0IP",      'd', &gp.ip[0],     "Sensor 0 IP is 192.168.0.sens0IP"    );
            nmp.registerParm( "sens1IP",      'd', &gp.ip[1],     "Replace 0 with 1 above"    );
            nmp.registerParm( "sens2IP",      'd', &gp.ip[2],     "Replace 0 with 2 above"    );
            nmp.registerParm( "sens3IP",      'd', &gp.ip[3],     "Replace 0 with 3 above"    );
            nmp.registerParm( "sens4IP",      'd', &gp.ip[4],     "Replace 0 with 4 above"    );

            PF("%d named parameters registed\r\n", nmp.getParmCount() );
            ASSERT( nmp.getSize() == sizeof( gp_t ) );                      
        }
        void printMyEEParms( char *prompt="", BUF *bp=NULL ) // ======= B4. Add to buffer (or print) all volatile parms
        {
            nmp.printAllParms( prompt );
        }
        #include <GLOBAL.hpp>                               // Common code for all Global implementations
      
        //    void initAllParms( int myMagic  )       
        //    void fetchMyEEParms()
        //    void saveMyEEParms()                              // saves USER parameters (only)
    };

// ----------------- allocated in mypGlobals.cpp -----------------------
   
    extern Global myp;
