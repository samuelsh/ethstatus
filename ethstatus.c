/***************************************************************************
    ethstatus.c  -  description

    begin              : Mar 13 1999
    original author    : Gabriel Montenegro
    current maintainer : Christoph Haas
 
 ***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program; if not, write to the Free Software            *
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,  *
 *  USA.                                                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <curses.h>
#include <getopt.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

#include <netinet/if_ether.h>
#include <linux/netdevice.h> 
#include <netinet/in.h>

#include "ethstatus.h"


/*-----------------------------------------------------------------------*
 *  IP_ADDRESS - Gets local IP address                                   *
 *-----------------------------------------------------------------------*/

void ip_address(int display)
{
   struct ifreq ifr;
   struct sockaddr_in *skt;

   if((eth_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
     {
        perror("socket");
        exit(0);
     }
   
   strcpy(ifr.ifr_name, interface);
   
   if(ioctl(eth_s, SIOCGIFADDR, &ifr) < 0);

   skt = (struct sockaddr_in*)&ifr.ifr_addr;
   
   if(skt->sin_addr.s_addr)
     stats.ip_addr_rtrn =  inet_ntoa(skt->sin_addr);
   else
    stats.ip_addr_rtrn = "Unknown";
   
   if(display)
     {
	chcolor(colors.data[0], colors.data[1]);
	mvprintw(17, 27, "%s", stats.ip_addr_rtrn);
	refresh();
     }
}

/*-----------------------------------------------------------------------*
 *  CHECK_EMAIL - Check for new emails                                   *
 *-----------------------------------------------------------------------*/

void check_email(void)
{
   FILE *email_box;
   int rtrn = 0;
   char location[128], buf[128];
   
   sprintf(location, MAIL_PATH "%s", stats.user_box);
   
   if(!stat(location, &st))
     rtrn = st.st_size; 
   stats.email = 0;
   chcolor(colors.data[0], colors.data[1]);
   if(rtrn){
      if((email_box = fopen(location, "r"))) 
	{
	   while(!feof(email_box))
	     {
		fgets(buf, 128, email_box);
		if(!strncmp(buf, "From ", 5))
		  stats.email++; 
	     }
	   if(stats.email < 10)
	     mvprintw(17, 65, "0%d         ", stats.email);
	   else
	     mvprintw(17, 65, "%d           ", stats.email);
	   fclose(email_box);
	}
      else
	mvprintw(17, 65, "Can't check");
   }
   else
     mvprintw(17, 65, "00         ");
   refresh();
}

/*-----------------------------------------------------------------------*
 *  CENTER   -   Put a string in center                                  *
 *-----------------------------------------------------------------------*/

void center(short int line, char *strg, ...)
{
   short int where;
   char *final;
   va_list trans;

   final = malloc(80);
   va_start(trans, strg);
   (void)vsnprintf(final, 80, strg, trans);
   va_end(trans);
   where = (int)(40 - (strlen(final) / 2));
   if(line != 0 && line != 24) {
      move(line, 20);
      hline(32, 50);
   }
   mvprintw(line, where, "%s",final);
   free(final);
}


/*-----------------------------------------------------------------------*
 *  IS_ONLINE - Check if the connection is active                        *
 *-----------------------------------------------------------------------*/
int is_online(short int force, short int only_check)
{
   int rtrn = 0;
   char check[256], red_tmp[256], *tmp;
   FILE *routefile;
   char b_rx[32], b_tx[32], e_rx[32], e_tx[32], p_rx[32], p_tx[32];
  
   if((routefile = fopen(PATH_PROC_NET_DEV, "r")) == NULL)
     {
	sprintf(red_tmp, "Error! Unable to open %s\n"
		"Check in your Kernel Configuration if /proc filesystem"
		" is supported\n\n", PATH_PROC_NET_DEV);
	close_eths(red_tmp);
     }
   while(!feof(routefile))
     {
	fgets(check, 256, routefile);
	if(strstr(check, interface)){
	   tmp=check;
	   while(*tmp!=':')
	     tmp++;
	   tmp++;
	   sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s %s",
		  b_rx, p_rx, e_rx,
		  red_tmp, red_tmp, red_tmp, red_tmp, red_tmp,
		  b_tx, p_tx, e_tx);
	   
	   stats.tx_packets[0] = (double)atof(p_tx);
	   stats.tx_bytes[0] = (double)atof(b_tx);
	   stats.tx_errors[0] = (double)atoi(e_tx);
	   stats.rx_packets[0] = (double)atof(p_rx);
	   stats.rx_bytes[0] = (double)atof(b_rx);
	   stats.rx_errors[0] = (double)atoi(e_rx);
	   
	   online = rtrn = 1;
	   break;
	}
     }
   fclose(routefile);

/* LED POWER ON/OFF */
   if(!only_check) {
      if(rtrn) {
	 if(!con || (force && con)){
	    chcolor(colors.power_led[0],colors.power_led[1]); move(12, 10); 
	    hline(CHAR,3);
	    con = 1; discon = 0;
	    refresh();
	 }
      }
      else{
	 if(!discon || (force && discon)) {
	    chcolor(colors.power_led[2], colors.power_led[3]); move(12, 10); 
	    hline(CHAR, 3);
	    con = online = 0; discon = 1;
	    refresh();
	 }
      }
   }
   return rtrn;
}

   
/*-----------------------------------------------------------------------*
 * UPDATE_INFO - Puts TX/RX Packets, Bytes, Erros on the screen          * 
 *-----------------------------------------------------------------------*/
void update_info(void)
{
   chcolor(colors.data[0] ,colors.data[1]);
   mvprintw(20, 27, "%.0f", stats.rx_packets[0]);
   mvprintw(20, 65, "%.0f", stats.tx_packets[0]);
   mvprintw(22, 27, "%ld", stats.rx_errors[0]);
   mvprintw(22, 65, "%ld", stats.tx_errors[0]);

	/* Calculate the measures automagically */
   chcolor(colors.labels[0], colors.labels[1]);
	autoscale(autoscalebytes, stats.rx_bytes[0]);
	mvprintw(21, 6, "Received:                        ");
	chcolor(colors.data[0], colors.data[1]);
	mvprintw(21, 27, "%sB", autoscalebytes);

   chcolor(colors.labels[0], colors.labels[1]);
	autoscale(autoscalebytes, stats.tx_bytes[0]);
	mvprintw(21,41, "Transmitted:                        ");
	chcolor(colors.data[0], colors.data[1]);
	mvprintw(21, 65, "%sB", autoscalebytes);
}

/*-------------------------------------------------------------------------*
 * CLEAR_INFO - Clear TX/RX Packets, Bytes, Erros...                       *
 *-------------------------------------------------------------------------*/
void clear_info(void)
{
   chcolor(colors.background ,0);
   mvprintw(19, 65, "   ");
   mvprintw(19, 27, "            ");
   mvprintw(20, 27, "            ");
   mvprintw(20, 65, "              ");
   mvprintw(22, 27, "            ");
   mvprintw(22, 65, "              ");
   mvprintw(21, 27, "            ");
   mvprintw(21, 65, "              ");
   mvprintw(16, 22, "               ");
   mvprintw(17, 27, "            ");
}


/*-----------------------------------------------------------------------*
 * DO_EXIT                                                               *
 *-----------------------------------------------------------------------*/

void do_exit(int x, short int line)
{
   fprintf(stderr, "\n\nEthStatus-v%s - %s", VERSION,AUTHOR);
/*
   if(!x)
     fprintf(stderr, "\n%s: Have no value in line %d\n\n", RC_FILENAME, line);
   else
     fprintf(stderr, "\n%s contains an invalid value on line %d.\n\n", RC_FILENAME, line);
     */
   fprintf(stdout, "Exiting...\n\n");
   exit(0);
}

/*-----------------------------------------------------------------------*
 * OPT_ANALISER - For cfgs files                                         *
 *-----------------------------------------------------------------------*/
char *opt_analiser(char *in)
{
   char real[256], temp[256], *rt;
   short int a, value = 0;
   bzero(temp, strlen(in));
   sprintf(temp, "%s", in);

   bzero(real, strlen(real));
   for(a = 0; a < strlen(temp)-1; a++){
      switch(temp[a])
        {
         case '\t':
           real[a] = '\0';
           break;
         case '#':
           value++;
           real[a] = '\0';
           break;
         default:
           if(!value)
             real[a] = temp[a];
        }
   }
   real[a] = '\0';
   rt = malloc(strlen(real));
   rt = (char *)real;
   return rt;
   free(rt);
}

/*-----------------------------------------------------------------------*
 *  GET_OPTION                                                           *
 *-----------------------------------------------------------------------*/
char *get_option(char *in)
{
   short int b = 0, a;
   char real[256], get[256], *rt;

   sprintf(real, "%s", in);
   for(a = 0; a <= strlen(real)-1; a++)
     if(real[a] == '=') {
        while(real[a+1] != '\0')
          {
             if(real[a+1] != ' ')
               get[b] = real[a+1];
             else
               b--;
             b++; a++;
          }
        get[b] = '\0';
        break;
     }

   rt = malloc(strlen(get));
   rt = (char *)get;
   return rt;
   free(rt);
}


/*-----------------------------------------------------------------------*
 *  LED_ON/OFF - Show the leds on the screen                             *
 *-----------------------------------------------------------------------*/
void led_on(unsigned int who)
{
   if(who == LED_RX){
      chcolor(colors.ingoing[0], colors.ingoing[1]);
      move(12, 37); hline(CHAR, 2);
   }
   else if(who == LED_TX){
      chcolor(colors.outgoing[0], colors.outgoing[1]); 
      move(12, 41); hline(CHAR, 2);
   }
}


void led_off(unsigned int who)
{
   if(who == LED_RX){
      chcolor(colors.labels[0], colors.labels[1]);
      mvprintw(12, 37, "  ");
   }
   else if(who == LED_TX){
      chcolor(colors.labels[0], colors.labels[1]);
      mvprintw(12, 41, "  ");
   }
}


/*-----------------------------------------------------------------------*
 *  UPDATE_STAT - Update The Graphical Statistic                         *
 *-----------------------------------------------------------------------*/
void update_stat(int reload)
{
   int x, y;
   int bps[2], pps[2];
   int ps;
   float makegraph_in = 0, makegraph_out = 0, ks;
   
   /* Putting in the last hline the new statistic */
   
   
   if(!stats.rx_bytes_comp)
     stats.rx_bytes_comp = stats.rx_bytes[0];
   if(!stats.tx_bytes_comp)
     stats.tx_bytes_comp = stats.tx_bytes[0];
   if(!stats.tx_packets_comp[1])
     stats.tx_packets_comp[1] = stats.tx_packets[0];
   if(!stats.rx_packets_comp[1])
     stats.rx_packets_comp[1] = stats.rx_packets[0];
   
   bps[0] = (stats.rx_bytes[0] - stats.rx_bytes_comp);
   bps[1] = (stats.tx_bytes[0] - stats.tx_bytes_comp);
   pps[0] = (stats.rx_packets[0] - stats.rx_packets_comp[1]);
   pps[1] = (stats.tx_packets[0] - stats.tx_packets_comp[1]);
   
   /* ks variable gets the speed in k/s of the connection */
   ks = (float)(bps[0] + bps[1]);
   ks /= 1024;
   ps = pps[0] + pps[1];
   
	autoscale(autoscalebytes, ks * 1024);
   center(14, "%sB/s   %d Packets/s", autoscalebytes, ps);
	/*
   center(14, "%.2f KB/s   %d Packets/s", ks, ps);
	*/

   if(ks > stats.top_speed)
     {
	mvprintw(19, 27, "             ");
	mvprintw(19, 27, "%sB/s", autoscalebytes);
	/* mvprintw(19, 27, "%.2f KB/s", ks); */
	stats.top_speed = ks;
     }
   if(ps > stats.top_packet)
     {
	mvprintw(19, 65, "             ");
	mvprintw(19, 65, "%d", ps);
	stats.top_packet = ps;
     }
   stats.rx_bytes_comp = stats.rx_bytes[0];
   stats.tx_bytes_comp = stats.tx_bytes[0];
   stats.rx_packets_comp[1] = stats.rx_packets[0];
   stats.tx_packets_comp[1] = stats.tx_packets[0];
   
   /* makegraph variable gets the amplitude of line bar */
	/* [0] = incoming / [1] = outgoing */
	/* SPEED = factor from command line */
	/* y = height of vertical bar */
	/* y=10 -> (1 KB/sec  *  factor) */
   
   for(y = 0; y < 10; y++)
     {
	if( ( (float)bps[0] > (float)(SPEED[0] * 102.4) * y ) &&
	    ( (float)bps[0] <= (float)(SPEED[0] * 102.4) * (y+1)) )
	  makegraph_in = y + 1;
	else if((float)bps[0] > (float)(SPEED[0] * 1024))
	  makegraph_in = 10;
	
	if( ( (float)bps[1] > (float)(SPEED[1] * 102.4) * y ) &&
	    ( (float)bps[1] <= (float)(SPEED[1] * 102.4) * (y+1)) )
	  makegraph_out = y + 1;
	else if((float)bps[1] > (float)(SPEED[1] * 1024))
	  makegraph_out = 10;
     }
											      
   for(x = 0; x != 10; x++) {
      graph[x].hline[75] = 0;
   }
   
   if(makegraph_in > 10)
     makegraph_in = 10;
   if(makegraph_out > 10)
     makegraph_out = 10;

        for(x = 0; x < makegraph_in; x++)
          graph[x].hline[75] = 1;

        for(x = 0; x < makegraph_out; x++)
          {
             if(graph[x].hline[75] == 1)
               graph[x].hline[75] = 3;
             else
               graph[x].hline[75] = 2;
	  }

   for(y = 0; y != 10; y++) {
      for(x = 0; x != 76; x++) {
         /* Move the graphic to the left) */
         if(!reload)
           if(x)
             graph[y].hline[x-1] = graph[y].hline[x];

         /* Print the graphic on screen */
         if(graph[y].hline[x] == 3) /* intersection color */
           {
              chcolor(colors.intersection[0], colors.intersection[1]);
              move((y+10)-(2*y), x+2);
              hline(CHAR, 1);
           }
         else if(graph[y].hline[x] == 1) /* ingoing color */
           {
              chcolor(colors.ingoing[0], colors.ingoing[1]);
              move((y+10)-(2*y), x+2);
              hline(CHAR, 1);
           }
         else if(graph[y].hline[x] == 2) /* outgoing color */
           {
              chcolor(colors.outgoing[0], colors.outgoing[1]);
              move((y+10)-(2*y), x+2);
              hline(CHAR, 1);
           }
         else
           {
              chcolor(colors.background, 0);
              mvaddstr((y+10)-(2*y), x+2, " ");
	   }
      }
   }
}
   

/*--------------------------------------------------------------------------*
 * SHOW_USAGE - Print the right usage on the screen                         *
 *--------------------------------------------------------------------------*/
void show_usage(char *name)
{
   fprintf(stderr, "\nEthStatus v" VERSION " - " AUTHOR "\n"
	   "Usage: %s\n"
	   "  [-i interface]                      interface name\n"
	   "  [-v [vga|mono]]                     video display mode\n"
	   "  [-S speed[k|m|g]]                   line speed in [K|M|G]bps\n"
	   "  [-s [10|100|64|128|256|768|1540]]   line speed (backward compatibility)\n"
	   "\n" , name);
}

/*--------------------------------------------------------------------------*
 *  CHCOLOR  -  Change the Color                                            *
 *--------------------------------------------------------------------------*/
void chcolor(int x, int y) {
   
   init_pair(1, COLOR_BLACK, colors.background);
   init_pair(2, COLOR_RED, colors.background);
   init_pair(3, COLOR_GREEN, colors.background);
   init_pair(4, COLOR_YELLOW, colors.background);
   init_pair(5, COLOR_BLUE, colors.background);
   init_pair(6, COLOR_MAGENTA, colors.background);
   init_pair(7, COLOR_CYAN, colors.background);
   init_pair(8, COLOR_WHITE, colors.background);
   
   if(!y)
     attrset(COLOR_PAIR(x+1));
   if(y)
     attrset(COLOR_PAIR(x+1)|A_BOLD);
}

/*---------------------------------------------------------------------------*
 * CLOSE_ETHS                                                                *
 *---------------------------------------------------------------------------*/

void close_eths(char *errmsg, ...)
{
   va_list trans;
   char *final;
   final = malloc(strlen(errmsg));
   va_start(trans, errmsg);
   (void)vsnprintf(final, strlen(errmsg), errmsg, trans);
   va_end(trans);
   endwin();
   curs_set(1);
   system("clear");
   fprintf(stderr, "%s", final);
   free(final);
   exit(0);
}

/*---------------------------------------------------------------------------*
 *  FACE - Draw the face of the program                                      *
 *---------------------------------------------------------------------------*/
void face(void)
{
   short int x, y;
   chcolor(colors.background, 0);
   for(x = 0; x <= 24; x++)
     for(y = 0; y <= 80; y++)
       mvprintw(x, y, " ");
   refresh();
   chcolor(colors.border[0], colors.border[1]);
   border(0, 0, 0, 0, 0, 0, 0, 0);
   /*center(24, ".Author : Gabriel Montenegro <johnpetrucci@users.sourceforge.net>.");*/
   refresh();
   chcolor(colors.labels[0], colors.labels[1]);
   mvprintw(17, 6,"IP Address:");
	/*
   if(check)
     mvprintw(17, 41, "New email(s):");
	  */
   mvprintw(20, 6, "Received Packets:");
   mvprintw(22, 6, "Errors on Receiving:");
   mvprintw(20, 41,"Transmited Packets:");
   mvprintw(22, 41,"Errors on Transmission:");
   mvprintw(13, 37,"RX");
   mvprintw(13, 41,"TX");
   mvprintw(18, 6,"Interface name:");
   mvprintw(18, 41, "Graphic Using Speed:");
   mvprintw(13, 9, "ON/OFF"); 
   mvprintw(19, 6, "Top Speed:");
   mvprintw(19, 41, "Top Packets/s:");
   refresh();
   chcolor(colors.version[0], colors.version[1]);
   mvprintw(00, 31,".EthStatus v" VERSION ".");
   refresh();
   chcolor(colors.data[0], colors.data[1]);
   mvprintw(18, 27, "%s", interface);
	/* technically incorrect - should be 1024 */
	autoscale(autoscalebytes, SPEED[0] * (double) 1000 );
   mvprintw(18, 65, "%sB/s", autoscalebytes);
   mvprintw(19, 27, "0.00 KB/s");
   mvprintw(19, 65, "0");
   refresh(); 
}


/*--------------------------------------------------------------------------*
 *  M A I N - The head of the program                                       *
 *--------------------------------------------------------------------------*/
int main(int argc, char *argv[]) 
{  
   short int x, y, key_pressed, first = 1, reload = FALSE;
   time_t cron;
   cron = time(NULL);
   stats.start_time = ctime(&cron);

	/* variables for -S option */
	int bps;
	char modifier;
	long int KBps;

   /* set the default colors */
	colors.ingoing[0] = COLOR_GREEN; colors.ingoing[1] = 1;
	colors.outgoing[0] = COLOR_YELLOW; colors.outgoing[1] = 1;
	colors.intersection[0] = COLOR_GREEN; colors.intersection[1] = 0;
	colors.background = COLOR_BLACK;
	colors.data[0] = COLOR_CYAN; colors.data[1] = 0;
	colors.border[0] = COLOR_WHITE; colors.data[1] = 0;
	colors.labels[0] = COLOR_WHITE; colors.labels[1] = 0;
	colors.version[0] = COLOR_WHITE; colors.labels[1] = 1;
	colors.power_led[0] = COLOR_GREEN; colors.power_led[1] = 1;	/* LED on */
	colors.power_led[2] = COLOR_RED; colors.power_led[3] = 0;	/* LED off */

	/* read command line options */
	int option;
	while ((option = getopt (argc, argv, "hi:s:S:v:")) != -1)
	{
		switch (option)
		{
			case 'h':
				show_usage(argv[0]);
				exit(0);
				break;

			case 'i':
				if(strlen(optarg) > 5)
				{
					show_usage(argv[0]);
					exit(10);
				}
				interface = optarg;
				break;

			case 'v' :
				if(!strcmp(optarg, "mono"))
				{
					VGA = 0;
					break;
				}
				if(!strcmp(optarg, "vga"))
				{
					VGA = 1;
					break;
				}
				else
				{
					show_usage(argv[0]);
					exit(10);
				}
				break;

			case 'S':	/* incoming/outgoing bandwidth */
				/* the format is e.g. 10m or 64k or 1g */
				if (!sscanf(optarg, "%i%c", &bps, &modifier))
				{
					show_usage(argv[0]);
					exit(10);
				}

				if (bps > 1024)
				{
					show_usage(argv[0]);
					exit(10);
				}

				/* printf("Number=%i, Modifier=%c\n", bps, modifier); */
				KBps = bps;

				/* consider the modifier (k/m/g/) */
				if (modifier == 'k' || modifier == 'K')
					KBps *= 1;
				else if (modifier == 'm' || modifier == 'm')
					KBps *= 1024;
				else if (modifier == 'g' || modifier == 'g')
					KBps *= (1024*1024);
				else
				{
					show_usage(argv[0]);
					exit(10);
				}

				/* convert bits per second into KB per second */
				KBps /= 8;

				if (KBps == 0)
				{
					show_usage(argv[0]);
					fprintf(stdout, "ERROR: bandwidth set too low\n");
					exit(10);
				}

				/*printf("KB/sec=%i\n", KBps); exit(0);*/

				SPEED[0]=(float) KBps;
				SPEED[1]=(float) KBps;
				break;

			case 's' :
				if(!strcmp(optarg, "100") || !strcmp(optarg, "10"))
					SPEED[0] = SPEED[1] = (atof(optarg) / 8) * 1024;
				else if(!strcmp(optarg, "64") || !strcmp(optarg, "128"))
					SPEED[0] = SPEED[1] = atof(optarg);
				else if(!strcmp(optarg, "256"))
				{
					SPEED[0] = atof(optarg);
					SPEED[1] = 64;
				}
				else if(!strcmp(optarg, "768"))
				{
					SPEED[0] = atof(optarg);
					SPEED[1] = 128;
				}
				else if(!strcmp(optarg, "1540"))
				{
					SPEED[0] = atof(optarg);
					SPEED[1] = 256;
				}
				else
				{
					show_usage(argv[0]);
					exit(10);
				}
				break;

			case '?':
				show_usage(argv[0]);
				exit(10);
				break;
		}
	}

   /* Initialize some info variables */
   stats.tx_packets[0] = 0; stats.tx_bytes[0] = 0; stats.tx_errors[0] = 0;
   stats.rx_packets[0] = 0; stats.rx_bytes[0] = 0; stats.rx_errors[0] = 0;
   stats.tx_bytes_comp = 0; stats.tx_bytes_comp = 0;
   fprintf(stderr, "Running EthStatus v%s...", VERSION);
   printf("\e(U"); /* Thanks kspoon */
   initscr();
   intrflush(stdscr, FALSE);
   if(VGA)
     start_color();
   init_color(1, COLOR_RED, COLOR_GREEN, COLOR_BLUE);
   curs_set(0); /* Toggle the cursor off */ 
   nonl(); 
   noecho(); 
   cbreak(); 
   nodelay(stdscr, TRUE);
   face(); 
   update_info();
   /*
   if(check)
     check_email();
     */
   while(1){ 
      if(is_online(0, 0)) {
	 if(disconnected) {
	    clear_info();
	    for(y = 0; y != 10; y++)
	      for(x = 0; x != 75; x++)
		graph[y].hline[x] = 0;
	    
	    stats.top_speed = stats.top_packet = stats.rx_bytes_comp = stats.tx_bytes_comp = stats.rx_packets_comp[0] = stats.tx_packets_comp[0] = stats.tx_packets_comp[1] = stats.rx_packets_comp[1] = 0;
	    update_stat(0);
	 }       
	 cron = time(NULL);
	 localt = *localtime(&cron);
	 /* In case of a new RX Packet... the RX Led Turn on 
	  * Else... Turn off */
	 if(stats.rx_packets[0] != stats.rx_packets_comp[0])
	   led_on(LED_RX);
	 else
	   led_off(LED_RX);
	 /* In case of a new TX Packet... the TX Led Turn on
	  * Else... Turn off */
	 if(stats.tx_packets[0] != stats.tx_packets_comp[0])	
	   led_on(LED_TX);
	 else
	   led_off(LED_TX);
	 
	 stats.rx_packets_comp[0] = stats.rx_packets[0];
	 stats.tx_packets_comp[0] = stats.tx_packets[0];
	 
	 update_info();
    
	 if((sec_value == 0 && first) || disconnected)
	   {
	      sec_value = localt.tm_sec;
	      stats.log_value = localt.tm_sec + (localt.tm_min * 60) + (localt.tm_hour * 3600);
	      ip_address(1); update_stat(0);
	      first = 0; disconnected = 0;
	   }
	 if((stats.log_value + stats.log_time) == localt.tm_sec + (localt.tm_min * 60) + (localt.tm_hour * 3600))
	   {
	      stats.log_value = localt.tm_sec + (localt.tm_min * 60) + (localt.tm_hour * 3600);
	   }
	 if(localt.tm_sec != sec_value)
	   {
	      if(check)
		check_email();
	      update_stat(0);
	      sec_value = localt.tm_sec;
	   }
	 usleep(10L); 
      }
      else
	usleep(100000L);
      
      key_pressed = getch();
      if(key_pressed != ERR && tolower(key_pressed) == 'q')
	break;
      else if(key_pressed != ERR && tolower(key_pressed) == 'r')
	{
	   face();
	   is_online(1, 0);
	   ip_address(1);
	   if(check)
              check_email();
	   update_stat(1);
	   reload = TRUE;
	}
   }
   endwin();
   system("clear");
   curs_set(1);
   fprintf(stdout, "EthStatus v" VERSION " - " AUTHOR "\n\n");
   return 0; 
}

/******
 * autoscale
 * input:  any byte or bytes/sec count
 * output: string holding a value with kilo,mega,giga...
 ******/
void autoscale(char *str, double bytes)
{
	if (bytes > (double) 1024*1024*1024)
		sprintf(str, "%.2f G", (bytes/ (double) (1024*1024*1024)));
	else if (bytes > (double) 1024*1024)
		sprintf(str, "%.2f M", (bytes/ (double) (1024*1024)));
	else if (bytes > (double) 1024)
		sprintf(str, "%.2f K", (bytes/ (double) (1024)));
	else sprintf(str, "%.2f ", bytes);
}
