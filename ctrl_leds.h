#ifndef _GARDIEN_CTRL_LEDS_
#define _GARDIEN_CTRL_LEDS_

#define LEDJ "7"
#define LEDV "8"
#define LEDR "25"

void init();
void uninit();
void setValue(char *, char *);
void readLine(char *, int, int);

#endif
