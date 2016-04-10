/*
 * i2ckiss.c
 *
 * code to allow a tnc connected to the i2c bus to be presented to the kernel as a pseudo ttys.
 
 	 will spawn a kissattach if the kissattach params are provided
 	 or create a symlink for use by linbpq node software (or other ax.25 packages that dont use the linux kernel ax.25)
	 
 * based on axtools mkiss.cki
 *
 *
 * mkiss.c
 *
 * Fake out AX.25 code into supporting dual port TNCS by routing serial
 * port data to/from two pseudo ttys.
 *
 * Version 1.03
 *
 * N0BEL
 * Kevin Uhlir
 * kevinu@flochart.com
 *
 * 1.01 12/30/95 Ron Curry - Fixed FD_STATE bug where FD_STATE was being used for
 * three state machines causing spurious writes to wrong TNC port. FD_STATE is
 * now used for real serial port, FD0_STATE for first psuedo tty, and FD1_STATE
 * for second psuedo tty. This was an easy fix but a MAJOR bug.
 *
 * 1.02 3/1/96 Jonathan Naylor - Make hardware handshaking default to off.
 * Allowed for true multiplexing of the data from the two pseudo ttys.
 * Numerous formatting changes.
 *
 * 1.03 4/20/96 Tomi Manninen - Rewrote KISS en/decapsulation (much of the
 * code taken from linux/drivers/net/slip.c). Added support for all the 16
 * possible kiss ports and G8BPQ-style checksumming on ttyinterface. Now
 * mkiss also sends a statistic report to stderr if a SIGUSR1 is sent to it.
 *
 * 1.04 25/5/96 Jonathan Naylor - Added check for UUCP style tty lock files.
 * As they are now supported by kissattach as well.
 *
 * 1.05 2
 0/8/96 Jonathan Naylor - Convert to becoming a daemon and added
 * system logging.
 *
 * 1.06 23/11/96 Tomi Manninen - Added simple support for polled kiss.
 *
 * 1.07 12/24/97 Deti Fliegl - Added Flexnet/BayCom CRC mode with commandline
 * parameter -f    
 *
 * 1.08 xx/xx/99 Tom Mazouch - Adjustable poll interval
 *
 * i2ckiss
 *
 * 1.00 xx/08/2012 John Wiseman G8BPQ - created from mkiss.c 1.08
 */


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#include <netax25/ttyutils.h>
#include <netax25/daemon.h>

#include "config.h"

//#include <linux/i2c.h>
#include <linux/i2c-dev.h>
//#include <i2c/smbus.h>
//#include "i2cbusses.h"

int posix_openpt(int oflag);
int unlockpt(int fd);
int grantpt(int fd);
int ptsname_r(int fd, char *buf, size_t buflen);

#define G8BPQ_CRC	1

#define	SIZE		400 //4096

#define FEND		0300	/* Frame End			(0xC0)	*/
#define FESC		0333	/* Frame Escape			(0xDB)	*/
#define TFEND		0334	/* Transposed Frame End		(0xDC)	*/
#define TFESC		0335	/* Transposed Frame Escape	(0xDD)	*/

#define ACKREQ		0x0C
#define POLL		0x0E

/*
 * Keep these off the stack.
 */
 
 
static int address;
static char progname[80];

static unsigned char ibuf[SIZE];	/* buffer for input operations	*/
static unsigned char obuf[SIZE];	/* buffer for kiss_tx()		*/

static int crc_errors		= 0;
static int invalid_ports	= 0;
static int return_polls		= 0;

static char *usage_string	= "usage: i2ckiss [-p interval] [-d] [-l] [-v] i2cbus i2cdevice [[-m mtu] port inetaddr]\n"
							  "       i2ckiss [-p interval] [-d] [-l] [-v] i2cbus i2cdevice symlink comname\n";

static int dump_report		= FALSE;

static int logging = TRUE;
static int crcflag = G8BPQ_CRC;			// Always use G8BPQ CRC
static int pollspeed = 1;				// Need to Poll
static int diags = 0;

static pid_t child_pid;	

static struct iface *pty;
static struct iface *tty;


struct iface
{
	char	*name;				/* Interface name (/dev/???)	*/
	int		device;				/* i2c device */
	int		fd;					/* File descriptor		*/
	int		escaped;			/* FESC received?		*/
	unsigned char	crc;		/* Incoming frame crc		*/
	unsigned char	obuf[SIZE];	/* TX buffer			*/
	unsigned char	*optr;		/* Next byte to transmit	*/
	unsigned int	errors;		/* KISS protocol error count	*/
	unsigned int	nondata;	/* Non-data frames rx count	*/
	unsigned int	rxpackets;	/* RX frames count		*/
	unsigned int	txpackets;	/* TX frames count		*/
	unsigned long	rxbytes;	/* RX bytes count		*/
	unsigned long	txbytes;	/* TX bytes count		*/
};

static int kiss_rx(struct iface *ifp, unsigned char c, int usecrc);
static int kiss_tx(int fd, int port, unsigned char *s, int len, int usecrc);
static void report(struct iface *tty, struct iface *pty, int numptys);

static int poll(int fd)
{
	unsigned int retval;
	int len;
	
	retval = i2c_smbus_read_byte(fd);
	
	//	Returns POLL (0x0e) if nothing to receive, otherwise the control byte of a frame
	
	if (retval == -1)	 		// Read failed		
  	{
		perror("i2ckiss: poll failed");	 
	
		if (logging)
			report(tty, pty, 1);
	
		exit(0);

	}
		
	if (retval == 0x0e)
		return 0;
		
	if (diags)
		printf("%x ", retval);
	
	
	// 	Read message up to a FEND
		
	while (TRUE)
	{
		len = kiss_rx(tty, retval, crcflag);
		
		if (len == -1)
		return 0;		// Bad Packet
			
		if (len)
		{			
			// We have a complete KISS frame from TNC
						
			kiss_tx(pty->fd, 0, tty->obuf, len, 0);
			pty->txpackets++;
			pty->txbytes += len;
		
			printf("Packet Received\n");
			return 0;
		}
		
		usleep(1000);
		
		retval = i2c_smbus_read_byte(fd);

		if (diags)
		{
			if (retval != 0x0e)
				printf("%x ", retval);
			else
				printf(".");
		}
				
		if (retval == -1)	 		// Read failed		
	  	{
			perror("i2ckiss: poll failed in packet loop");	
				
			if (logging)
				report(tty, pty, 1);
	
			exit(0);
		}
	}

}

static int put_ubyte(unsigned char* s, unsigned char * crc, unsigned char c, int usecrc)
{ 
  	int len = 1;

  	if (c == FEND) { 
		*s++ = FESC;
		*s++ = TFEND;
		len++;
  	} else { 
		*s++ = c;
		if (c == FESC) {
			*s++ = TFESC;
			len++;
		}
	}

	switch (usecrc) {
	case G8BPQ_CRC:
		*crc ^= c;	/* Adjust checksum */
		break;
	}

	return len;
}

static int kiss_tx(int fd, int port, unsigned char *s, int len, int usecrc)
{
	unsigned char *ptr = obuf;
	unsigned char c, cmd;
	unsigned char crc = 0;
	int i;
	int ret = 0;

	cmd = s[0] & 0x0F;

	/* Non-data frames don't get a checksum byte */
	if (usecrc == G8BPQ_CRC && cmd != 0 && cmd != ACKREQ)
		usecrc = FALSE;

	/*
	 * Send an initial FEND character to flush out any
	 * data that may have accumulated in the receiver
	 * due to line noise.
	 */

	*ptr++ = FEND;

	c = *s++;
	c = (c & 0x0F) | (port << 4);
	ptr += put_ubyte(ptr, &crc, c, usecrc);
	
	/*
	 * For each byte in the packet, send the appropriate
	 * character sequence, according to the SLIP protocol.
	 */

	for(i = 0; i < len - 1; i++)
		ptr += put_ubyte(ptr, &crc, s[i], usecrc);

	/*
	 * Now the checksum...
	 */
	switch (usecrc) {
	case G8BPQ_CRC:
		c = crc & 0xFF;
		ptr += put_ubyte(ptr, &crc, c, usecrc);
		break;
	}
	
	*ptr++ = FEND;
	
	if (fd == pty->fd)
		return write(fd, obuf, ptr - obuf);
	
	for (i = 0; i < ptr - obuf; i++)
	{
		ret += i2c_smbus_write_byte(fd, obuf[i]);
		usleep(1000);
	}
	
	return ret;
}

static int kiss_rx(struct iface *ifp, unsigned char c, int usecrc)
{
	int len;

	switch (c)
	{
	case FEND:
	
		len = ifp->optr - ifp->obuf;

		if (len != 0 && ifp->escaped)
		{
		 	/* protocol error...	*/
			len = 0;		/* ...drop frame	*/
			syslog(LOG_INFO, "Dropping Frame\n");

			ifp->errors++;
		}
		
		if (len != 0)
		{
			if (usecrc)
			{
				if ((ifp->crc & 0xFF) != 0)
				{
					syslog(LOG_INFO, "CRC Error %d\n", ifp->crc);
					/* checksum failed...	*/
					/* ...drop frame	*/
					len = 0;
					crc_errors++;
				}
				 else
				{
					/* delete checksum byte	*/
					len--;
				}				 
			}
		}
		if (len != 0)
		{ 
			ifp->rxpackets++;
			ifp->rxbytes += len;
		}
		/*
		 * Clean up.
		 */
		ifp->optr = ifp->obuf;
		ifp->crc = 0;
		ifp->escaped = FALSE;
		return len;
		
	case FESC:
		ifp->escaped = TRUE;
		return 0;
		
	case TFESC:
		if (ifp->escaped) {
			ifp->escaped = FALSE;
			c = FESC;
		}
		break;
	case TFEND:
		if (ifp->escaped) {
			ifp->escaped = FALSE;
			c = FEND;
		}
		break;
	default:
		if (ifp->escaped) {		/* protocol error...	*/
			ifp->escaped = FALSE;
			ifp->errors++;
		}
		break;
	}

	*ifp->optr++ = c;

	if (usecrc)
		ifp->crc ^= c;
		
	len = ifp->optr - ifp->obuf;
	
	if (len > SIZE)
	{
		ifp->optr--;
		syslog(LOG_INFO, "Frame too long\n");
		return -1;
	}
		
	return 0;
}

static void sigterm_handler(int sig)
{
	if (logging) {
		syslog(LOG_INFO, "terminating on SIGTERM\n");
		closelog();
	}
		
	exit(0);
}

static void sigusr1_handler(int sig)
{
	signal(SIGUSR1, sigusr1_handler);
	dump_report = TRUE;
}

static void report(struct iface *tty, struct iface *pty, int numptys)
{
	long t;

	time(&t);
	syslog(LOG_INFO, "version %s.", VERSION);
	syslog(LOG_INFO, "Status report at %s", ctime(&t));	      
	syslog(LOG_INFO, "Poll interval %d00ms", pollspeed);
	syslog(LOG_INFO, "i2cinterface is %s/%x (fd=%d)", tty->name, tty->device, tty->fd);
	syslog(LOG_INFO, "pty is %s (fd=%d)", pty->name, pty->fd);
	syslog(LOG_INFO, "Checksum errors: %d", crc_errors);
	syslog(LOG_INFO, "Invalid ports: %d", invalid_ports);
	syslog(LOG_INFO, "Returned polls: %d", return_polls);
	syslog(LOG_INFO, "Interface   TX frames TX bytes  RX frames RX bytes  Errors");
	syslog(LOG_INFO, "%-11s %-9u %-9lu %-9u %-9lu %u",
	       tty->name,
	       tty->txpackets, tty->txbytes,
	       tty->rxpackets, tty->rxbytes,
	       tty->errors);

	syslog(LOG_INFO, "%-11s %-9u %-9lu %-9u %-9lu %u",
	       pty->name,
	       pty->txpackets, pty->txbytes,
		   pty->rxpackets, pty->rxbytes,
		   pty->errors);
	
	return;
}

int main(int argc, char *argv[])
{
	unsigned char *icp;
	int topfd;
	fd_set readfd;
	struct timeval timeout, pollinterval;
	int retval, size, len;
	int masterfd;
	char slavedevice[80];
	int mtu = 0;
	char i2cname [20];
	
	
	char* arg_list[] = {"kissattach", NULL, NULL, NULL, NULL, NULL, NULL};	// must be 3 args - pty, port and inetaddr, May be -m mtu -l
	

	//	We must poll. Default is set to 100 ms, and we must use BPQ CRC
	
	pollinterval.tv_sec = pollspeed / 10;
	pollinterval.tv_usec = (pollspeed % 10) * 100000L;

	while ((size = getopt(argc, argv, "dlp:m:v")) != -1) {
		switch (size) {
	
		case 'd':
	        diags++;
			break;
		case 'l':
	        logging = TRUE;
	        break;
		case 'p':
			pollspeed = atoi(optarg);
			pollinterval.tv_sec = pollspeed / 10;
			pollinterval.tv_usec = (pollspeed % 10) * 100000L;
	        break;
		case 'm':
			mtu = atoi(optarg);
			break;
		case 'v':
			printf("i2ckiss: %s\n", VERSION);
			return 1;
		case ':':
		case '?':
			fprintf(stderr, usage_string);
			return 1;
		}
	}

	// Allow 2 or 4 params (if 4, invoke KISSATTACH or symlink with the last two params

	if ((argc - optind) < 2 || (argc - optind) == 3 || (argc - optind) > 4){
		fprintf(stderr, usage_string);
		return 1;
	}

	// Open and configure the i2c interface
	
	if ((tty = calloc(1, sizeof(struct iface))) == NULL) {
		perror("i2ckiss: malloc");
		return 1;
	}
	
	sprintf(i2cname, "/dev/i2c-%s", argv[optind]);

                              
	tty->fd = open(i2cname, O_RDWR);
	if (tty->fd < 0)
	{
		fprintf(stderr, "%s: Cannot find i2c bus %s, no such file or directory.\n", progname, i2cname);
		exit(1);
	}
		
	//	check_funcs(file, size, daddress, pec)
	 
	address = strtol(argv[optind + 1], 0, 0);
 
 	retval = ioctl(tty->fd,  I2C_SLAVE, address);
	
	printf("%d %d\n", retval, errno);
	
	if(retval == -1)
	{
		fprintf(stderr, "%s: Cannot open i2c device %x\n", progname, address);
		exit (1);
	}
 
 	retval = ioctl(tty->fd,  I2C_TIMEOUT, 100);
	
	printf("%d %d\n", retval, errno);
	
	tty->name = argv[optind];
	tty->optr = tty->obuf;
	topfd = tty->fd;

	/*
	 * Open and configure the pty interface
	 */
	 
	masterfd = posix_openpt(O_RDWR|O_NOCTTY);

	if (masterfd == -1 || grantpt (masterfd) == -1 || unlockpt (masterfd) == -1 ||
		 ptsname_r(masterfd, slavedevice, 80) != 0)
	{
		perror("i2ckiss: Create PTY pair failed");
		return 1;
	}                                           
                                       
	printf("slave device is: %s\n", slavedevice);
                                                                                                                                                                                                                                                                            
	if ((pty = calloc(1, sizeof(struct iface))) == NULL) {
		perror("i2ckiss: malloc");
		return 1;
	}
	
 	pty->fd = masterfd;
	pty->name = slavedevice; 
	tty_raw(pty->fd, FALSE);
	pty->optr = pty->obuf;
	topfd = (pty->fd > topfd) ? pty->fd : topfd;

	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, sigusr1_handler);
	signal(SIGTERM, sigterm_handler);
	
	// Reset the TNC and wait for completion
	
	i2c_smbus_write_byte(tty->fd, FEND);		
	i2c_smbus_write_byte(tty->fd, 9);
	
	printf("Resetting TNC...\n");
	
	sleep(2);

	if (logging) {
		openlog("i2ckiss", LOG_PID, LOG_DAEMON);
		syslog(LOG_INFO, "starting");
	}
	
	
	if ((argc - optind) == 4)
	{
		
	if (strcmp(argv[optind + 2], "symlink") == 0)
	{
		int ret;
		
		unlink (argv[optind + 3]);
		
		ret = symlink (slavedevice, argv[optind + 3]);
		
		perror("symlink");
		
		if (ret == 0)
			printf ("symlink from %s to %s created\n", slavedevice, argv[optind + 3]);
		else
			printf ("symlink from %s to %s failed\n", slavedevice, argv[optind + 3]);	
	}
	else
	{
	
		//	Spawn KISSATTACH

		arg_list[1] = slavedevice;
		arg_list[2] = argv[optind + 2];
		arg_list[3] = argv[optind + 3];
	
		if (mtu)
		{
			char mtustring[20];
			sprintf(mtustring, "-m %d", mtu);
			arg_list[4] = mtustring;
		
			if (logging)
				arg_list[5] = "-l";
			
		}
		else
			if (logging)
				arg_list[4] = "-l";
	 
  
    	/* Duplicate this process. */ 

		child_pid = fork (); 

		if (child_pid != 0) 
 		{    
			close(tty->fd);			// Close the i2c device
			close(pty->fd);			// Close the pty master device
				
			execvp (arg_list[0], arg_list); 
        
			/* The execvp  function returns only if an error occurs.  */ 

			fprintf (stderr,  "Failed to run kissattach\n"); 
			exit(1);
		
		}	                                     
	}								 

	}
	if (!daemon_start(FALSE)) {
		fprintf(stderr, "i2ckiss: cannot become a daemon\n");
		return 1;
	}

	/*
	 * Loop until an error occurs on a read.
	 */

	while (TRUE) {
		FD_ZERO(&readfd);
		FD_SET(pty->fd, &readfd);

		if (pollspeed)
			timeout = pollinterval;

		errno = 0;
		retval = select(topfd + 1, &readfd, NULL, NULL, pollspeed ? &timeout : NULL);

		if (retval == -1) {
			if (dump_report) {
				if (logging)
					report(tty, pty, 1);
				dump_report = FALSE;
				continue;
			} else {
				perror("i2ckiss: select");
				continue;
			}
		}

		/*
		 * Timer expired - let's poll...
		 */
		if (retval == 0 && pollspeed) {
			poll(tty->fd);
			continue;
		}

		//	TTY is i1c device - we get messages from it via poll()
		
		// See if a character has arrived on pty 
		
		if (FD_ISSET(pty->fd, &readfd))
		{
			if ((size = read(pty->fd, ibuf, SIZE)) < 0 && errno != EINTR)
			{
				if (logging)
					syslog(LOG_ERR, "pty->fd: %m\n");
				goto end;
			}
			
			for (icp = ibuf; size > 0; size--, icp++)
			{
				len = kiss_rx(pty, *icp, FALSE);
				
				if (len)
				{
					// We have a complete KISS frame from Kernel
						
					kiss_tx(tty->fd, 0, pty->obuf, len, crcflag);
					tty->txpackets++;
					tty->txbytes += len;
				}
			}
		}
	}
	
end:
	
	if (logging)
		report(tty, pty, 1);
	
	close(tty->fd);
	close(pty->fd);
		
	return 1;
}
