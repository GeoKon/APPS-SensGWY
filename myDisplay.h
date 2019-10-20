#pragma once

    void swapSerial( int );
    
void showOLED( char *status, int lineno = -1 );
void brightOLED( int value );
void updateOLED( );

void defineMyMenus( int imenu );

// --------------------------- MENU CLASS DECLARATION --------------------------------------

enum itemtype_t
{
    ITEMTYPE_MENU   = -4,         // treated exactly as ITEMTYPE_CALL but refeshes the display
    ITEMTYPE_EXIT   = -1,         // exits menus
    ITEMTYPE_ARGS   = -2,         // calls a function
    ITEMTYPE_CALL   = -3,         // calls a function
};
typedef struct item_t
{
    char *text;
    union
    {
        int *pvar;              // >0
        void (*func)( int );    // ITEMTYPE_ARGS
        void (*funv)( );        // ITEMTYPE_CALL
    } u;
    itemtype_t type;    

} ITEM;

class MENU
{
 private:
    int items;
    bool mactive;               // menu is active
    int nextitem;
    ITEM item[8];
    ITEM *pitem;

    void displayAll();
    void nextItem();

 public:
    void init();
    void resetItems();
    void registerMenu( char *name, void (*func)(int) );
    void registerItem( char *name, int *pvalue, int count );
    void registerItem( char *name, void (*func)(int) );
    void registerItem( char *name, void (*func)() );
    void registerItem( char *name );
    
    bool active();
    void process( int k );

 };

 extern MENU menu;
