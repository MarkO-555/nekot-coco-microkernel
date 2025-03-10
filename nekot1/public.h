#ifndef _g_PUBLIC_H_
#define _g_PUBLIC_H_

// Proposed "g" API for Nekot Gaming OS.

// In the following, remember that "Game" is what we call the
// user programs or processes or applications or apps that this OS can run.

/////////////////////
//
//  Fundamental Types and Definitions.

typedef unsigned char bool;
typedef unsigned char byte;  // Best type for a 8-bit machine byte.
typedef unsigned int word;  // Best type for a 16-bit machine word.
typedef void (*func)(void);
typedef union wordorbytes {
    word w;
    byte b[2];
} wob;

#define true ((bool)1)
#define false ((bool)0)
#define NULL ((void*)0)

#ifndef CONST
#define CONST const   // For variables the Kernel changes, but Games must not.
#endif

#define Peek1(ADDR) (*(volatile byte*)(word)(ADDR))
#define Poke1(ADDR,VALUE) (*(volatile byte*)(word)(ADDR) = (byte)(VALUE))

#define Peek2(ADDR) (*(volatile word*)(word)(ADDR))
#define Poke2(ADDR,VALUE) (*(volatile word*)(word)(ADDR) = (word)(VALUE))

// These do a Peek1, some bit manipulaton, and a Poke1.
#define PAND(ADDR, X) ((*(volatile byte*)(word)(ADDR)) &= (byte)(X))
#define POR(ADDR, X) ((*(volatile byte*)(word)(ADDR)) |= (byte)(X))
#define PXOR(ADDR, X) ((*(volatile byte*)(word)(ADDR)) ^= (byte)(X))

// If your ".bss" allocation of 128 bytes in Page 0 (the direct page)
// fills up, you can mark some of the global variable definitions with
// this attribute, to move those variables into a larger section.
#define MORE_DATA      __attribute__ ((section (".data.more")))

#define assert(COND) if (!(COND)) Fatal(__FILE__, __LINE__)

void Fatal(const char* s, word value);

#define INHIBIT_IRQ() asm volatile("  orcc #$10")
#define ALLOW_IRQ()   asm volatile("  andcc #^$10")
byte gIrqSaveAndDisable();
void gIrqRestore(byte cc_value);

////////////////////////
//  Pre-allocation

// Use these at the top of your main .c file
// to carve screens and regions out of high memory.
//
// Pre-allocating defines addresses for these
// before the compiler runs, so the compiler can
// generate more optimized code.
//
// Programs that start with some of these elements
// in common may chain values in memory from one
// to the other, for as many as are specified the same.

#ifndef g_DEFINE_SCREEN
#define g_DEFINE_SCREEN(Name,NumPages)       extern byte Name[NumPages*256];
#endif

#ifndef g_DEFINE_REGION
#define g_DEFINE_REGION(Type,Name)    extern Type Name;
#endif

// Example:
//
// g_DEFINE_SCREEN(T, 2);   // T for Text, needs 2 pages (512 bytes).
// g_DEFINE_SCREEN(G, 12);  // G for PMode1 Graphics, needs 12 pages (3K bytes).
// g_DEFINE_REGION(struct common, Common, 44);  // Common to all levels.
// g_DEFINE_REGION(struct maze, Maze, 106);     // Common to maze levels.

////////////////////////
//
//  64-byte Chunks

// gAlloc64 allocate a 64 byte Chunk of memory.
// Succeeds or returns NULL.
byte* gAlloc64();

// gFree64 frees a 64 byte Chunk that was allocated with gAlloc64().
// If ptr is NULL, this function returns without doing anything.
void gFree64(byte* ptr);

/////////////////////
//
//  Networking

// In the following, messages can be 1 to 64 bytes long.
// The very first byte is reserved by the OS, and all the rest
// are available to the game.  The OS fills in the
// player number of the sender in the first byte.
// The operating system will call gFree64 when it
// has been sent.

// gSend64 attempts to send a "multicast" message of 1 to 64 bytes
// to every active player in your game shard.
// It succeeds or it calls Fatal().
void gSend64(byte* ptr, byte size);

// gReceive64 attempts to receive a "multicast" message sent by
// anyone in your game shard, including your own,
// that were set with gSend().  If no message has
// been received, the NULL pointer is returned.
// If you need to know the length of the received
// message, that needs to be sent in the "fixed"
// portion at the front of the message, perhaps as
// the second byte.  You should call gFree() on the
// chunk when you are done with it.
byte* gReceive64();

// If you can view the MCP logs, you can log to them.
// Don't log much!
void gNetworkLog(const char* s);

///////////////
//
//  Video Mode

// gGameShowsTextScreen sets the VDG screen mode for game play to a Text Mode.
void gGameShowsTextScreen(byte* screen_addr, byte colorset);

// gGameShowsPMode1Screen sets the VDG screen mode for game play to PMode1 graphics.
void gGameShowsPMode1Screen(byte* screen_addr, byte colorset);

// gModeForGame sets the VDG screen mode for game play to the given mode_code.
// TODO: document mode_code.
void gModeForGame(byte* screen_addr, word mode_code);

// GetConsoleTextModeAddress return the starting address
// of the kernel's Text Console, which is always 512 bytes,
// in the ordinary VDG Text mode.  The first 32 bytes
// are the "top bar" that can might be useful for
// realtime debugging markings.  Production games
// shouldn't need this.
inline byte* GetConsoleTextModeAddress() {
    return (byte*)0x0200;
}

/////////////////////
//
//  Scoring

// g_MAX_PLAYERS is the maximum number of active players
// in a single game shard.
#define g_MAX_PLAYERS 8

// gNumberOfPlayers is the current number of
// active players in the game.
extern CONST byte gNumberOfPlayers;

// YgThisPlayerNumber tells you your player number
// (from 0 to g_MAX_PLAYERS-1) if you are active in the game.
// If you are just a viewer, you are not an active player,
// and this variable will be 255.
extern CONST byte gThisPlayerNumber;

// You change these to add or deduct points to a player.
extern int gPartialScores[g_MAX_PLAYERS];

// Read Only, set by the OS, the sum of all Partial Scores.
extern CONST int gTotalScores[g_MAX_PLAYERS];

/////////////////////
//
//  Kern Module

// Do not call gSendClientPacket directly.
// It is used by the next few calls.
void gSendClientPacket(word p, char* pay, word size);

// Normal end of game.  Scores are valid and may be published
// by the kernel.
#define gGameOver(WHY)  gSendClientPacket('o', (byte*)(WHY), 64)

// Abnormal end of game.  Scores are invalid and will be ignored
// by the kernel.
// This is less drastic than calling Fatal(),
// because the kernel keeps running, but it also
// indicates something went wrong that should be fixed.
#define gGameAbort(WHY)  gSendClientPacket('a', (byte*)(WHY), 64)

// Replace the current game with the named game.
// This can be used to write different "levels" or
// interstitial screens as a chain of games.
// Carry scores forward to the new game.
// Common pre-allocated regions are also kept in memory.
#define gGameChain(NEXT_GAME_NAME)  gSendClientPacket('c', (byte*)(NEXT_GAME_NAME), 64)

// gBeginMain must be called at the beginning of your main()
// function.
#define gBeginMain()                           \
        { asm volatile(".globl __n1pre_entry");   \
        Poke2(0, &_n1pre_entry); }

// Sometimes if you are using inline assembly language
// inside a function, you need to tell GCC that the function
// is needed even if it isn't explicitly called.
// gPin(f) will pin down function f, so GCC doesn't ignore it.
#define gPin(THING)  Poke2(0, &(THING))

// gAfterMain does not end the game -- it just ends the startup code,
// and frees the startup code in memory, making that memory available.
//
// If you call this at the end of your main function,
// it will exit the main function and enter the function f.
// The idea is that you put all your startup initialization
// code in main, and the memory for the startup code is
// freed up when the startup is done.  f points to the
// function where the non-startup code continues.
#define gAfterMain(after_main) gAfterMain3((after_main), &_n1pre_final, &_n1pre_final_startup)
void gAfterMain3(func after_main, word* final, word* final_startup);

// Global variables or data tables that are only used
// by startup code can be marked with the attribute
// STARTUP_DATA.  They will be freed when you call
// gAfterMain().
#define STARTUP_DATA   __attribute__ ((section (".data.startup")))

// The following Kern variables can be read by the game
// to find out what state the Kernel is in.
//
// A game should always see `in_game` is true!
// A game should always see `in_irq` is false!
//
// The important one is `focus_game`:  If `focus_game`
// is true, the game can scan the keyboard (but it must
// disable interrupts while doing so).
//
// If the game has an infinite loop (say, as the
// outer game loop) it is better to use
//
//     while (Kern.always_true) { ... }
//
// rather than
//
//     while (true) { ... }
//
// due to bugs in GCC.

extern CONST struct kern {
    // For GCC bug workaround.  Must always be true.
    bool volatile always_true;

    // A game is active and has the foreground task.
    bool volatile in_game;

    // We are currently handling a 60Hz Clock IRQ.
    bool volatile in_irq;

    // The active game also owns and can scan the keyboard
    // (except for the BREAK key), and the game's screen
    // is being shown.   If a game is active but
    // focus_game is false, the game must ignore the
    // keyboard (not scan it!) and the game's screen is
    // not visible -- the Chat screen is shown, instead.
    bool volatile focus_game;
} Kern;

/////////////////////
// Real Time

// Your game may watch these variables, and when
// they change, you can do some action that you want to
// do 60x, 10x, or 1x per second.  These variables
// will not match those on other cocos.  They are reset
// only when your coco boots, and as long as IRQs
// are never disabled for too long, they should
// reliably increment.  After about 18.2 hours,
// seconds wraps back to zero.

extern CONST struct real {
    byte volatile ticks;  // Changes at 60Hz:  0 to 5
    byte volatile decis;  // Tenths of a second: 0 to 9
    word volatile seconds;  // 0 to 65535
} gReal;  // Instance is named Real.

////////////////////////////
//
//  Wall Time

// If your game wants to display the date or time, it can use these
// variables.  These may not be accurate to the split second, but hopefully
// within a second or two.  It is possible that this time changes, even
// backwards, as it might be corrected asynchronously, and there is also
// no indication of daylight/summer time.  These are not problems we're
// trying to solve.

extern CONST struct wall {
    byte volatile second; // 0 to 59
    byte volatile minute; // 0 to 59
    byte volatile hour;  // 0 to 23

    byte volatile day;   // 1 to 31
    byte volatile month; // 1 to 12
    byte volatile year2000;  // e.g. 25 means 2025
    byte volatile dow[4];  // e.g. Mon\0
    byte volatile moy[4];  // e.g. Jan\0

    // If hour rolls over from 23 to 0,
    // these values are copied to day, month, etc.
    // These are preset by the server, so the kernel does not
    // need to understand the Gregorian Calendar.
    byte next_day;
    byte next_month;
    byte next_year2000;
    byte next_dow[4];
    byte next_moy[4];
} gWall; // Instance is named Wall.

#endif // _g_PUBLIC_H_
