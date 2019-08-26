#pragma once
#include <bufClass.h>
#include <ESP8266HTTPClient.h>

/* HTTP Error Codes
#define HTTPC_ERROR_CONNECTION_REFUSED  (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED       (-4)
#define HTTPC_ERROR_CONNECTION_LOST     (-5)
#define HTTPC_ERROR_NO_STREAM           (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
#define HTTPC_ERROR_ENCODING            (-9)
#define HTTPC_ERROR_STREAM_WRITE        (-10)
#define HTTPC_ERROR_READ_TIMEOUT        (-11)
 */

class SENSORS
{
private:
    int      scanidx;       // Indicates sensor to be sampled/HTTP. Runs from 0...SCAN_COUNT-1. Incremented by 'nextScan()'
    int      previdx;       // previous index
    uint32_t totalsc;       // Counts total scans. Used to compute the percent of error
    uint32_t T0;            // Initialized when HTTP Request is send. Used to report delay of response by 'printURL()'
    WiFiClient client;      // might be needed

public:    
    SENSORS();
    
    int getScanCount();                     // returns scan count (same is totalsc)
    int getIndex();
    int getPrevIndex();
    int setIndex( int id );
    void nextIndex();                       // just advances 'scanidx'. Called by the Modulo() timer in main loop
    
    void fetchSensor( BUF *bf,              // Sends HTTP/GET request to sensor previous specified using setIndex()
                        int idx,            // On return, it sets F_ERROR() and F_DELAY()
                        const char *url1 );    
    void fetchSensor( BUF *bp );            
                                            
    int  parseResponse( BUF *bp );
    void readNextSensor();                  // combines nextIndex(); fetchSensor(); parseSensor()
    void fillThinger();                     // fills data structure of strings needed by Thinger
};
extern SENSORS sens;                        // See this .cpp module
