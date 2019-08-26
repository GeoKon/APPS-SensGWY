#include <ESP8266HTTPClient.h>
#include <bufClass.h>
#include <prsClass.h>           // parser class to decode JSON
#include "myGlobals.h"
#include "mySupport.h"

    char *homeurl = "http://192.168.0.";    // to be completed with IP3
    char *urlend  = "/tstat";

    PRS pr( 128 );                          // working buffer for parser
    SENSORS sens;                           // this file
   
// ----------------------- CLASS TO HANDLE ALL Temp Scanning ------------------------------------------
    SENSORS::SENSORS()
    {
        scanidx = 0;       // Indicates sensor to be sampled/HTTP. Runs from 0...SCAN_COUNT-1. Incremented by 'nextScan()'
        previdx = 0;
        totalsc = 0;       // Counts total scans. Used to compute the percent of error
    }
    int SENSORS::getIndex()
    {
        return scanidx;
    }
    int SENSORS::getPrevIndex()
    {
        return previdx;
    }
    int SENSORS::setIndex( int id )
    {
        previdx = scanidx;
        scanidx = id;        
    }
    void SENSORS::nextIndex()
    {
        previdx = scanidx;
        scanidx++;
        if( scanidx >= SCAN_COUNT )
        {    
            scanidx = 0;
            totalsc++;          
            if( totalsc>99 )    // loop around 0...99
                totalsc = 0;
        }
    }
    int SENSORS::getScanCount()
    {
        return totalsc;
    }
 
    // see: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h

    void SENSORS::fetchSensor( BUF *response )    
    {
        fetchSensor( response, scanidx, "/tstat" );
    }
    void SENSORS::fetchSensor( BUF *bf, int idx, const char *url1 )    
    {
        if( WiFi.status() != WL_CONNECTED )
        {
            F_ERROR( idx ) = -99;
            return;
        }              
        HTTPClient httpX;
        
        char url[80];                       // URL construction
        sprintf( url,"%s%d%s", homeurl, myp.gp.ip[ idx ], url1 );
    
        T0  = millis();                     // start timer

        httpX.begin( /*client, */ url );        // ---------- TCP SESSION STARTS --------------
        //httpX.setReuse( true );

        cpu.heapUpdate();
        yield();
        
        if( myp.gp.timeout & 1 )
            httpX.addHeader( "Connection","close");                  // even-odd timeout to set connection

        if( TRACE(1) )
            PFN("%4d: Request [%s] sent", millis()-T0, url);        // typical 1ms 
        
        if( myp.gp.timeout <= 1 )                                   // do not use timeout if 0
            httpX.setTimeout( myp.gp.timeout );

        int httpCode = httpX.GET();
        
        if( TRACE(1) )
        {
            if( (httpCode==200) )
                PFN("%4d: Data received OK", millis()-T0 );             // typical is 32-43-128ms
            else
                PFN("%4d: HTTP code=%d (%s)", millis()-T0, httpCode, httpX.errorToString(httpCode).c_str() );      // no more than 1000ms
        }
        if( httpCode==200 )
        {
            if( bf )                                                // copy if not NULL
                bf->copy( httpX.getString().c_str() );
            if( TRACE(1) )
                PFN("%4d: Received", millis()-T0 );                     // typcal delay 40-265ms         
        }   
        httpX.end();                     // ---------- TCP SESSION ENDS --------------     

        F_ERROR( idx ) = (httpCode==200) ? 0 : httpCode;
        F_DELAY( idx ) = millis()-T0;
    }

    // {"type":1, "count":2, "temp":69.6, "temp2":79.1, "tmode":0, "t_heat":32.0, "tstate":0, "hold":0, "humidity":-1}
    int SENSORS::parseResponse( BUF *bp )
    {
        char *p = bp->c_str();
        int type, count;
        int i=scanidx;
        
        INIT( p );               
        LCURL;
        FIND("type");                 // look for "type"
        if( ERROR )                   // that's a RADIOSTAT
        {
            INIT( p );               
            LCURL;
            FIND("temp");                 // look for "type"
            COLON;
            S_TEMP1( i ) = atof( COPYD );

            S_TYPE(i) = RADIOSTAT;
            S_TEMP2( i ) = 0.0;
            S_HUMID( i ) = 0.0;
        }
        else
        {
            type  = 0;
            count = 0;

            COLON;
            type = atoi( COPYD );

            FIND("count");
            COLON;
            count = atof( COPYD );

            FIND("temp");
            COLON;
            S_TEMP1( i ) = atof( COPYD );
            //PFN("Temp1 %.1f", S_TEMP1(i) );
            
            FIND("temp2");
            COLON;
            S_TEMP2( i ) = atof( COPYD );
            //PFN("Temp2 %.1f", S_TEMP2(i) );

            FIND("t_heat");
            COLON;
            S_TARGET( i ) = atof( COPYD );

            FIND("tstate");
            COLON;
            S_RELAY( i ) = atof( COPYD );

            FIND("humidity");
            COLON;
            S_HUMID( i ) = atof( COPYD );
            //PFN("Humid %.1f", S_TEMP1(i) );

            if( type == 1 )
            {
                if( count == 1 )
                    S_TYPE(i) = OWN_DS18;
                if( count == 2 )
                    S_TYPE(i) = OWN_DS18x2;
            }
            else if( type == 2 )
                S_TYPE(i) = OWN_DHT;
            else if( type == 3 )
                S_TYPE(i) = OWN_HTU;
            else
                S_TYPE(i) = NONE;
        }
        P_ERROR(i) = ERROR;

        if( !P_ERROR(i) )
        {
            myp.sensdat[i].temp01 = myp.sensdat[i].temp1;     
            myp.sensdat[i].temp02 = myp.sensdat[i].temp2;
            myp.sensdat[i].humid0 = myp.sensdat[i].humid;
        }
        if( TRACE(2) )   // copy this to cliAskSensor()
        {
            PF ( "Index:%d, Err:%d, Typ:%d %s ", i, P_ERROR(i), S_TYPE(i), S_LABEL(i) );
            PFN( "T1:%.1fF, T2:%.1fF, Tgt:%.1fF, RL:%d, Humid:%.0f%%", S_TEMP1(i), S_TEMP2(i), S_TARGET(i), S_RELAY(i), S_HUMID(i) );
        }
    }
    
    BUF sensbuf(256);                                   // Temp buffer used only by readNextSensor()
    void SENSORS::readNextSensor()
    {
        nextIndex();                                    // advance to next index
        
        for( int j=0; j<SCAN_COUNT; j++ )
        {
            if( myp.gp.mask[scanidx] != '-' )             // ready to be scanned
            {
                //PFN("Scanning sensor %d", scanidx );
                sens.fetchSensor( &sensbuf, scanidx, "/tstat"  );    
                if( ! F_ERROR(scanidx) )
                    sens.parseResponse( &sensbuf ); 
                break;                                   
            }
            else                                        // loop around until an active sensor is found
                nextIndex();
        }        
    }

    void SENSORS::fillThinger()
    {
        int i = scanidx;
            
        myp.thingsens[i].units1 = S_TEMP01(i);
        switch( S_TYPE(i) )
        {
            case OWN_DS18x2:    myp.thingsens[i].units2 = S_TEMP02(i);
                                sprintf( myp.thingsens[ i ].label, "S%d(T1,T2) %s IP:%d %dms", i, S_LABEL(i), myp.gp.ip[i], F_DELAY(i) );
                                break;
            default:
            case OWN_DS18:      myp.thingsens[i].units2 = 0.0;
                                sprintf( myp.thingsens[ i ].label, "S%d(T) %s IP:%d %dms", i, S_LABEL(i), myp.gp.ip[i], F_DELAY(i) );
                                break;
            case OWN_HTU:
            case OWN_DHT:       myp.thingsens[i].units2 = S_HUMID0(i);
                                sprintf( myp.thingsens[ i ].label, "S%d(T,H) %s IP:%d %dms", i, S_LABEL(i), myp.gp.ip[i], F_DELAY(i) );
                                break;
        }
        sprintf( myp.thingsens[ i ].targS, "%.1f°F (relay:%s)", S_TARGET(i), S_RELAY(i)?"ON":"OFF" );
        
        if( TRACE(4) )
        {
            for( int i=0; i<SCAN_COUNT; i++ )
                PFN("S%d: %5.1f, %5.1f %s %s", i, myp.thingsens[i].units1, myp.thingsens[i].units2, myp.thingsens[i].label, myp.thingsens[i].targS  );
        }
    }
   
