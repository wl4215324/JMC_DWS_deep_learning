#include "serial_port_commu.h"

static int speed_arr[] = {B115200, B57600, B38400, B19200, B9600, B4800};
static int name_arr[] = {115200, 57600, 38400,  19200,  9600, 4800};

static int open_serial(const char* dev_name)
{
	return (open(dev_name, O_RDWR));
}

static int set_speed(int fd, int speed)
{
	int i;
	int status;
	struct termios  Opt;
	tcgetattr(fd, &Opt);

	for( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
	{
		if(speed == name_arr[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);

			if (status != 0)
			{
     			return -1;
			}
         }

		tcflush(fd,TCIOFLUSH);
    }

	return 1;
}


static int set_parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if( tcgetattr( fd,&options)  !=  0)
	{
		return -1;
	}
	options.c_cflag &= ~CSIZE;

	// 数据位数
	switch (databits)
	{
  		case 7:
  			options.c_cflag |= CS7;
  			break;
  		case 8:
  			options.c_cflag |= CS8;
  			break;
  		default:
  			fprintf(stderr,"Unsupported data size\n");
  			return -1;
	}

	//parity setting
	switch (parity)
  	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;           /* Clear parity enable */
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			options.c_lflag = ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH);
			options.c_lflag &= ~(ICANON|ISIG);
			options.c_iflag &= ~(ICRNL|IGNCR);
			options.c_iflag &= ~(ICRNL | IXON);
			options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
			options.c_oflag &= ~(ONLCR | OCRNL | ONOCR | ONLRET);
			break;

		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			options.c_lflag = ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH);
			options.c_lflag &= ~(ICANON|ISIG);
			options.c_iflag &= ~(ICRNL|IGNCR);
			options.c_iflag &= ~(ICRNL | IXON);
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;      /* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S':
		case 's':  /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported parity\n");
			return -1;
	}

	//停止位
	switch (stopbits)
  	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported stop bits\n");
			return -1;
	}

	if (parity != 'n')
	{
  		options.c_iflag |= INPCK;
	}
//    options.c_cc[VTIME] = 30; // 3S
	options.c_cc[VTIME] = 10; // 1000ms
    options.c_cc[VMIN] = 0;

    tcflush(fd,TCIFLUSH);  /* Update the options and do it NOW */
    if (tcsetattr(fd,TCSANOW,&options) != 0)
  	{
		return -1;
	}

    return 1;
}

int open_set_serial_port( )
{
	int fd = -1;

	if((fd = open_serial(RS232_DEV_NAME)) < 0)
	{
		return -1;
	}

	if(set_speed(fd, 115200) < 0)
	{
		return -1;
	}

	if(set_parity(fd, 8, 1, 'N') < 0)
	{
		return -1;
	}

	return fd;
}
