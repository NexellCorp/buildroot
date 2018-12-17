#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>

#define	MMAP_ALIGN		4096
#define	MMAP_DEVICE		"/dev/mem"

static void *iomem_map(unsigned long phys, unsigned long len, unsigned long *map_phys)
{
	void *virt;
	int fd;

	fd = open(MMAP_DEVICE, O_RDWR|O_SYNC);
	if (fd < 0) {
		printf("Fail, open %s, %s\n", MMAP_DEVICE, strerror(errno));
		return 0;
	}

	if (phys & (MMAP_ALIGN - 1))
		phys = (phys & ~(MMAP_ALIGN - 1));

	if (len & (MMAP_ALIGN - 1))
		len = (len & ~(MMAP_ALIGN - 1)) + MMAP_ALIGN;

	virt = mmap((void*)0,
			len,
			PROT_READ|PROT_WRITE, MAP_SHARED,
			fd,
			(off_t)phys);
	if ((long)virt == -1) {
		printf("Fail: map PV:0x%08x, Len:%d, %s \n",
			phys, len, strerror(errno));
		goto _err_map;
	}

	if (map_phys)
		*map_phys = phys;

_err_map:
	close(fd);
	return virt;
}

static void iomem_free(void *virt, unsigned long len)
{
	if (virt && len)
		munmap(virt, len);
}

void print_usage(void)
{
    printf(
	"usage: options\n"
       	"-a physical address (hex)  \n"
       	"-w write data (hex)  \n"
       	"-l read length (hex) \n"
	"-n line feed \n"
	);
}

#define readl(a)	(*(unsigned long *)(a))	
#define writel(v, a)	(*(unsigned long *)(a) = v)

unsigned long disable_addr[36] = {  0x27000420, 0x27000820, 0x27001a20, 0x27002220,                 
				0x27002420, 0x27002620 ,0x27002820, 0x27003020,
				0x27003220, 0x27003420, 0x27003620, 0x27003820,
				0x27003a20, 0x27003c20, 0x27003e20, 0x27004020,
				0x27004020, 0x27004020, 0x27004020, 0x27005620,
				0x27005820, 0x27005a20, 0x27005c20, 0x27005e20,
				0x27006020, 0x27006220, 0x27006420, 0x27006620,
				0x2700a820, 0x27006e20, 0x27007020, 0x27007220,
				0x27007420, 0x27007620, 0x27007a20, 0x27007c20
};



//int main(int argc, char **argv)
int set_cmu(void)
{
	int opt;
	unsigned long addr = 0, data = 0, size = 4;
	unsigned long map_phys = 0;
	void *virt;
	bool wop = false, lfeed = false;
	int i;

	printf(" Disable CMU SRC\n");
	for (i = 0; i < 36; i++) {
		//virt = iomem_map(disable_addr[i], size, &map_phys);
		virt = iomem_map(disable_addr[i], size, &map_phys);
		if (!virt)
			return -EINVAL;
	
		writel(0xff, virt + (disable_addr[i] - map_phys));
		iomem_free(virt, 4);
	}
	return 0;
}
