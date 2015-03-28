/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Teunis van Beelen
*
* teuniz@gmail.com
*
***************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
***************************************************************************
*
* This version of GPL is at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*
***************************************************************************
*/

/* last revision: February 1, 2013 */

/* For more info and how to use this libray, visit: http://www.teuniz.net/RS-232/ */

// This version is adapted for use in the Smartmeter context \
// on a Raspberry PI by Hans de Rijck, 2014

#include "serial.h"

int Cport[30],
    error;

struct termios new_port_settings,
       old_port_settings[30];

char comports[30][16]={"/dev/ttyS0",    //  0
                       "/dev/ttyS1",    //  1
                       "/dev/ttyS2",    //  2
                       "/dev/ttyS3",    //  3
                       "/dev/ttyS4",    //  4
                       "/dev/ttyS5",    //  5
                       "/dev/ttyS6",    //  6
                       "/dev/ttyS7",    //  7
                       "/dev/ttyS8",    //  8
                       "/dev/ttyS9",    //  9
                       "/dev/ttyS10",   // 10
                       "/dev/ttyS11",   // 11
                       "/dev/ttyS12",   // 12
                       "/dev/ttyS13",   // 13
                       "/dev/ttyS14",   // 14
                       "/dev/ttyS15",   // 15
                       "/dev/ttyUSB0",  // 16
                       "/dev/ttyUSB1",  // 17
                       "/dev/ttyUSB2",  // 18
                       "/dev/ttyUSB3",  // 19
                       "/dev/ttyUSB4",  // 20
                       "/dev/ttyUSB5",  // 21
                       "/dev/ttyAMA0",  // 22 (Pi onboard serial)
                       "/dev/ttyAMA1",  // 23
                       "/dev/ttyACM0",  // 24
                       "/dev/ttyACM1",  // 25
                       "/dev/rfcomm0",  // 26
                       "/dev/rfcomm1",  // 27
                       "/dev/ircomm0",  // 28
                       "/dev/ircomm1"}; // 29



int Serial_OpenComport(int comport_number, int baudrate, int data_bits, int stop_bits, char parity )
{
  int BAUD, status;
  int DATABITS;
  int STOPBITS;
  int PARITYON;
  int PARITY;

  if((comport_number>29)||(comport_number<0))
  {
    printf("illegal comport number\n");
    return(1);
  }

  switch(baudrate)
  {
    case      50 : BAUD = B50;
                   break;
    case      75 : BAUD = B75;
                   break;
    case     110 : BAUD = B110;
                   break;
    case     134 : BAUD = B134;
                   break;
    case     150 : BAUD = B150;
                   break;
    case     200 : BAUD = B200;
                   break;
    case     300 : BAUD = B300;
                   break;
    case     600 : BAUD = B600;
                   break;
    case    1200 : BAUD = B1200;
                   break;
    case    1800 : BAUD = B1800;
                   break;
    case    2400 : BAUD = B2400;
                   break;
    case    4800 : BAUD = B4800;
                   break;
    case    9600 : BAUD = B9600;
                   break;
    case   19200 : BAUD = B19200;
                   break;
    case   38400 : BAUD = B38400;
                   break;
    case   57600 : BAUD = B57600;
                   break;
    case  115200 : BAUD = B115200;
                   break;
    case  230400 : BAUD = B230400;
                   break;
    case  460800 : BAUD = B460800;
                   break;
    case  500000 : BAUD = B500000;
                   break;
    case  576000 : BAUD = B576000;
                   break;
    case  921600 : BAUD = B921600;
                   break;
    case 1000000 : BAUD = B1000000;
                   break;
    default      : printf("invalid BAUDrate\n");
                   return(1);
                   break;
  }
      switch (data_bits)
      {
         case 8:
         default:
            DATABITS = CS8;
            break;
         case 7:
            DATABITS = CS7;
            break;
         case 6:
            DATABITS = CS6;
            break;
         case 5:
            DATABITS = CS5;
            break;
      }  //end of switch data_bits
      switch (stop_bits)
      {
         case 1:
         default:
            STOPBITS = 0;
            break;
         case 2:
            STOPBITS = CSTOPB;
            break;
      }  //end of switch stop bits
      
      switch (parity)
      {
         case 'n':
         default:                       //none
            PARITYON = 0;
            PARITY = 0;
            break;
         case 'o':                        //odd
            PARITYON = PARENB;
            PARITY = PARODD;
            break;
         case 'e':                        //even
            PARITYON = PARENB;
            PARITY = 0;
            break;
      }  //end of switch parity
       

  Cport[comport_number] = open(comports[comport_number], O_RDWR | O_NOCTTY | O_NDELAY);
  if(Cport[comport_number]==-1)
  {
    perror("unable to open comport ");
    return(1);
  }
  else
  {
	 printf( "COM port %s opened\n", comports[comport_number] );
  }

  error = tcgetattr(Cport[comport_number], old_port_settings + comport_number);
  if(error==-1)
  {
    close(Cport[comport_number]);
    perror("unable to read portsettings ");
    return(1);
  }
  memset(&new_port_settings, 0, sizeof(new_port_settings));  /* clear the new struct */

  new_port_settings.c_cflag = BAUD | CRTSCTS | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;
  new_port_settings.c_iflag = IGNPAR; 
  new_port_settings.c_oflag = 0;
  
  new_port_settings.c_lflag = 0;
  new_port_settings.c_cc[VMIN] = 1;      /* block untill n bytes are received */
  new_port_settings.c_cc[VTIME] = 0;     /* block untill a timer expires (n * 100 mSec.) */
  error = tcsetattr(Cport[comport_number], TCSANOW, &new_port_settings);
  if(error==-1)
  {
    close(Cport[comport_number]);
    perror("unable to adjust portsettings ");
    return(1);
  }

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
    return(1);
  }

  status |= TIOCM_DTR;    /* turn on DTR */
  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
    return(1);
  }

  return(0);
}


int Serial_PollComport(int comport_number, unsigned char *buf, int size)
{
  int n;

#ifndef __STRICT_ANSI__                       /* __STRICT_ANSI__ is defined when the -ansi option is used for gcc */
  if(size>SSIZE_MAX)  size = (int)SSIZE_MAX;  /* SSIZE_MAX is defined in limits.h */
#else
  if(size>4096)  size = 4096;
#endif

  n = read(Cport[comport_number], buf, size);

  return(n);
}


int Serial_ReadLine( int comport_number, unsigned char *buf, int size )
{
  int n, count = 0;
  char charRead[10];
  
  while ( 1 )
  {
    n = read( Cport[comport_number], charRead, 1 );
    if ( n == 1 )
    {
        if ( charRead[0] == '\n' )
        {
            break;
        }
        
        buf[ count++ ] = charRead[0];
        if ( count > size ) 
        {
            count = -1;
            break;
        }
    }
  }
  if ( count >= 0 )
  {
    buf[count] = 0x00;
  }
  return( count );
}

int Serial_SendByte(int comport_number, unsigned char byte)
{
  int n;

  n = write(Cport[comport_number], &byte, 1);
  if(n<0)  return(1);

  return(0);
}


int Serial_SendBuf(int comport_number, unsigned char *buf, int size)
{
  return(write(Cport[comport_number], buf, size));
}


void Serial_CloseComport(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */
  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }

  close(Cport[comport_number]);
  tcsetattr(Cport[comport_number], TCSANOW, old_port_settings + comport_number);
}

/*
Constant  Description
TIOCM_LE  DSR (data set ready/line enable)
TIOCM_DTR DTR (data terminal ready)
TIOCM_RTS RTS (request to send)
TIOCM_ST  Secondary TXD (transmit)
TIOCM_SR  Secondary RXD (receive)
TIOCM_CTS CTS (clear to send)
TIOCM_CAR DCD (data carrier detect)
TIOCM_CD  Synonym for TIOCM_CAR
TIOCM_RNG RNG (ring)
TIOCM_RI  Synonym for TIOCM_RNG
TIOCM_DSR DSR (data set ready)
*/

int Serial_IsCTSEnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_CTS) return(1);
  else return(0);
}

int Serial_IsDSREnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_DSR) return(1);
  else return(0);
}

void Serial_enableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_DTR;    /* turn on DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void Serial_disableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void Serial_enableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void Serial_disableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void Serial_cputs(int comport_number, const char *text)  /* sends a string to serial port */
{
  while(*text != 0)   Serial_SendByte(comport_number, *(text++));
}


