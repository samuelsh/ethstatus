#define VERSION "0.4a"
#define AUTHOR "Gabriel Montenegro / Christoph Haas"

#define LED_RX    1
#define LED_TX    2
#define LED_POWER 3
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_WHITE 7

/* Edited for Debian GNU/Linux. */
/* #define ROOTandRUNONBOOTcfgPATH "/etc"
*/
#define RC_FILENAME "ethstatus.cfg"
#define PATH_PROC_NET_DEV "/proc/net/dev"
/* Edited for Debian GNU/Linux. */
#define MAIL_PATH "/var/mail/"
#define LOG_PATH "/var/log/"

/* This will put the statistic coordinates (THIS WILL WORK WITH 0 or 1) */
typedef struct statvline {
   int hline[76]; /* This will be the horzontal lines */
} statvline[10]; /* ... and this will be vertical lines */
statvline graph;

struct tm localt;

typedef struct theme {
   int background;
   int border[2];
   int labels[2];
   int data[2];
   int version[2];
   int outgoing[2];
   int ingoing[2];
   int intersection[2];
   int power_led[4];
} theme;
theme colors;

/* This structure stays the INFO variables */
typedef struct DataStats {
   double rx_packets[2]; /* This is for RX LED/TX PACKETS INFO */
   unsigned long rx_errors[2];  /* Shows how many erros has ocurred on receive */
   double rx_bytes[2];  /* How many bytes has received */
   /* --------------------------------------------------------------------- */
   double tx_packets[2]; /* This is for TX LED/TX PACKETS INFO */ 
   unsigned long tx_errors[2]; /* Shows how many erros has ocurred on transmition */
   double tx_bytes[2]; /* How many Bytes has sent */
   /* --------------------COMPARATION-VARIABLES---------------------------- */
   double rx_packets_comp[2]; 
   double tx_packets_comp[2]; 
   double rx_bytes_comp;
   double tx_bytes_comp;
   /* --------------------------------------------------------------------- */
   unsigned long connect_time;
   unsigned long current_time;
   float top_speed;
   int top_packet;
   unsigned long log_time;
   unsigned long log_value;
   char *ip_addr_rtrn;
   char user_box[32];
   int email;
   char *start_time;
} DataStats;
DataStats stats;
struct stat st;

/* Functions Declarations */
void close_eths(char *errmsg, ...);
void chcolor(int x, int y);
void the_face(void);
void update_stat(int reload);
void update_info(void);
void led_on(unsigned int who);
void led_off(unsigned int who);
int is_online(short int force, short int only_check);
void clear_info(void);
void show_usage(char *name);
void get_time();
void ip_address(int display);
/* int log(int quit); */
void check_email(void);
void autoscale(char *, double);
/* End of Functions Declarations */

/* Variables Declarations */
char home[128];
char *interface = "eth0"; /* eth0... Default interface */
char data_type[6] = "bytes";
char mth[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
                    "Sep", "Oct", "Nov", "Dec" };

int eth_s = -1;
int CHAR = '#'; /* Bars character */
int sec_value = 0;

/* allocate memory for using the autoscale() subroutine */
char autoscalebytes[15];

float SPEED[2] = {10240, 10240} ;

unsigned short int VGA = 1;
unsigned short int con = 0; 
unsigned short int discon = 0;
unsigned short int disconnected = 0;
unsigned short int check = 0;
unsigned short int online = 0;
unsigned short int first_log = 1, lsnumber = 1; /* for logging info */
/* End of Variables Declarations */
