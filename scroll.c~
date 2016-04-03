//
//  main.c
//  cos350_Prog4
//  Created by Daniel Dusabimana on 4/1/16.
//  Copyright (c) 2016 danieldusabimana. All rights reserved.
//
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
//.
#define FILE_BUFF_SZ (8 * 1024 * 1024) //.8 MB Buffer for File I/O
//.
//.
//.
int clearPrompt(int);
int togglesttyraw(int);
int printPrompt(char *, char *, int);
//.
//.
//.
int main(int argc, char ** argv)
{
  FILE * fpInput;
  FILE * fpOutput;
  char * pBuff;
  char * strInput;
  char * strPrompt;
  char * buffFileIn;
  char * strPromptFull;
  int nW, nX, nY, nZ;
  int nRet, nRows, nRowsX, nCols, nByts;
  struct winsize win_size;
  //.
  nRows = 25;
  nCols = 80;
  nW = nX = nY = nZ = nRet = nByts = 0;
  //.
  //.-------------------------------------------------------------------------
  //.
  if((nRet = ioctl(1, TIOCGWINSZ, &win_size)) != 0) {
    printf("\ncall to ioctl failed : %d [ %s ]", errno, strerror(errno));
    return(0);
  }
  else {
    nRows = win_size.ws_row;
    nCols = win_size.ws_col;
    printf("\nRows : %d, Cols : %d\n", nRows, nCols);
        
  }
  //.
  if(argc > 1) {
    strInput = argv[1];
    if((fpInput = fopen(argv[1], "r")) == NULL) {
      printf("\nmymore - could not open input file : %s", argv[1]);
      printf("\nErrNum : %d : [ %s ]\n", errno, strerror(errno));
    }
  }
  else {
    fpInput = stdin;
  }
  //.
  fpOutput = stdout;
  nRowsX = nRows - 1;
  strPrompt = (char *)malloc(nCols * sizeof(char));
  memset(strPrompt, 0x00, nCols);
  strPromptFull = (char *)malloc(nCols * sizeof(char));
  memset(strPromptFull, 0x00, nCols);
  //.
  //.-------------------------------------------------------------------------
  //.
  strcpy(strPrompt, "my-more : ");
  strcat(strPrompt, argv[1]);
  buffFileIn = (char *)malloc(FILE_BUFF_SZ * sizeof(char));
  memset(buffFileIn, 0x00, FILE_BUFF_SZ);
  nByts = (int)fread(buffFileIn, sizeof(char), FILE_BUFF_SZ, fpInput);
  pBuff = buffFileIn;
  while((pBuff - buffFileIn) < nByts) {
    for(nX = 0; nX < nRowsX; nX++) {
      for(nY = 0; nY < nCols; nY++) {
	fwrite(pBuff, sizeof(char), 1, fpOutput);
	if(*pBuff == '\n') {
	  pBuff++;
	  break;
	}
	pBuff++;
      }
    }
    printPrompt(strPrompt, strPromptFull, nCols);
    //.togglesttyraw(1);
    nRet = getchar();
    //.togglesttyraw(0);
    clearPrompt(nRet);
    if(nRet == ' ') {
      //.Scroll Forward by a Page
    }
    else if(nRet == '\n') {
      //.
      //.Toggle Auto-Scroll On-or-Off
      //.
    }
  }
  //.
  //.-------------------------------------------------------------------------
  //.
  if(fpInput) {
    fclose(fpInput);
  }
  if(fpOutput) {
    fclose(fpOutput);
  }
  if(buffFileIn) {
    free(buffFileIn);
  }
  if(strPrompt) {
    free(strPrompt);
  }
  if(strPromptFull) {
    free(strPromptFull);
  }
}


int printPrompt(char * strInput, char * strBuff, int nCols)
{
  int nLen, nRem;
  //.
  nLen = nRem = 0;
  //.
  //.printf("\033[7m\033[1m"); //.Reverse Video
  printf("\033[7;32m");
  nLen = strlen(strInput);
  nRem = nCols - nLen;
  memset(strBuff, 0x00, nCols);
  strcpy(strBuff, strInput);
  memset(strBuff + nLen, 0x20, nRem);
  printf("%s", strBuff);
  printf("\033[0m\033[0K");
  //.
  return(1);
}


int clearPrompt(int nRet)
{
  int nLen, nRem;
  //.
  nLen = nRem = 0;
  //.
  if(nRet == '\n') {
    printf("\033[A");
    printf("\033[0K");
  }
  else if(nRet == ' ') {
    printf("\033[A");
    printf("\033[0K");
  }
  //.
  return(nRet);
}


int togglesttyraw(int bRaw)
{
  //.
  //.Hack - Probably should be using stty -g
  //.
  if(bRaw) {
    system("/bin/stty raw");
    return(1);
  }
  else {
    system("/bin/stty -raw");
    return(0);
  }
  return(1);
}
//.
//. Src - /Users/petitmadjima/Desktop/USM/Spring2016/COS_350_SystemsProgramming/cos350_Prog4/cos350_Prog4
//. Bin - /Users/petitmadjima/Library/Developer/Xcode/DerivedData/cos350_Prog4-cvfpaqveeanteaexvbjbaekfvhwu/Build/Products/Debug
//. Cmd - ./cos350_Prog4 test.txt
//.
/* - E N D - */
