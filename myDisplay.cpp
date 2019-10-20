// Minimum necessary for this file to compile

#include <externIO.h>               // IO includes and cpu...exe extern declarations
#include "oledClass.h"
#include "myGlobals.h"
#include "mySupport.h"
#include "myDisplay.h"

OLED oled;

// expand in the future with call to function instead of changing a variable only

#define BUTTON_PRESSED      1
#define BUTTON_RELEASED     2
#define BUTTON_LONGPRESS    3

// ------------------------------- UART SWAP ROutines ---------------------------------------
    
    static int secbaudrate = 0;
    static int pribaudrate = 0;
    //   Primary pins: Tx=GPIO1, Rx=GPIO3 (marked as Tx Rx in NodeMCU)
    // Secondary pins: Tx2/RTS0=GPIO15, Rx2/CTS0=GPIO13 (marked as D8 and D7 in NodeMCU)
    //   Power pin: GPIO12

    void swapSerialSec()            // switch to Tx Rx to 
    {
        PF("Swapping to SEC Serial\r\n");
        if( myp.swapon )                        // already in secondary pins
            return;
        Serial.flush();                     // empty Primary transmit buffer
        
        if( secbaudrate )
        {
            Serial.end();                   // change baud rate
            Serial.begin( secbaudrate );    // change baudrate to Primary
        }
        pinMode( 1, OUTPUT );               // force Tx to high. Does not help to deglitch!
        digitalWrite( 1, HIGH );
        Serial.swap();                      // swap pins
        
        while( Serial.available() )         // empty Secondary receive buffer
        {
            Serial.read();
            yield();
        }
        PF("SEC Serial Active\r\n");
        myp.swapon = true;
    }
    void swapSerialPri()
    {        
        PF("Swapping to PRI Serial\r\n");
        if( !myp.swapon )                        // already in primary pins
            return;
        Serial.flush();                     // empty Secondaary transmit buffer
    
        pinMode( 15, OUTPUT );              // force Tx2 to high. Does not help to deglitch!
        digitalWrite( 15, HIGH );
        Serial.swap();                      // swap pins to primary
         
        if( pribaudrate )
        {
            Serial.end();                   // Stop
            Serial.begin( pribaudrate );    // change baudrate of Primary
        }
        while( Serial.available() )         // empty Primary receive buffer
        {
            Serial.read();
            yield();
        }
        myp.swapon = false;
        PF("PRI Serial Active\r\n");
    }
    void swapSerial( int mode )
    {
        if( mode )
            swapSerialSec();
        else
            swapSerialPri();
    }

// ------------------------------- MENU HANDLERS --------------------------------------

void defineMyMenus( int imenu )
{
    if( imenu==2 )
    {
        menu.resetItems();
        menu.registerMenu( "MENU 1 of 2",   defineMyMenus );
        menu.registerItem( "scan:",         &myp.gp.scan,   3 );
        menu.registerItem( "trace:",        &myp.gp.trace,  3 );
        menu.registerItem( "oledon:",       &myp.oledon,    2 );
        menu.registerItem( "PRI Serial",    swapSerialPri );
        menu.registerItem( "SEC Serial",    swapSerialSec );
        menu.registerItem( "EXIT" );
    }
    if( imenu==1 )
    {
        menu.resetItems();
        menu.registerMenu( "MENU 2 of 2",    defineMyMenus );
        menu.registerItem( "OLED Low",       [](){ brightOLED(1); }  );
        menu.registerItem( "OLED High",      [](){ brightOLED(254); } );
        menu.registerItem( "SaveParms",      [](){ myp.saveMyEEParms(); } );
        menu.registerItem( "InitParms",      [](){ myp.initAllParms(MAGIC_CODE); } );
        menu.registerItem( "EXIT" );        
    }
}
// ------------------------------------- MENU CLASS INSTANCE ----------------------------

    MENU menu;

// ------------------------------------- MENU CLASS DEFINITION --------------------------

    #define ITEM_PVAR(A)  (pitem+A)->u.pvar
    #define ITEM_FUNC(A)  (pitem+A)->u.func
    #define ITEM_FUNV(A)  (pitem+A)->u.funv
    #define ITEM_TEXT(A)  (pitem+A)->text
    #define ITEM_COUNT(A) (int) ((pitem+A)->type)
    #define ITEM_TYPE(A) (pitem+A)->type

    void MENU::init(  )
    {
        mactive  = false;
        nextitem = 0;
        items    = 0;
        pitem    = &item[0];
    }
    void MENU::resetItems()
    {
        nextitem = 0;
        items = 0;
    }
    void MENU::registerItem( char *name )
    {
        if( items > 8 )
            return;
        ITEM_TEXT( items )  = name;
        ITEM_TYPE( items ) = ITEMTYPE_EXIT;
        ITEM_PVAR( items ) = NULL;
        items++;
    }
    void MENU::registerItem( char *name, int *pvalue, int count )
    {
         if( items > 8 )
            return;
        ITEM_TEXT( items )  = name;
        ITEM_TYPE( items )  = (itemtype_t) count;
        ITEM_PVAR( items ) = pvalue;
        items++;
    }
    void MENU::registerItem( char *name, void (*func)(int) )
    {
        if( items > 8 )
            return;

        ITEM_TEXT( items )  = name;
        ITEM_TYPE( items ) = ITEMTYPE_ARGS;
        ITEM_FUNC( items ) = func;
        items++;
    }
    void MENU::registerItem( char *name, void (*func)() )
    {
        if( items > 8 )
            return;

        ITEM_TEXT( items )  = name;
        ITEM_TYPE( items ) = ITEMTYPE_CALL;
        ITEM_FUNV( items ) = func;
        items++;
    }
    void MENU::registerMenu( char *name, void (*func)(int) )
    {
        if( items > 8 )
            return;

        ITEM_TEXT( items )  = name;
        ITEM_TYPE( items ) = ITEMTYPE_MENU;
        ITEM_FUNC( items ) = func;
        items++;
    }
    bool MENU::active()
    {
        return mactive;
    }
    void MENU::process( int key )
    {
        if( key == BUTTON_PRESSED )              // ignore pressing down
            return;
        if( !mactive )
        {
            displayAll();
            mactive = true;
            return;
        }
        if( key==BUTTON_RELEASED )            
        {
            nextItem();                            // point to next item
            displayAll();
        }
        if( key==BUTTON_LONGPRESS )        
        {
            itemtype_t code = ITEM_TYPE( nextitem );
            int v;
            int iarg;
            char *p;
            
            switch( code )
            {                    
                case ITEMTYPE_EXIT:
                    showOLED("INIT");
                    mactive = false;
                    break;
                    
                case ITEMTYPE_CALL:
                    //PF("Calling func with no arg\r\n" );
                    (*ITEM_FUNV(nextitem))( );                      // call the function
                    oled.dspL( nextitem, ">%s OK", ITEM_TEXT(nextitem) );
                    break;
                    
                case ITEMTYPE_ARGS:
                case ITEMTYPE_MENU:
                    p = ITEM_TEXT( nextitem );
                    while( *p )                                     // find first space
                    {
                        if( *p++==' ' )
                            break;
                    }
                    iarg = atoi( p );
                    //PF("Calling func with arg=%d\r\n", iarg );
                    (*ITEM_FUNC(nextitem))( iarg );                  // call the function

                    if( code == ITEMTYPE_ARGS )
                        oled.dspL( nextitem, ">%s OK", ITEM_TEXT(nextitem) );
                    else
                        displayAll();
                    break; 

                default:                                            // greater than 0
                    ASSERT( (int) code >= 0 );
                    v = *ITEM_PVAR(nextitem);
                    v++;
                    if( v >= ITEM_COUNT(nextitem) )
                        v = 0;
                    *ITEM_PVAR(nextitem) = v;
                    displayAll();
                    break;
            }
        }
    }
    void MENU::displayAll()     // displays menu pointing to nextitem
    {
        int i;
        for( i=0; ; i++ )
        {
            int *vp = ITEM_PVAR(i);
            char *p = ITEM_TEXT(i);
            if( ITEM_COUNT(i) <= 0 )
            {
                if( i == nextitem )     
                    oled.dspL( i, ">%s", p );        // PF("> %d: %s\r\n", i, p );        
                else
                    oled.dspL( i, " %s", p );        // PF("  %d: %s\r\n", i, p );
                if( ITEM_COUNT(i) == ITEMTYPE_EXIT )  // this is the end of the list
                    break;
            }
            else
            {
                if( i == nextitem )     
                    oled.dspL( i, ">%s%d", p, *vp ); // PF("> %d: %s%d\r\n", i, p, *vp );        
                else
                    oled.dspL( i, " %s%d",  p, *vp ); // PF("  %d: %s%d\r\n", i, p, *vp );
            }
        }       
        for( int j=i+1; j<8; j++ )                        // remaining of the display with blanks
            oled.dspL( j, " ");                         // PF("  %d:empty\r\n", j ); 
    }
    void MENU::nextItem()           // get the next available item
    {
        if( ITEM_COUNT( nextitem ) == ITEMTYPE_EXIT )
            nextitem = 0;
        else
            nextitem++;
    }
// ------------------------------- OLED ROutines --------------------------------------------
    
    void brightOLED( int value )
    {
        if( value <=0 )
            value = 1;
        myp.gp.oledbr = value;
        oled.setBrightness( myp.gp.oledbr );        
    }
    void showOLED( char *status, int lineno )           // Status="INIT" initializes OLED
    {                                       // Status="CLEAR" clears the display
        if( !strcmp(status,"INIT") )
        {
            oled.init( OLED130 );
            myp.oledon = true;
            oled.setBrightness( myp.gp.oledbr );
            oled.dsp( 0, "\vGKE Sensor GWY" ); 
            return;
        }
        if( !strcmp(status,"CLEAR") )
        {
            oled.clearDisplay();
            myp.oledon = false;
            return;
        }
        if( lineno >= 0 )
        {
            oled.dspL( lineno, status );
        }
        else
        {
            oled.dspL( 0, "\vGKE Sensor GWY" ); 
            static int line=2;
            
            char temp[32];
            sprintf( temp, "\a%s", status );
            oled.dspL( line++, temp );
            if( line > 7 )
                line = 7;
        }
        PF("OLED: %s\r\n", status );
        delay(1000);
        
    }
    static char *spinner()
    {
        static int count;
        switch( (count++) & 3 )
        {
            case 0: return "\a-";
            case 1: return "\a\\";
            case 2: return "\a|";
            case 3: return "\a/";
        }
    }
    void updateOLED( )
    {
        int i = sens.getIndex();
        oled.dspC( 1,  "\a " );
       
        if( F_ERROR(i) )                        // if a fetch() error...
        {
            oled.dspC( 0,  "\aS%d IP:%d", i, myp.gp.ip[i] );
            oled.dspC( 2,  "\vFetchEr:%d", F_ERROR(i) );
            oled.dspC( 4,  "\vDelay:%dms", F_DELAY(i) );            
        }
        else                                    // fetch() is done correctly
        {            
            if( P_ERROR(i) )                    // if a parsing error...
            {
                oled.dspC( 0,  "\aS%d IP:%d", i, myp.gp.ip[i] );
                oled.dspC( 2,  "\vParseEr:%d", P_ERROR(i) );
                oled.dspC( 4,  "\vDelay:%dms", F_DELAY(i) );  
            }
            else                                // no parsing errors...
            {
                oled.dspC( 0,  "\aS%d:%du%d %s", i, myp.gp.ip[i], myp.swapon?1:0, S_LABEL(i) );
                                
                switch( S_TYPE(i) )             // depanding on sensor type, show results
                {
                    case OWN_DS18x2:    oled.dspC( 2, "\b%.1f'F", S_TEMP1(i) );
                                        oled.dspC( 4, "\b%.1f'F", S_TEMP2(i) );
                                        break;
                    default:
                    case OWN_DS18:      oled.dspC( 2, "\b.1f'F", S_TEMP1(i) );
                                        oled.dspC( 4, "\b " );
                                        break;
                    case OWN_HTU:
                    case OWN_DHT:       oled.dspC( 2, "\b%.1f'F", S_TEMP1(i) );
                                        oled.dspC( 4, "\b%.0f%%", S_HUMID(i) );
                                        break;
                }                
            }
        }
        IPAddress ip = WiFi.localIP();
        oled.dspC( 7,"\a%s", ip.toString().c_str() );  
    }
