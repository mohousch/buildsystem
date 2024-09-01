#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include "usb.h"

#include "linux/fb.h"

#include "jpg.h"

static const char *progname = "minimon";

static int need_switch = 0;
static int have_idx = -1;

typedef struct
{
	int mass_id;
	int custom_id;
	const char *name;
	int width;
	int height;
} id_info_t;

// there are many more,
static id_info_t ids[] =
{
	{0x200a, 0x200b, "SPF-72H", 800, 480},
	{0x200e, 0x200f, "SPF-75H/76H", 800, 480},
	{0x200c, 0x200d, "SPF-83H", 800, 600},
	{0x2012, 0x2013, "SPF-85H/86H", 800, 600},
	{0x2016, 0x2017, "SPF-85P/86P", 800, 600},
	{0x2025, 0x2026, "SPF-87H", 800, 480},
	{0x2033, 0x2034, "SPF-87H v2", 800, 480},
	{0x201c, 0x201b, "SPF-105P", 1024, 600},
	{0x2027, 0x2028, "SPF-107H", 1024, 600},
	{0x2035, 0x2036, "SPF-107H v2", 1024, 600},
	{0x204f, 0x2050, "SPF-700T", 800, 600},
	{0x2039, 0x2040, "SPF-1000P", 1024, 600},
	{0, 0, } // end-of-list
};

static int in_list(int id, id_info_t *list)
{
	if (!list)
		return 0;

	int idx = 0;
	while (list->mass_id || list->custom_id)
	{
		if (id == list->mass_id)
		{
			// still in mass-storage mode, need to switch
			need_switch = 1;
			return idx;
		}
		else if (id == list->custom_id)
		{
			need_switch = 0;
			return idx;
		}
		idx++;
		list++;
	}

	return -1;
}

static struct usb_device *find_dev()
{
	struct usb_bus *bus;
	struct usb_device *dev;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for (bus = usb_busses; bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor == 0x04e8)
			{
				// found a Samsung device, good
				int idx = -1;
				if ((idx = in_list(dev->descriptor.idProduct, ids)) >= 0)
				{
					have_idx = idx;
					return dev;
				}
			}
		}
	}

	return NULL;
}

static usb_dev_handle *dev_open(struct usb_device *dev)
{
	int res = -1;
	usb_dev_handle *udev;
	int numeps = 0;

	udev = usb_open(dev);
	if (!udev)
	{
		fprintf(stderr, "%s: failed to open device, exit.\n", progname);
		exit(EXIT_FAILURE);
	}

//  setuid(getuid());

	res = usb_set_configuration(udev, 1);

	usb_claim_interface(udev, 0);
	numeps = dev->config[0].interface[0].altsetting[0].bNumEndpoints;
	if (numeps == 0)
	{
		fprintf(stderr, "%s: no endpoints, exit.\n", progname);
		exit(EXIT_FAILURE);
	}

	{
		int eplist[] = { 0x2, 0x81, 0x83 };
		int eplength = sizeof(eplist)/sizeof(eplist[0]);
		int *endpoint = eplist;
		int i;
		for (i = 0; i < eplength; i++)
		{
			res = usb_resetep(udev, *endpoint);
			res = usb_clear_halt(udev, *endpoint);
			endpoint++;
		}
	}

	return udev;
}

static int send_jpg(jpg_buf_t *jpg_buf, usb_dev_handle *udev)
{
#define URBBUF_MAX 0x20000
	char buf[URBBUF_MAX];

#define HDR_LEN 12
	char hdr[HDR_LEN] = {0xa5, 0x5a, 0x18, 0x04, 0xff, 0xff, 0xff, 0xff, 0x48, 0x00, 0x00, 0x00};
	*(int *)(hdr+4) = jpg_buf->size;

	memcpy(buf, hdr, HDR_LEN);
	int off = HDR_LEN;
	int jpg_off = 0;
	int jpg_left = jpg_buf->size;
	while (jpg_left > 0)
	{
		int nr = MIN(URBBUF_MAX - off, jpg_left);
		memcpy(buf+off, &jpg_buf->ptr[jpg_off], nr);
		// pad
		memset(buf + off + nr, 0, URBBUF_MAX - off - nr);

		// write it out chunk by chunk
		int timeout = 1000;
		int endpoint = 0x2;
		int res = usb_bulk_write(udev, endpoint, buf, URBBUF_MAX, timeout);

		if (res < 0)
			return 0;
		off = 0; // no header on subsequent chunks
		jpg_off += nr;
		jpg_left -= nr;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s </dev/fbX>\n", progname);
		return EXIT_FAILURE;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("minimon framebuffer");
		exit(EXIT_FAILURE);
	}

	struct fb_fix_screeninfo sif;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &sif) < 0)
	{
		perror("minimon framebuffer info");
		exit(EXIT_FAILURE);
	}

	printf("id %s\n", sif.id);
	printf("type %d, aux %d\n", sif.type, sif.type_aux);
	printf("visual %d\n", sif.visual);
	printf("accel %d\n", sif.accel);
	printf("line length %d\n", sif.line_length);
	printf("mem %d\n", sif.smem_len);

	struct fb_var_screeninfo siv;
	if (ioctl(fd, FBIOGET_VSCREENINFO, &siv) < 0)
	{
		perror("minimon framebuffer info");
		exit(EXIT_FAILURE);
	}

	printf("res x %d y %d\n", siv.xres, siv.yres);
	printf("bpp %d\n", siv.bits_per_pixel);

	while (1)
	{
		struct usb_device *dev = find_dev(index);
		if (!dev)
		{
			fprintf(stderr, "%s: no photo frame device found, suspending...\n", progname);
			sleep(1);
			continue;
		}

		if (need_switch)
		{
			fprintf(stderr, "%s: found %s, trying to switch to custom product mode...\n",
			        ids[have_idx].name, progname);

			usb_dev_handle *udev;
			udev = usb_open(dev);
			if (!udev)
			{
				fprintf(stderr, "%s: failed to open device, exit.\n", progname);
				exit(EXIT_FAILURE);
			}

			char buf[254];
			memset(buf, 0, 254);
			int res = usb_control_msg(udev, USB_TYPE_STANDARD | USB_ENDPOINT_IN,
			                          0x06, 0xfe, 0xfe, buf, 0xfe, 1000);
			fprintf(stderr, "%s: usb_control_msg() = %d\n", progname, res);
			usb_close(udev);
			sleep(1);
		}

		dev = find_dev(index);
		if (!dev || need_switch)
		{
			fprintf(stderr, "%s: no photo frame device found, suspending...\n", progname);
			sleep(1);
			continue;
		}

		int mon_width = ids[have_idx].width;
		int mon_height = ids[have_idx].height;
		fprintf(stderr, "%s: found %s (%d x %d)\n",
		        progname, ids[have_idx].name, mon_width, mon_height);

		siv.xres = siv.xres_virtual = mon_width;
		siv.yres = siv.yres_virtual = mon_height;

		if (ioctl(fd, FBIOPUT_VSCREENINFO, &siv)<0)
		{
			perror("FBIOPUT_VSCREENINFO");
			exit(EXIT_FAILURE);
		}

		printf("resized fb to x %d y %d\n", siv.xres, siv.yres);
		if (ioctl(fd, FBIOGET_FSCREENINFO, &sif) < 0)
		{
			perror("minimon framebuffer info");
			exit(EXIT_FAILURE);
		}

		printf("resized line length %d\n", sif.line_length);
		printf("resized mem %d\n", sif.smem_len);

		size_t fb_mem_size = siv.xres * siv.yres * siv.bits_per_pixel / 8;
		void *fb_mem = mmap(NULL, fb_mem_size, PROT_READ, MAP_SHARED, fd, 0);
		if (fb_mem == MAP_FAILED)
		{
			perror("minimon framebuffer mapping");
			exit(EXIT_FAILURE);
		}

		usb_dev_handle *udev = dev_open(dev);

		while (1)
		{
			int transfer = 1;

			// poll status to avoid going back to photoframe mode
			char buf[2];
			int res = usb_control_msg(udev, USB_TYPE_VENDOR | USB_ENDPOINT_IN,
			                          0x06, 0x0, 0x0, buf, 0x2, 1000);
			if (res != 2)
			{
				break;
			}
			else if (buf[0] != 0)
			{
				transfer = 0;
			}

			if (transfer)
			{
				fprintf(stderr, ".");
				jpg_buf_t jpg_buf = build_jpg_from_fb((unsigned char *)fb_mem, siv.xres, siv.yres, siv.bits_per_pixel, mon_width, mon_height);

				if (!send_jpg(&jpg_buf, udev))
				{
					free(jpg_buf.ptr);
					break;
				}
				free(jpg_buf.ptr);
			}
			else
				fprintf(stderr, "o");

			usleep(500000);
		}

		usb_close(udev);
		munmap(fb_mem, fb_mem_size);
	}

	close(fd);

	return EXIT_SUCCESS;
}
