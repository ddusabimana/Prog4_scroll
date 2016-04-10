/*
 Authors: Daniel Dusabimana and Samuel capotosto
 */
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
//.
//.
//.Hash Defines
//.
#define TRUE				1					//.True
#define FALSE				0					//.False
#define FILE_BUFF_SZ		(8 * 1024 * 1024)	//.8 MB
#define MAX_LINE_SZ			(512)				//.512 Characters
#define STR_TTY_WRP_OFF		"\033[?7l"			//.
#define STR_TTY_RV_ON		"\033[7m"			//.Reverse Video ON
#define STR_TTY_RV_OFF		"\033[0m"			//.Reverse Video OFF
#define STR_TTY_CLR_SC		"\033[2J"			//.Clear Screen
#define STR_TTY_RST_CRSR	"\033[1;1H"			//.Reset Cursor to Top-Left
#define STR_TTY_DEL_EOL		"\033[0K"			//.Delete to E-O-L
#define STR_TTY_BOLD_ON		"\033[1m"			//.Turn ON Bold
#define STR_TTY_UNDL_ON		"\033[4m"			//.Turn ON UnderLine
#define STR_TTY_FRMT_OFF	"\033[0m"			//.Turn OFF any Formatting

//.Function Prototypes

void echoOn();
void echoOff();
void scrollFile();
void clearScreen();
void printPrompt();
void erasePrompt();
void refreshTimer();
void printTestStr();
void scrollHandler();
void sigHandler(int);
void scrollLines(int);
void exitProgram(int);
void printAndScroll();
void installHandlers();
void printAndScrollLine();
void ttyControl(const char *);
void printDebugStr(const char *);

// Global Variables

int termWidth;
int totalLines;
int termHeight;
int bytsInFile;
int eofReached;
int currentLine;
int isScrolling; //.Boolean for Auto-Scroll state
int bytsPrinted;
int linesToScroll;
double scrollSpeed;
struct itimerval timer;
struct termios originalInfo;
char * pBuffPtr;
char * pFileBuff;
FILE * input;   //.The file to read from




int main(int argc, char ** argv)
{
    struct winsize w;
    //.
    eofReached = 0;
    bytsInFile = 0;
    bytsPrinted = 0;
    totalLines = 400;
    scrollSpeed = 2000.0; //.Set the default Auto-Scroll speed to 2 seconds per line
    ioctl(0, TIOCGWINSZ, &w); //.Get the terminal height & width
    termHeight = w.ws_row - 1;
    if(w.ws_col > MAX_LINE_SZ) {
        termWidth = MAX_LINE_SZ;
    }
    else {
        termWidth = w.ws_col;
    }
    ttyControl(STR_TTY_WRP_OFF); //.Set up the terminal so that long lines truncate as opposed to wrapping
    //.
    //.printTestStr();
    //.exitProgram(0);
    //.
    installHandlers(); //.Catch the signals
    refreshTimer();
    if(argc == 2) { //.Check number of arguments
        printf("\nOpening File '%s'\n", argv[1]);
        if((input = fopen(argv[1], "r")) != NULL) {	//.Try opening input file
            scrollFile(input);
        }
        else {
            perror("\nError : Input file error !\n");
            exitProgram(EXIT_FAILURE);
        }
    }
    else if(argc == 1) { //.If we just type the command, assume we're reading from stdin
        printf("\nReading from standard input\n");
        input = stdin; //.Open the file & handle the error that the file does not exist
        scrollFile(input);
    }
    else {
        perror("\nIncorrect number of arguments!\n");
        exitProgram(EXIT_FAILURE);
    }
    exitProgram(0);
}




void scrollFile()
{
    char * pPtr;
    int c, count, lines;
    //.
    c = count = lines = 0;
    if((pFileBuff = malloc(FILE_BUFF_SZ * sizeof(char))) == NULL) {
        printf("\nError : While allocating memory for file buffer'\n");
        exit(EXIT_FAILURE);
    }
    memset(pFileBuff, 0x0, FILE_BUFF_SZ);
    bytsInFile = (int)fread(pFileBuff, sizeof(char), FILE_BUFF_SZ, input);
    pBuffPtr = pPtr = pFileBuff;
    for(count = 0; count < bytsInFile; count++) {
        if(*pPtr == '\n') {
            lines++;
        }
        pPtr++;
    }
    totalLines = lines;
    scrollLines(termHeight);
   
    
    
    while(TRUE) {  //.Main program logic
        printAndScroll(); //.Print screen-full
        echoOff();
        switch(c = getchar()) {
            case ' ': //.Skip to next screenful
                scrollLines(termHeight);
                break;
            case '\n': //.Toggle Auto-Scroll
                scrollLines(1);
                isScrolling = !isScrolling;
                break;
            case 'j': //.Quit
            case 'J':
                scrollLines(1);
                break;
            case 'q': //.Quit
            case 'Q':
                exitProgram(0);
                break;
            case 'f': //.Speed-Up by 20 %
            case 'F':
                scrollSpeed -= (scrollSpeed * 0.2);
                if(scrollSpeed < 0.0) {
                    scrollSpeed = 0.0;
                }
                refreshTimer();
                break;
            case 's': //.Slow by 20 %
            case 'S':
                scrollSpeed += (scrollSpeed * 0.2);
                refreshTimer();
        }
        echoOn();
    }
}


void scrollLines(int amount)
{
    linesToScroll = amount;
}


void printAndScroll()
{
    int nX;
    //.
    if(eofReached) {
        erasePrompt();
        printPrompt();
        return;
    }
    erasePrompt();
    for(nX = 0; nX < linesToScroll; nX++) {
        printAndScrollLine();
    }
    printPrompt();
    fflush(stdout);
}


void printAndScrollLine()
{
    int nX, nY, nZ;
    char strLine[MAX_LINE_SZ];
    char * pX, *pY, * pLinePtr;
    //.
    if(eofReached) {
        erasePrompt();
        printPrompt();
        return;
    }
    nX = nY = nZ = 0;
    nY = termWidth - 1;
    pLinePtr = strLine;
    memset(strLine, 0x0, sizeof(strLine));
    while(nX < nY) {
        *pLinePtr = *pBuffPtr;
        if(*pBuffPtr == '\n') {
            pBuffPtr++;
            pLinePtr++;
            bytsPrinted++;
            nX++;
            break;
        }
        if(*pBuffPtr == '\b') { //.Extra-Credit
            pX = pBuffPtr - 1;
            pY = pBuffPtr + 1;
            if(*pX == *pY) { //.Bold
                pBuffPtr++;
                bytsPrinted++;
                pLinePtr--;
                strcpy(pLinePtr, STR_TTY_BOLD_ON);
                pLinePtr += 4;
                *pLinePtr = *pY;
                pLinePtr++;
                strcpy(pLinePtr, STR_TTY_FRMT_OFF);
                pLinePtr += 3;
                nX -= 3;
            }
            if(*pX == '_') { //.Underline
                pBuffPtr++;
                bytsPrinted++;
                pLinePtr--;
                strcpy(pLinePtr, STR_TTY_UNDL_ON);
                pLinePtr += 4;
                *pLinePtr = *pY;
                pLinePtr++;
                strcpy(pLinePtr, STR_TTY_FRMT_OFF);
                pLinePtr += 3;
                nX -= 3;
            }
        }
        pBuffPtr++;
        pLinePtr++;
        bytsPrinted++;
        nX++;
    }
    if(bytsPrinted >= bytsInFile) {
        eofReached = 1;
        if(isScrolling) {
            isScrolling = FALSE;
        }
    }
    strLine[nY] = '\n';
    fputs(strLine, stdout);
    fflush(stdout);
}


void printPrompt()
{
    int nX, nY;
    char strPrompt[512];
    char strScrollState[8];
    char strScrollSpeed[16];
    //.
    nX = nY = 0;
    memset(strPrompt, 0x0, sizeof(strPrompt));
    memset(strScrollState, 0x0, sizeof(strScrollState));
    memset(strScrollSpeed, 0x0, sizeof(strScrollSpeed));
    if(isScrolling) {
        sprintf(strScrollSpeed, "%.1f Secs", (scrollSpeed / 1000.0));
        strcpy(strScrollState, "Yes");
        sprintf(strPrompt, "Q:Exit  Space:PgDn  J:LnDn  Entr:ToggleAS  F:Fstr  S:Slwr  AutoScrl:%s-%s", strScrollState, strScrollSpeed);
    }
    else {
        strcpy(strScrollState, "No");
        sprintf(strPrompt, "Q:Exit  Space:PgDn  J:LnDn  Entr:ToggleAS  F:Fstr  S:Slwr  AutoScrl:%s", strScrollState);
    }
    if(eofReached) {
        strcat(strPrompt, " *EOF*");
    }
    ttyControl(STR_TTY_RV_ON); //.Print out the program controls in Reverse-Video
    nX = (int)(strlen(strPrompt));
    nY = termWidth - nX;
    if(nY > 0) {
        for(nX = 0; nX < nY; nX++) {
            strcat(strPrompt, " ");
        }
    }
    fputs(strPrompt, stdout);
    ttyControl(STR_TTY_RV_OFF);
    fflush(stdout);
}


void erasePrompt()
{
    int nX;
    //.
    for(nX = 0; nX < termWidth; nX++) {
        fputs("\b", stdout);
    }
    ttyControl(STR_TTY_DEL_EOL);
    fflush(stdout);
}


void ttyControl(const char * strCode)
{
    fputs(strCode, stdout);
    fflush(stdout);
}


void clearScreen()
{
    ttyControl(STR_TTY_CLR_SC);   //.Clear screen
    ttyControl(STR_TTY_RST_CRSR); //.Move the cursor to the top left
}


void installHandlers() //.Install timer_handler as the signal handler for SIGALRM
{
    if(signal(SIGALRM, scrollHandler) == SIG_ERR) { //.(void (*)(int))
        perror("\nError : Unable to catch SIGALRM !\n");
        exitProgram(EXIT_FAILURE);
    }
    signal(SIGINT, sigHandler);
}


void refreshTimer()
{
    int lSecs;
    int lMuSecs;
    //.
    lSecs = scrollSpeed / 1000;
    lMuSecs = (scrollSpeed - (lSecs * 1000)) * 1000;
    timer.it_value.tv_sec = lSecs;
    timer.it_value.tv_usec = lMuSecs;
    timer.it_interval = timer.it_value;
    if(setitimer(ITIMER_REAL, &timer, NULL) == -1) { //.Start a virtual timer which counts down whenever this process is executing
        perror("Error : While calling setitimer()");
        exitProgram(EXIT_FAILURE);
    }
}


void echoOff()
{
    struct termios info;
    //.
    tcgetattr(0, &info);				//.get attribs
    info.c_lflag &= ~(ICANON | ECHO);	//.turn off echo bit
    tcsetattr(0, TCSANOW, &info);		//.set attribs
}


void echoOn()
{
    struct termios info;
    //.
    tcgetattr(0, &info);				//.Get TTY Attribs
    info.c_lflag |= (ICANON | ECHO);	//.Turn off Echo bit
    tcsetattr(0, TCSANOW, &info);		//.Set TTY Attribs
}


void sigHandler(int sigNum) //.Kill Signal Handlers
{
    exitProgram(EXIT_FAILURE);
}


void scrollHandler()
{
    if(isScrolling) {
        echoOn();
        scrollLines(1);
        printAndScroll();
        fflush(stdout);
        echoOff();
    }
}


void exitProgram(int retCode) //.Quit the program
{
    fclose(input);
    echoOff();
    if(pFileBuff) {
        free(pFileBuff);
    }
    echoOn();
    //.clearScreen();
    erasePrompt();
    printf("\n");
    exit(retCode);
}

/*
void printDebugStr(const char * strDebug)
{
    int nX;
    char strLen[16];
    char strDebugMsg[MAX_LINE_SZ];
    
    //nX = strlen(strDebug);
    if(nX < (MAX_LINE_SZ - 16)) {
        memset(strLen, 0x0, sizeof(strLen));
        sprintf(strLen, "%d", nX);
        memset(strDebugMsg, 0x0, sizeof(strDebugMsg));
        strcat(strDebugMsg, "[");
        strcat(strDebugMsg, strLen);
        strcat(strDebugMsg, ":");
        strcat(strDebugMsg, strDebug);
        strcat(strDebugMsg, "]");
        fputs(strDebugMsg, stdout);
        fflush(stdout);
    }
}


void printTestStr()
{
    int nX;
    char strLen[16];
    char strDebugMsg[MAX_LINE_SZ];
    memset(strLen, 0x0, sizeof(strLen));
    memset(strDebugMsg, 0x0, sizeof(strDebugMsg));
    strcat(strDebugMsg, "[");
    strcat(strDebugMsg, STR_TTY_BOLD_ON);
    strcat(strDebugMsg, "B");
    strcat(strDebugMsg, STR_TTY_FRMT_OFF);
    strcat(strDebugMsg, STR_TTY_BOLD_ON);
    strcat(strDebugMsg, "O");
    strcat(strDebugMsg, STR_TTY_FRMT_OFF);
    strcat(strDebugMsg, STR_TTY_BOLD_ON);
    strcat(strDebugMsg, "L");
    strcat(strDebugMsg, STR_TTY_FRMT_OFF);
    strcat(strDebugMsg, STR_TTY_BOLD_ON);
    strcat(strDebugMsg, "D");
    strcat(strDebugMsg, STR_TTY_FRMT_OFF);
    strcat(strDebugMsg, "]");
   // nX = strlen(strDebugMsg);
    sprintf(strLen, "%d", nX);
    strcat(strDebugMsg, " - (");
    strcat(strDebugMsg, strLen);
    strcat(strDebugMsg, ")");
    fputs(strDebugMsg, stdout);
    fflush(stdout);
}
*/