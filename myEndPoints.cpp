/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#include <ESP8266HTTPClient.h>
#include "SimpleSRV.h"      // in GKE-Lw

#include "myGlobals.h"      // in this project
#include "mySupport.h"
#include "myEndPoints.h"    // in this project

// ---------------- Local variable/class allocation ---------------------

extern ESP8266WebServer server;
   
// ---------------- Local variable/class allocation ---------------------

    BUF cliresp(1000);                      // allocate buffer for CLI response

// ------------- main Callbacks (add your changes here) -----------------
void gwyCallbacks()
{
    static uint32_t T0;
    
//    server.on("/",
//    [](){
//        server.send(200, "text/html",
//        "<h1 align=\"center\">Gateway Services<br/><a href=\"index.htm\">Click for INDEX</a></h1>"
//        );
//    });

    // ===================== WEB CLI INCLUDING REMOTES =============================
//    server.on("/webcli.htm", HTTP_GET,              // when command is submitted, webcli.htm is called  
//    [](){
//        showArgs();
//        if( server.args() )                             // if command given
//            rcli.commandStr( !server.arg(0) );          // Command execution is started here but valid responses might be pending...
//        // T0 = millis();                               // start a timer;
//        showJson( !cli_rspbuf );                        // show the same on the console
//        handleFileRead("/webcli.htm" );                 // reprint same html
//    });
//    server.on("/clirsp", HTTP_GET,      
//    [](){
//        showArgs();
//        if( rcli.pending )
//            server.send(200, "text/html", getFullURL("Waiting for", rcli.sensoridx ).pntr );
//        else
//        {
//            showJson( !cli_rspbuf );
//            server.send(200, "text/html", !cli_rspbuf );
//        }
//    });
//    server.on("/cancel", HTTP_GET,      
//    [](){
//        showArgs();
//        rcli.pending = false;
//        cli_rspbuf.set("Cancelled");
//        server.send(200, "text/html", !cli_rspbuf );
//    });

    // ===================== WEB CLI ENDPOINT ====================================
//    server.on("/cli", HTTP_GET,              // when command is submitted, webcli.htm is called  
//    [](){
//        showArgs();
//        if( server.args() )                                 // if command given
//        {
//            cliresp.init();                                 // initialize the response buffer
//            exe.dispatchBuf( !server.arg(0), cliresp );     // command is executed here and response is saved in 'cliresp' buffer
//        }        
//        showJson( !cliresp );                           // show the same on the console
//        server.send(200, "text/plain", !cliresp );
//    });

    // ===================== MULTI SENSOR READING ====================================
    server.on("/sense", HTTP_GET,              // when command is submitted, webcli.htm is called  
    [](){
        showArgs();
        BUF temp(100);
        temp.set("{");
//        temp.add("'ip0':%d,'sen0':%5.1f,", myp.gp.ip[0], rd[0].measurement );
//        temp.add("'ip1':%d,'sen1':%5.1f,", myp.gp.ip[1], rd[1].measurement );
//        temp.add("'ip2':%d,'sen2':%5.1f,", myp.gp.ip[2], rd[2].measurement );
//        temp.add("'ip3':%d,'sen3':%5.1f,", myp.gp.ip[3], rd[3].measurement );
//        temp.add("'ip4':%d,'sen4':%5.1f}", myp.gp.ip[4], rd[4].measurement );
//        temp.quotes();
        showJson( !temp );                         
        server.send(200, "text/plain", !temp );
    });

    // ====================== VARIABLE SHOW/SET ======================================
    
    server.on("/set", HTTP_GET, 
    [](){
        showArgs();
        BUF resp(128);
        BUF final(128);
            
        if( server.args() )                              // if variable is given
        {
            char cmd[80];
            sprintf( cmd, "uset %s %s", !server.argName(0), !server.arg(0) );

            resp.init();                      
            exe.dispatchBuf( cmd, resp );          // command is executed here and RESPONSE1 is saved in 'resp' buffer

            final.set("<h3 align='center'>\r\n");
            final.add("%s</h3>\r\n", !resp );
            final.add( navigate );
        }
        showJson( !final );                           // show the same on the console
        server.send(200, "text/html", !final );
    });
    server.on("/show", HTTP_GET, 
    [](){
        showArgs();
        BUF s(512);

        s.set("<h3 align='center'>\r\n");

        for( int i=0; i<nmp.getParmCount(); i++ )
            s.add("%s = %s<br/>\r\n", nmp.getParmName(i), nmp.getParmValueStr(i) );

        s.add("<br/>(Use '/set?parm=value' to modify)<br/>");
        s.add("</h3>\r\n");
        s.add( navigate );
        showJson( !s );                              
        server.send(200, "text/html", !s );
    });
}
