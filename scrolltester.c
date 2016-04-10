#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <curses.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

void scrollHandler();
void scrollFile();
void scrollLines(int lines);
void refreshTimer();
void refreshScreen();
void printScreen();
void clearScreen();
void echoOff();
void echoOn();

//Let's define global variables
int termHeight;
int termWidth;
int currentLine;
int totalLines;
//Boolean to see if we are scrolling? 0 is false 1 is true
int isScrolling;
double scrollSpeed;

//The file to read from
FILE * input;

struct termios originalInfo;
struct itimerval timer;

//Quit the program
void quitScroll()
{
    echoOff();
    echoOn();
    clearScreen();
    //close the file
    fclose(input);
    exit(0);
}
/*kill signal handler*/
void sigHandler(int sigNum)
{
    quitScroll();
}
void scrollHandler()
{
    if(isScrolling)
    {
        echoOn();
        scrollLines(1);
        refreshScreen();
        fflush(stdout);
        echoOff();
    }
}
void installHandlers()
{
    
    /* Install timer_handler as the signal handler for SIGALRM. */
    if (signal(SIGALRM, (void (*)(int)) scrollHandler) == SIG_ERR) {
        perror("Unable to catch SIGALRM");
        quitScroll();
    }
    signal(SIGINT, sigHandler);
}
int main(int argc, char ** argv)
{
    //Get the terminal height and width
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    termHeight = w.ws_row-1;
    termWidth = w.ws_col;
    
    //Set up the terminal so that long lines truncate as opposed to wrapping
    fputs("\033[?7l", stdout);
    
    //Catch the all signals
    installHandlers();
    
    //Set the default scroll speed to 2 seconds (2000ms) per line
    scrollSpeed = 2000.0;
    refreshTimer();
    totalLines = 400;
    
    //Handle the error that the file does not exist
    if(argc == 2)
    {
        printf("Opening File '%s'\n", argv[1]);
        //Open the file and handle the error that the file does not exist
        input = fopen(argv[1], "r");
        if(input != NULL)
        {
            scrollFile(input);
        }
        else
        {
            perror("File does not exist!\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (argc == 1)
    {
        //if we just type the command, assume we're reading from stdin
        printf("Reading from standard input\n");
        //Open the file and handle the error that the file does not exist
        input = stdin;
        scrollFile(input);
    }
    else
    {
        perror("Too many/too few arguments!\n");
        exit(EXIT_FAILURE);
    }
    
}

void scrollFile()
{
    //First, clear the screen
    clearScreen();
    //Main program logic
    while(1)
    {
        refreshScreen();
        echoOff();
        int c;
        c = getchar();
        if(c == 'q')
        {
            //Quit the program
            quitScroll();
        }
        else if (c == ' ')
        {
            //Skip to the next screenful
            //termHeight for screenful
            scrollLines(termHeight);
        }
        else if(c == '\n')
        {
            //toggle auto scrolling
            isScrolling = !isScrolling;
        }
        else if (c == 's')
        {
            //Slower by 20%
            scrollSpeed *= 1.25;
            refreshTimer();
        }
        else if (c == 'f')
        {
            scrollSpeed *= .8;
            refreshTimer();
        }
        echoOn();
    }
}
void scrollLines (int amount)
{
    currentLine += amount;
    if(currentLine > totalLines-termHeight)
    {
        if(totalLines - termHeight < 0)
        {
            currentLine = 0;
        }
        else
        {
            currentLine = totalLines-termHeight;
        }
    }
}
void refreshTimer()
{
    //Set the timer to the scrollTime
    if(scrollSpeed > 1000)
    {
        timer.it_value.tv_sec = scrollSpeed/1000;
        timer.it_value.tv_usec = 0;
    }
    else
    {
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = (int)scrollSpeed*1000;
    }
    
    timer.it_interval = timer.it_value;
    /* Start a virtual timer. It counts down whenever this process is
     executing. */
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("error calling setitimer()");
        quitScroll();
    }
}
void refreshScreen()
{
    clearScreen();
    printScreen();
}
void printScreen()
{
    //Write all the lines to the screen
    /*unsigned int i;
     for(i = 0; i < termHeight; i++)
     {
     //old for testing
     //printf("Testing Full Screen %d\n", i + currentLine);
     int actualLine;
     actualLine = i+currentLine;
     }*/
    int count = 0;
    int lines = 0;
     char line[256]; /* or other suitable maximum line size */
    while (fgets(line, sizeof line, input) != NULL) /* read a line */
    {
        if (count >= currentLine && count < currentLine+termHeight)
        {
            //print the line
            printf("%s", line);
            lines++;
        }
        count++;
    }
    //reset the file
    rewind(input);
    while(lines < termHeight)
    {
        printf("\n");
        lines++;
    }
    
    //And now, print out the program controls
    //Reverse video for pretty formatting
    fputs("\033[7m", stdout);
    printf("Controls: SPACE - Page down, ENTER - Toggle auto-scroll, f - Faster 20%%, s - Slower 20%%, q - Quit, Scroll speed = %fms, autoScroll = %d", scrollSpeed, isScrolling);
    fputs("\033[0m", stdout);
    fflush(stdout);
}
void clearScreen()
{
    //Clear the screen
    fputs("\033[2J",stdout);
    //Move the cursor to the top left
    fputs("\033[1;1H",stdout);
}
void echoOff()
{
    struct termios info;
    tcgetattr(0,&info);    //get attribs
    info.c_lflag &= ~(ICANON | ECHO) ;    //turn off echo bit
    tcsetattr(0,TCSANOW,&info);   //set attribs
    
}
void echoOn()
{
    
    struct termios info;
    tcgetattr(0,&info);    //get attribs
    info.c_lflag |= (ICANON | ECHO) ;    //turn off echo bit
    tcsetattr(0,TCSANOW,&info);   //set attribs
}
