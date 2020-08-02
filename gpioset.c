// compile with
// gcc -O -o gpioset gpioset.c
// run with
// sudo chrt 99 ./dht <GPIO_PIN> <SAMPLING_INTERVAL>

#define DEVICE "/dev/gpiomem"
#define GPIOMEM_SIZE 0xB4
#define GPFSEL0 0 //GPIO function select 0
#define GPFSEL1 0x4 //GPIO function select 1
#define GPFSEL2 0x8 //GPIO function select 2
#define GPFSEL3 0xC //GPIO function select 3
#define GPFSEL4 0x10 //GPIO function select 4
#define GPFSEL5 0x14 //GPIO function select 5
//function select mask
//GPFSEL0 applies to GPIO0-GPIO9
//GPSEL1 applies to GPIO10-GPIO19
//GPSEL2 applies to GPIO20-GPIO29
//GPSEL3 applies to GPIO30-GPIO39
//GPSEL4 applies to GPIO40-GPIO49
//GPSEL5 applies to GPIO50-GPIO53
#define FINPUT 0
#define FOUTPUT 1
#define FUNC0 4
#define FUNC1 5
#define FUNC2 6
#define FUNC3 7
#define FUNC4 3
#define FUNC5 2
#define GPSET0 0x1C
#define GPSET1 0x20
#define GPCLR0 0x28
#define GPCLR1 0x2C
#define GPLEV0 0x34
#define GPLEV1 0x38


#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char ** argv) {
	int fd;
	unsigned int pin;
	useconds_t time_usec = 1000000;

	if (argc < 2 || argc > 3) {
		printf("gpioset sets gpio pin high for time_usec microseconds, default 1s\n");
		printf("Usage: gpioset <gpio_pin> [time_usec]\n");
		return -1;
	}

	if (!isdigit(argv[1][0])) {
		perror("GPIO pin is not a number");
		return -1;
	}
	pin = atoi(argv[1]);

	if (argc == 3) {
		if (!isdigit(argv[2][0])) {
			perror("time_usec is not a number");
			return -1;
		}
		time_usec = atoi(argv[2]);
	}
	//long pagesize = sysconf(_SC_PAGE_SIZE);
	//printf("pagesize %ld\n", pagesize);

	fd = open(DEVICE, O_RDWR | O_SYNC);

	if (fd < 0) {
		printf("error opening device %s\n", DEVICE); return -1;
	}

	char * gpio_reg = mmap(NULL, GPIOMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!gpio_reg) {
		perror("mmap failed");
		goto exit_on_error;
	}
	//printf("gpio_reg %lu\n", (uint64_t)gpio_reg);

	char *fsel_reg, *set_reg, *clr_reg, *get_reg;
	unsigned int fsel_mask, fsel_val = 0, shift, val, v;
/*
	for (int i = 0; i < 6; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPFSEL%d %u\n", i, val);
	}
	for (int i = 13; i < 15; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPLEV %u\n", val);
	}
	for (int i = 16; i < 18; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPEDS %u\n", val);
	}
	for (int i = 19; i < 21; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPREN %u\n", val);
	}
	for (int i = 22; i < 24; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPFEN %u\n", val);
	}
	for (int i = 25; i < 27; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPHEN %u\n", val);
	}
	for (int i = 28; i < 30; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPLEN %u\n", val);
	}
	for (int i = 31; i < 33; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPAREN %u\n", val);
	}
	for (int i = 34; i < 36; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPAFEN %u\n", val);
	}
	memcpy(&val, gpio_reg + 4 * 37, sizeof(val));
	printf("GPPUD %u\n", val);

	for (int i = 38; i < 40; i++) {
		memcpy(&val, gpio_reg + 4 * i, sizeof(val));
		printf("GPPUDCLK %u\n", val);
	}
	memcpy(&val, gpio_reg + 4 * 41, sizeof(val));
	printf("val %u\n", val);
*/
	switch (pin / 10) {
	case 0: fsel_reg = gpio_reg + GPFSEL0; break;
	case 1: fsel_reg = gpio_reg + GPFSEL1;	break;
	case 2: fsel_reg = gpio_reg + GPFSEL2; break;
	case 3: fsel_reg = gpio_reg + GPFSEL3; break;
	case 4: fsel_reg = gpio_reg + GPFSEL4; break;
	case 5: fsel_reg = gpio_reg + GPFSEL5; break;
	default: 
		perror("GPIO pin number range is from 0 to 53");
		goto exit_on_error;
	}
	//printf("fsel_reg %lu\n", (uint64_t)fsel_reg);

	shift = 3 * (pin % 10);
	//printf("shift %u\n", shift);
	fsel_mask = ~(7 << shift);
	//printf("fsel_mask %u\n", fsel_mask);

	memcpy(&fsel_val, fsel_reg, sizeof(fsel_val));
	//printf("fsel_val %u\n", fsel_val);
	fsel_val &= fsel_mask;
	//printf("fsel_val & fsel_mask %u\n", fsel_val);
	fsel_val |= (FOUTPUT << shift);
	//printf("fsel_val Ored with shifted FOUTPUT %u\n", fsel_val);
	if (pin < 32) {
		set_reg = gpio_reg + GPSET0;
		clr_reg = gpio_reg + GPCLR0;
		get_reg = gpio_reg + GPLEV0;
		val = 1 << pin;
	}
	else {
		set_reg = gpio_reg + GPSET1;
		clr_reg = gpio_reg + GPCLR1;
		get_reg = gpio_reg + GPLEV1;
		val = 1 << (pin - 32);
	}
	//printf("set_reg %lu\n", (uint64_t)set_reg);
	//printf("clr_reg %lu\n", (uint64_t)clr_reg);
	//printf("get_reg %lu\n", (uint64_t)get_reg);
	//printf("val %u\n", val);

	//setting GPIO pin as an output
	memcpy(fsel_reg, &fsel_val, sizeof(fsel_val));

	//setting GPIO pin high
	memcpy(set_reg, &val, sizeof(val));

	usleep(time_usec);

	//setting GPIO pin low
	memcpy(clr_reg, &val, sizeof(val));

	/*
	//setting GPIO pin as an input
	memcpy(&v, fsel_reg, sizeof(v));
	v &= fsel_mask;
	memcpy(fsel_reg, &v, sizeof(v));
		
	//read value v (0 or 1)
	memcpy(&v, get_reg, sizeof(v));
	v &= val;
	v >>= pin;
	*/

exit_on_error:
	if (gpio_reg) munmap(gpio_reg, GPIOMEM_SIZE);
	close(fd);
	return 0;
}
