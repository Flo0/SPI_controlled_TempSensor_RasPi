/*
 * simulation of wiringPi-lib
 * W. Tasin - 2020
 *
 *   to create a static lib:
 *     gcc -c -o libwiringPiSim.o wiringPiSim.c
 *     ar rcs libwiringPiSim.a libwiringPiSim.o
 */
#include "wiringPiSim.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>

const char *piModelNames    [20] ;
const char *piRevisionNames [16] ;
const char *piMakerNames    [16] ;
const int   piMemorySize    [ 8] ;

struct wiringPiNodeStruct *wiringPiNodes ;

// Export variables for the hardware pointers

unsigned volatile int *_wiringPiGpio ;
unsigned volatile int *_wiringPiPwm ;
unsigned volatile int *_wiringPiClk ;
unsigned volatile int *_wiringPiPads ;
unsigned volatile int *_wiringPiTimer ;
unsigned volatile int *_wiringPiTimerIrqRaw ;

//// special simulation definitions
#define NUMBER_OF_PINS	41
#define MAP_FILENAME	"/tmp/wiringPi_gpio"

unsigned long long start_time;

volatile struct pin_map { unsigned char mode; char pud; char value; }
	gpio_map[NUMBER_OF_PINS];

// return system time in usec
static unsigned long long get_usec()
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return (unsigned long long) ts.tv_sec*1000000ull +
			ts.tv_nsec /1000ul;
}

// write out gpio map
static int write_out_map()
{
	int ok=FALSE;
	int fd;
	struct flock fl = { F_WRLCK, SEEK_SET, 0, 0 };
	mode_t mode = S_IRUSR | S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;
	fl.l_pid=getpid();
	
	umask(0);
	if ((fd=open(MAP_FILENAME, O_CREAT | O_WRONLY, mode))>-1)
	{
		if (fcntl(fd, F_SETLKW, &fl)>-1)
		{
			char linebuf[80];
			for (int i=0; i<NUMBER_OF_PINS; i++)
			{
				sprintf(linebuf, "%hhd %hhd %hhd\n", gpio_map[i].mode,
					gpio_map[i].pud, gpio_map[i].value);
				if (write(fd, linebuf, strlen(linebuf))<0)
				{
					fprintf(stderr, "Cannot write to %s.\n", MAP_FILENAME);
					perror("write");
				}

			}
			fl.l_type=F_UNLCK;
			if (fcntl(fd, F_SETLK, &fl)==-1)
			{
				fprintf(stderr, "Cannot unlock after writing.\n");
				perror("unlock");
			}
			else
			{
				ok=TRUE;
			}
		}
		else
		{
			fprintf(stderr, "Cannot lock for writing.\n");
			perror("lock");
		}
		close(fd);
	}
	else
	{
		fprintf(stderr, "Cannot create %s for writing.\n", MAP_FILENAME);
		perror("open()");
	}
	
	return ok;
}

// get a text line from an ioctl opened file
static int get_line(int fd, char *buf, unsigned max)
{
	int res=0, read_res;
	char ch;
	while ((read_res=read(fd, &ch, sizeof(char)))>0 && 
			res<max && ch!='\n')
	{
		*buf++=ch;
		res++;
	}
	*buf='\0';
	return (read_res>0) ? res : -1;
}
	
// read in the gpio map
static int read_in_map()
{
	int ok=FALSE;
	int fd;
	struct flock fl = { F_RDLCK, SEEK_SET, 0, 0 };
	fl.l_pid=getpid();
	
	if ((fd=open(MAP_FILENAME, O_RDONLY))>-1)
	{
		if (fcntl(fd, F_SETLKW, &fl)>-1)
		{
			char linebuf[80+1];
			int cnt=3;
			
			for (int i=0; cnt==3 && i<NUMBER_OF_PINS; i++)
			{
				get_line(fd, linebuf, 80);
				cnt=sscanf(linebuf, "%hhd %hhd %hhd", &gpio_map[i].mode,
					&gpio_map[i].pud, &gpio_map[i].value);
				if (cnt!=3)
				{
					fprintf(stderr, "Problem reading port %d\n", i-1);
				}

			}
			if (cnt==3)
				ok=TRUE;

			fl.l_type=F_UNLCK;
			if (fcntl(fd, F_SETLK, &fl)==-1)
			{
				fprintf(stderr, "Cannot unlock after reading.\n");
				perror("unlock");
			}

		}
		else
		{
			fprintf(stderr, "Cannot lock for reading.\n");
			perror("lock");
		}
		close(fd);
	}
	else
	{
		fprintf(stderr, "Cannot open %s for reading.\n", MAP_FILENAME);
		perror("open()");
	}
	return ok;
}

#ifdef __cplusplus
extern "C" {
#endif

// Data

// Internal

// extern int wiringPiFailure (int fatal, const char *message, ...) ;

// Core wiringPi functions

// extern struct wiringPiNodeStruct *wiringPiFindNode (int pin) ;
// extern struct wiringPiNodeStruct *wiringPiNewNode  (int pinBase, int numPins) ;

void wiringPiVersion	(int *major, int *minor) 
{
	*major=0;
	*minor=1;
}

int  wiringPiSetup       (void) 
{
	start_time=get_usec();
	write_out_map();
	return 0;
}

int  wiringPiSetupSys    (void) 
{
	fprintf(stderr, "not implemented yet.\n");
	return 0;
}
int  wiringPiSetupGpio   (void) 
{
	fprintf(stderr, "not implemented yet.\n");
	return 0;
}
int  wiringPiSetupPhys   (void) 
{
	fprintf(stderr, "not implemented yet.\n");
	return 0;
}

void pinModeAlt          (int pin, int mode) 
{
	fprintf(stderr, "not implemented yet.\n");
}

void pinMode             (int pin, int mode) 
{
	if (pin<NUMBER_OF_PINS)
	{
		gpio_map[pin].mode=mode;
		write_out_map();
	}
}

void pullUpDnControl     (int pin, int pud)
{
	if (pin<NUMBER_OF_PINS)
	{
		gpio_map[pin].pud=pud;
		write_out_map();
	}
}

int  digitalRead         (int pin)
{
	int value=-1;
	if (pin<NUMBER_OF_PINS)
	{
		read_in_map();
		value=gpio_map[pin].value;
	}
	return value;
}
void digitalWrite        (int pin, int value) 
{
	if (pin<NUMBER_OF_PINS)
	{
		if (gpio_map[pin].mode==OUTPUT)
		{
			gpio_map[pin].value=value;
			write_out_map();
		}
		else
			fprintf(stderr, "Port %d is not OUTPUT\n", pin);
	}
}
unsigned int  digitalRead8        (int pin) 
{
	return digitalRead(pin) & 0xFF;
}

void digitalWrite8       (int pin, int value) 
{
	digitalWrite(pin, value);
}

void pwmWrite            (int pin, int value) 
{
	fprintf(stderr, "not implemented yet.\n");
}

int  analogRead          (int pin) 
{
	fprintf(stderr, "not implemented yet.\n");
	return -1;
}

void analogWrite         (int pin, int value) 
{
	fprintf(stderr, "not implemented yet.\n");
}

// PiFace specifics 
//	(Deprecated)

int  wiringPiSetupPiFace (void) 
{
	fprintf(stderr, "deprecated.\n");
	return -1;
}
int  wiringPiSetupPiFaceForGpioProg (void) 	// Don't use this - for gpio program only
{
	fprintf(stderr, "deprecated.\n");
	return -1;
}

/* DISABLE the rest for now (wt)
// On-Board Raspberry Pi hardware specific stuff

extern          int  piGpioLayout        (void) ;
extern          int  piBoardRev          (void) ;	// Deprecated
extern          void piBoardId           (int *model, int *rev, int *mem, int *maker, int *overVolted) ;
extern          int  wpiPinToGpio        (int wpiPin) ;
extern          int  physPinToGpio       (int physPin) ;
extern          void setPadDrive         (int group, int value) ;
extern          int  getAlt              (int pin) ;
extern          void pwmToneWrite        (int pin, int freq) ;
extern          void pwmSetMode          (int mode) ;
extern          void pwmSetRange         (unsigned int range) ;
extern          void pwmSetClock         (int divisor) ;
extern          void gpioClockSet        (int pin, int freq) ;
extern unsigned int  digitalReadByte     (void) ;
extern unsigned int  digitalReadByte2    (void) ;
extern          void digitalWriteByte    (int value) ;
extern          void digitalWriteByte2   (int value) ;

// Interrupts
//	(Also Pi hardware specific)

extern int  waitForInterrupt    (int pin, int mS) ;
extern int  wiringPiISR         (int pin, int mode, void (*function)(void)) ;

// Threads

extern int  piThreadCreate      (void *(*fn)(void *)) ;
extern void piLock              (int key) ;
extern void piUnlock            (int key) ;

// Schedulling priority

extern int piHiPri (const int pri) ;
*/
// Extras from arduino land

void         delay             (unsigned int howLong) 
{
	unsigned int max_delay=1u << 31;
	unsigned cnt=(unsigned int)(1000ull*howLong/max_delay);
	for (unsigned int i=0; i<cnt ; i++)
		delayMicroseconds(max_delay);
	delayMicroseconds((unsigned int) (howLong*1000ull % max_delay));
}

void         delayMicroseconds (unsigned int howLong) 
{
	usleep(howLong);
}

unsigned int millis            (void) 
{
	return (unsigned int)((get_usec()-start_time)/1000ul);
}

unsigned int micros            (void) 
{
	return (unsigned int)(get_usec()-start_time);
	
}

////////////////// wiringSPI (wt)
#define SPI_DATA	"/tmp/wiringPiSPI_%d"
#define MAX_CHANNELS 2

static int spi_setup_ok[MAX_CHANNELS]= { FALSE };

int wiringPiSPIGetFd     (int channel)
{
	fprintf(stderr, "not implemented yet.\n");
	return -1;
}

int wiringPiSPIDataRW    (int channel, unsigned char *data, int len) 
{
	char spiFile[PATH_MAX];
	int ok=TRUE;
	int fd;
	struct flock fl = { F_RDLCK, SEEK_SET, 0, 0 };
	fl.l_pid=getpid();

	
	if (len>20)
	{
		fprintf(stderr, "only max. len of 20 supported\n");
		len=20;
	}

	if (channel<0 || channel>=MAX_CHANNELS)
	{
		fprintf(stderr, "unsupported channel number.\n");
		ok=FALSE;
	}
	
	if (!spi_setup_ok[channel])
	{
		fprintf(stderr, "channel hasn\'t been setup.\n");
		ok=FALSE;
	}
	
	sprintf(spiFile, SPI_DATA, channel);
	if (ok && (fd=open(spiFile, O_RDONLY))>-1)
	{
		if (fcntl(fd, F_SETLKW, &fl)>-1)
		{
			char linebuf[80+1], *lbuf=linebuf, *end=NULL;
			int cnt=0;
			long value;
			
			get_line(fd, linebuf, 80);
			value=strtol(lbuf, &end, 10);
			while (ok && cnt<len && lbuf!=end)
			{
				data[cnt]= (unsigned char) (value & 0xFF);
				cnt++;
				lbuf=end;
				value=strtol(lbuf, &end, 10);
			}
			if (cnt!=len || !ok)
			{
				fprintf(stderr, "Problem getting correct values from channel %d\n", channel);
			}

			fl.l_type=F_UNLCK;
			if (fcntl(fd, F_SETLK, &fl)==-1)
			{
				fprintf(stderr, "Cannot unlock SPI file after reading.\n");
				perror("unlock");
			}

		}
		else
		{
			fprintf(stderr, "Cannot lock SPI file for reading.\n");
			perror("lock");
		}
		close(fd);
	}
	else
	{
		fprintf(stderr, "The SPI sensor isn\'t ready.\n"
						"(Cannot open %s)\n", spiFile);
	}

	return (ok) ? 0 : -1;
}

int wiringPiSPISetup     (int channel, int speed) 
{
	int ok=-1;
	if (channel>=0 && channel<MAX_CHANNELS)
	{
		if (speed>=500000 && speed<=32000000)
		{
			ok=0;
			spi_setup_ok[channel]=TRUE;
		}
		else
			fprintf(stderr, "Wrong SPI speed.\n");
	}
	else
		fprintf(stderr, "Wrong SPI channel.\n");
	
	return ok;
}

int wiringPiSPISetupMode (int channel, int speed, int mode) 
{
	return wiringPiSPISetup(channel, speed);
}



#ifdef __cplusplus
}
#endif
