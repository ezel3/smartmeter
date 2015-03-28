//    Smart Meter data aggregation program
//    Copyright (C) 2014-2015 Hans de Rijck
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Compile with: gcc -pthread smartmeter.c serial.c -l bcm2835 -o smartmeter
// without bcm2835: gcc -pthread smartmeter.c serial.c -o smartmeter
// with sqlite: gcc -pthread smartmeter.c serial.c -lsqlite3 -o smartmeter
// with MySQL: gcc -pthread smartmeter.c serial.c -o smartmeter $(mysql_config --cflags) $(mysql_config --libs)


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql.h>

#include "serial.h"

#define COMPORT           22
#define BAUDRATE        9600
#define RECEIVE_CHARS   4096
#define MEASURES         500

#define FALSE 0
#define TRUE !FALSE

// RPi Plug P1 pin 11 (which is GPIO pin 17) is RTS pin of P1 interface
// RPi Plug P1 pin 15 (which is GPIO pin 22) is OE pin of TXB0106
#define RTS_PIN RPI_GPIO_P1_11
#define OE_PIN RPI_GPIO_P1_15

#define meetdata  "electra"
#define dailydata "meetdata"

int writeToDatabase( char * database, char * query );
void siginthandler(int param);
void *transact( );

typedef struct Standen
{
    int laag_tarief;
    int hoog_tarief;
    int totaal_tarief;
    float gas;
} Standen;

Standen standen;

int electra_current = 0;
int numMeasures = 0;
int electraSum[MEASURES + 5];
char timeSum[MEASURES + 5][50];
int refresh = 0;

// Stand van de laatste afrekening (17 september 2013)
// Moet eigenlijk in settings bestand
int jaar_electra = 3563;
int jaar_gas = 2684;

pthread_t tid;

int main(int argc, char **argv)
{
    unsigned char receive_buffer[RECEIVE_CHARS + 5];
    int lastHour = 0;
    int lastDay = 0;
    int lastMinute = 0;
    char contfile[50] = {0};
    char allfile[50] = {0};
    char dbBuffer[512];
    char midnightTime[80];

    char tempDone = FALSE;
    signal(SIGINT, siginthandler);

    FILE * file;
    file = fopen( "smsave.dat", "rb" );
    if ( file != 0 )
    {
        fread( &standen, sizeof( struct Standen ), 1, file );
        fclose( file );
    }

    printf( "Vorige waarden:\n" );
    printf( "Laag tarief    : %d kWh\n", standen.laag_tarief );
    printf( "Hoog tarief    : %d kWh\n", standen.hoog_tarief );
    printf( "Totaal electra : %d kWh\n", standen.totaal_tarief );
    printf( "Gasmeter       : %g m3\n", (float)standen.gas );

    int err = pthread_create( &(tid), NULL, &transact, NULL);
    if (err != 0)
	{
        printf("\ncan't create thread :[%s]", strerror(err));
	}
    else
	{
        printf("\n Listener thread created successfully\n");
	}

    printf( "open de poort...\n" );
    Serial_OpenComport( COMPORT, 9600, 7, 1, 1 );

    printf( "zet buffer...\n" );

    setbuf(stdout, NULL);

    printf( "waiting for meter...\n" );
    while(1)
    {
        int electra_low_tarif = 0;
        int electra_high_tarif = 0;
        int electra_total_tarif = 0;
        int huidig_tarief = 0;
        char tarief[12];
        float gas = 0;

        while( receive_buffer[0] != '!' )
        {
            int n = Serial_ReadLine( COMPORT, receive_buffer, RECEIVE_CHARS );

            if ( n >= 0 )
            {
                if ( !strncmp( "1-0:1.8.1", (const char *)receive_buffer, 9 ) )
                {
                    char temp[24];
                    strncpy( temp, (const char*)receive_buffer + 10, 5 );
                    temp[9] = 0x00;
                    electra_low_tarif = atoi( temp );
                }
                if ( !strncmp( "1-0:1.8.2", (const char *)receive_buffer, 9 ) )
                {
                    char temp[24];
                    strncpy( temp, (const char *)receive_buffer + 10, 5 );
                    temp[9] = 0x00;
                    electra_high_tarif = atoi( temp );
                }
                if ( !strncmp( "1-0:1.7.0", (const char *)receive_buffer, 9 ) )
                {
                    char temp[24];
                    strncpy( temp, (const char *)receive_buffer + 10, 9 );
                    temp[9] = 0x00;
                    electra_current = (int)(atof( temp ) * 1000 );
                }
                if ( !strncmp( "0-0:96.14.0", (const char *)receive_buffer, 11 ) )
                {
                    char temp[24];
                    strncpy( temp, (const char *)receive_buffer + 12, 4 );
                    temp[4] = 0x00;
                    huidig_tarief = atoi( temp );
                }
                if ( !strncmp( "(", (const char *)receive_buffer, 1 ) )
                {
                    char temp[24];
                    strncpy( temp, (const char *)receive_buffer + 1, 9 );
                    temp[9] = 0x00;
                    gas = (float)( atof( temp ) );
                }
            }

            // wait a bit
            usleep( 500*1000 );  // 500 milliseconds
        }

        // calculate time
        char timebuffer[50] = {0};
        time_t current_time;
        struct tm * loctime;

        current_time = time( NULL );
        loctime = localtime( &current_time );
        strftime( timebuffer, 50, "%Y-%m-%d %H:%M:%S", loctime );

        if ( huidig_tarief == 2 )
        {
            strcpy( tarief, "pt" );
        }
        else if ( huidig_tarief == 1 )
        {
            strcpy( tarief, "dt" );
        }
        else
        {
            strcpy( tarief, "??" );
        }

        receive_buffer[0] = 0x00;
		
        // every hour
        if ( (loctime->tm_hour != lastHour) || 
		       numMeasures >= MEASURES      ||
			   refresh == 1 ) 
        {
            int i = 0;
            lastMinute = loctime->tm_min;
            lastHour = loctime->tm_hour;

			refresh = 0;

            printf( "hourly %d.\n", lastHour );
            // Filename should be in properties file
            FILE * file = fopen( "/mnt/usb_hdd/hourly.dat", "a+b" );
			if ( file != NULL )
			{
				fprintf( file, "%s %s %d %d %d %g\n", timebuffer,
						 tarief,
						 electra_low_tarif,
						 electra_high_tarif,
						 electra_low_tarif + electra_high_tarif,
						 (float)gas
					   );
				fclose( file );
			}

            file = fopen( "continuous.dat", "a+b" );
			if ( file != NULL )
			{
				for ( i = 0; i < numMeasures; i++ )
				{
					fprintf( file, "%s\t%d\n", timeSum[ i ], electraSum[ i ] );
				}
				fclose( file );
			}
			
            file = fopen( contfile, "a+b" );
			if ( file != NULL )
			{
				for ( i = 0; i < numMeasures; i++ )
				{
					fprintf( file, "%s,%d\n", timeSum[ i ], electraSum[ i ] );
				}
				fclose( file );
			}
			
            file = fopen( "/var/www/current.html", "w+b" );
            fprintf( file, "Gegevens van %s<br/>", timebuffer );
            fprintf( file, "<pre><big>Vandaag verbruikt electra  : %d kWh<br/>", ( electra_low_tarif + electra_high_tarif) - (standen.laag_tarief + standen.hoog_tarief) );
            fprintf( file, "Vandaag verbruikt gas      : %.2g m<sup>3</sup><br/><br/>", (float)(gas - standen.gas) );

            fprintf( file, "Sinds 17 sept 2013 electra : %d kWh<br/>", ( electra_low_tarif + electra_high_tarif) - (jaar_electra ) );
            fprintf( file, "Sinds 17 sept 2013 gas     : %.2f m<sup>3</sup><br/><br/>", (float)(gas - jaar_gas) );

            fprintf( file, "Laag tarief    : %d kWh<br/>", electra_low_tarif );
            fprintf( file, "Hoog tarief    : %d kWh<br/>", electra_high_tarif );
            fprintf( file, "Totaal electra : %d kWh<br/>", electra_low_tarif + electra_high_tarif );
            fprintf( file, "Gas            : %g m<sup>3</sup><br/>", (float)gas );
            fprintf( file, "Samples        : %d <br/></big></pre>", numMeasures );
            fclose( file );

            numMeasures = 0;

            system( "./hourtrunc.sh" );
            system( "./hourplt.sh" );

            printf( "%s\n", timebuffer );
            printf( "Huidig verbruik: %d W %s\n", electra_current, tarief );
            printf( "Laag tarief    : %d kWh\n", electra_low_tarif );
            printf( "Hoog tarief    : %d kWh\n", electra_high_tarif );
            printf( "Totaal electra : %d kWh\n", electra_low_tarif + electra_high_tarif );
            printf( "Gasmeter       : %g m3\n", (float)gas );
            file = fopen( "hourly.dat", "a+b" );
            fprintf( file, "%s\t%d\t%d\t%d\t%g\n", timebuffer, huidig_tarief, electra_low_tarif, electra_high_tarif, gas );
            fclose( file );

            system( "./daytrunc.sh" );
            system( "./dayplt.sh" );
        }

        // every day
        if ( loctime->tm_hour == 0 &&
                lastDay != loctime->tm_mday )
        {

            lastDay = loctime->tm_mday;
            strcpy( midnightTime, timebuffer );
            // Need to do temperature later
            tempDone = FALSE;

            printf( "daily %d.\n", lastDay );
           
            file = fopen( "daily.dat", "a+b" );
			if ( file != NULL )
			{
				fprintf( file, "%s %d %d %d %g %d %d %d %g\n", timebuffer,
						 electra_low_tarif,
						 electra_high_tarif,
						 electra_low_tarif + electra_high_tarif,
						 (float)gas,
						 electra_low_tarif - standen.laag_tarief,
						 electra_high_tarif - standen.hoog_tarief,
						 electra_low_tarif + electra_high_tarif - standen.totaal_tarief,
						 (float)( (float)gas - (float)standen.gas )
					   );
				fclose( file );
			}
            sprintf( dbBuffer, "insert into daily values ('%s','%d','%d','%g','%d','%d','%g','%g','%g','%g')", 
                     timebuffer,
                     electra_low_tarif,
                     electra_high_tarif,
                     (float)gas,
                     electra_low_tarif - standen.laag_tarief,
                     electra_high_tarif - standen.hoog_tarief,
                     (float)( (float)gas - (float)standen.gas ),
                     0,
                     0,
                     0
                   );
            
            writeToDatabase( dailydata, dbBuffer );
            
//insert into daily values ('2014-08-08 00:00:04','2520','3623','4722.93','2','5','0.520996');
            standen.laag_tarief = electra_low_tarif;
            standen.hoog_tarief = electra_high_tarif;
            standen.totaal_tarief = electra_high_tarif + electra_low_tarif;
            standen.gas = gas;

            file = fopen( "smsave.dat", "w+b" );
            fwrite( &standen, sizeof( struct Standen ), 1, file );
            fclose( file );

	        strftime( contfile, 50, "/mnt/usb_hdd/%Y-%m-%d.dat", loctime );
			printf( "daily file %s\n", contfile );
            
            system( "./monthplt.sh" );
        }

        ////////////////////////////////
        // every day 8 o'clock (knmi data is not available earlier)
        if ( loctime->tm_hour == 8 &&
                !tempDone )
        {
        char urlbuffer[300];
        char str[100];
        int i = 0;
        float tavg, tmin, tmax;
        time_t yester_time;
        struct tm * yesterday;

            tempDone = TRUE;

            printf( "knmi.\n" );

            yester_time = time( NULL ) - (60 * 60 * 24);
            yesterday = localtime( &yester_time );

            sprintf( urlbuffer, 
                 "curl \"http://www.knmi.nl/klimatologie/daggegevens/getdata_dag.cgi?stns=270&vars=TG:TN:TX&byear=%d&bmonth=%d&bday=%d&eyear=%d&emonth=%d&eday=%d\" > knmi.txt",
                 yesterday->tm_year + 1900,
                 yesterday->tm_mon + 1,
                 yesterday->tm_mday,
                 yesterday->tm_year + 1900,
                 yesterday->tm_mon + 1,
                 yesterday->tm_mday
                 );

            system( urlbuffer );

            file = fopen( "knmi.txt", "r+b" );
            for ( i = 0; i < 24; i++ )
            {
                if ( fgets( str, 80, file ) == NULL ) break;
            }
            fclose( file );

            file = fopen( "curlurl.txt", "a+b" );
            fprintf( file, "%s\nlastDay: %d, loctime->tm_mday: %d\n", urlbuffer, lastDay, loctime->tm_mday );
            fclose( file );
            
            str[20] = 0x00;
            str[26] = 0x00;
            str[32] = 0x00;
            tavg = atof(str+15) / 10;
            tmin = atof(str+21) / 10;
            tmax = atof(str+27) / 10;            
            sprintf( dbBuffer, "update daily set t_min=%g, t_max=%g, t_avg=%g where datum='%s'", 
                     (float)tmin,
                     (float)tmax,
                     (float)tavg,
                     midnightTime
                   );
            
            writeToDatabase( dailydata, dbBuffer );

        }
        ////////////////////////////////        

        strftime( allfile, 50, "/mnt/usb_hdd/all-%Y-%m-%d.dat", loctime );
        file = fopen( allfile, "a+b" );
        if ( file != NULL )
        {
             fprintf( file, "%s,%s,%d,%d,%d,%g\n", timebuffer,
                                                   tarief,
                                                   electra_low_tarif,
                                                   electra_high_tarif,
                                                   electra_current,
                                                  (float)gas
                                       );
             fclose( file );
        }

        strftime( allfile, 50, "/mnt/usb_hdd/plot-%Y-%m-%d.dat", loctime );
        //file = fopen( allfile, "a+b" );
        file = fopen( "/var/www/electra.csv", "a+b" );
        if ( file != NULL )
        {
             fprintf( file, "%s,%d\n", timebuffer,electra_current );
             fclose( file );
        }

        sprintf( dbBuffer, "insert into electra (datum, waarde) values ('%s','%d')", timebuffer,electra_current );
        writeToDatabase ( meetdata, dbBuffer );

        char balk[80], spaties[80];
        int balklengte = electra_current/50;
        if ( balklengte > 70 ) balklengte = 70;
        int i = 0;
        for ( i = 0; i < balklengte; i++ )
        {
            balk[i] = '=';
        }
        balk[balklengte] = 0x00;

        balklengte = 60 - balklengte;
        if ( balklengte < 0 ) balklengte = 0;
        for ( i = 0; i < balklengte; i++ )
        {
            spaties[i] = ' ';
        }
        spaties[balklengte] = 0x00;

        printf( "%s (%d W %s) %s\r", balk, electra_current, tarief, spaties );
        electraSum[ numMeasures ] = electra_current;
        strcpy( timeSum[ numMeasures ], timebuffer );
        numMeasures++;

    }
    Serial_CloseComport(COMPORT);

    printf( "done.\n" );

    return 0;
}

int writeToDatabase( char * database, char * query )
{        
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *server = "localhost";

    /* should be in properties file */
    char *user = "DBUSER";
    char *password = "DBPASSWORD"; 

    conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) 
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        return(1);
    }
    
    /* send SQL insert */
    if ( mysql_query(conn, query ) ) 
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        return(1);
    }

    res = mysql_use_result(conn);
    
    /* close connection */
    mysql_free_result(res);
    mysql_close(conn);
}
/////////////////////////////////////////////////////////////////////////////

void siginthandler(int param)
{
	int i = 0;
    FILE * file;
    file = fopen( "smsave.dat", "w+b" );
    fwrite( &standen, sizeof( struct Standen ), 1, file );
    fclose( file );
    printf( "Standen vastgelegd.\n" );

	file = fopen( "continuous.dat", "a+b" );
	if ( file != NULL )
	{
		for ( i = 0; i < numMeasures; i++ )
		{
			fprintf( file, "%s\t%d\n", timeSum[ i ], electraSum[ i ] );
		}
		fclose( file );
	
		numMeasures = 0;
		printf( "Laatste metingen vastgelegd.\n" );
	}
	else
	{
		printf( "Kan continuous.dat niet schrijven.\n" );
	}
    exit(1);
}

//void error( const char *msg )
//{
//    perror( msg );
//    exit( 1 );
//}

void * transact( )
{
    int                 sockfd, newsockfd, portno;
    socklen_t           clilen;
    char                buffer[256];
    struct sockaddr_in  serv_addr, cli_addr;
    int                 n;

    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if( sockfd < 0 )
    {
        perror( "ERROR opening socket" );
    }
    printf( "Socket created.\n" );

    bzero( (char *) &serv_addr, sizeof(serv_addr) );
    portno = 8001;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons( portno );
    if( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 )
    {
        perror( "ERROR on binding" );
		exit(1);
    }
    printf( "Socket bound.\n" );

    listen( sockfd, 5 );
    clilen = sizeof( cli_addr );
    while( 1 )
    {
        newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, &clilen );
        if( newsockfd < 0 )
        {
            perror( "ERROR on accept" );
			close( newsockfd );
			continue;
        }
	    //printf( "Connection accepted.\n" );

        bzero( buffer, 256 );
        n = read( newsockfd, buffer, 255 );
        if( n < 0 )
        {
            perror( "ERROR reading from socket" );
        }

        //printf( "Message: %s\n", buffer );

		if ( !strcmp( buffer, "refresh" ) )
		{
			refresh = 1;
		}
		
		//sprintf( buffer, "{\n  \"Current\": %d\n}", electra_current );
		sprintf( buffer, "%d", electra_current );
        n = write( newsockfd, buffer, strlen( buffer ) );
        if( n < 0 )
        {
            perror( "ERROR writing to socket" );
        }

		close( newsockfd );
    }
    close( sockfd );
    return;
}
