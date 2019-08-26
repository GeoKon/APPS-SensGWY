/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
 
 // --------------------- Add in this file CLI handlers -------------------------

#include "nmpClass.h"
#include "myGlobals.h"

// ------------------- allocation of base classes ------------------------------
    
    Global  myp;                    // Allocation of the Global parameters
    NMP     nmp;                    // Allocation of Named Parms needed by this module   

    // Labels used for sensor types
    char *senstype[]={"Disabled", "RadioStat", "DS18", "DS18x2", "DHT", "HTU"};
    
    //struct rdt_t rd[ SCAN_COUNT ]; 
    //struct rpc_t rpc;
 
