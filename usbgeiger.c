/*
 * Implementation for USB-GEIGER V2(Strawberry Linux Co.,Ltd. USB-Geiger Counter) with HIDAPI
 *
 * Copyright (c) 2021 stzlab
 *
 * This software is released under the MIT License, see LICENSE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <hidapi/hidapi.h>

#define USBGEIGER_VENDOR_ID			0x1774
#define USBGEIGER_PRODUCT_ID		0x1002

#define USBGEIGER_BUFFER_SIZE		7

static int debug = 0;

struct usbgeiger_value {
	unsigned char dev_count[4];
	unsigned char time[3];
};

struct usbgeiger_firmware_version {
	unsigned char year;
	unsigned char month;
	unsigned char date;
	unsigned char reserved[4];
};

void hex_dump(const void* buf, int size)
{
	int i;
	unsigned char* ptr;

	ptr = (unsigned char*)buf;
	for (i = 0; i < size; i++) {
		fprintf(stderr, "%02hhx ", *ptr++);
	}
	fprintf(stderr, "\n");
}

int usbgeiger_read_sensor(hid_device* dev, struct usbgeiger_value* value, int clear_flag)
{
	int result;
	int read_size;
	unsigned char buffer[USBGEIGER_BUFFER_SIZE];

	memset(buffer, 0, sizeof(buffer));
	buffer[0] = 0x00; // Report ID

	 // Initialize counter
	if(clear_flag) {
		buffer[1] = 0xaa;
	}

	result = hid_write(dev, buffer, sizeof(buffer));
	if (debug) {
		fprintf(stderr, "debug: hid_write: ");
		hex_dump(buffer, sizeof(buffer));
	}
	if (result < 0) {
		fprintf(stderr, "error: hid_write: %ls\n", hid_error(dev));
		return -1;
	}

	read_size = hid_read_timeout(dev, (unsigned char*)value, sizeof(*value), 5000);
	if (read_size < 0) {
		fprintf(stderr, "error: hid_read_timeout: %ls\n", hid_error(dev));
		return -1;
	}
	if (debug) {
		fprintf(stderr, "debug: usbgeiger_read_sensor: ");
		hex_dump(value, read_size);
	}

	return (read_size == sizeof(*value)) ? 0 : -1;
}

int usbgeiger_get_version(hid_device* dev, struct usbgeiger_firmware_version* version)
{
	int read_size;

	read_size = hid_get_feature_report(dev, (unsigned char*)version, sizeof(*version));
	if (debug) {
		fprintf(stderr, "debug: hid_get_feature_report: ");
		hex_dump(version, read_size);
	}
	if (read_size < 0) {
		fprintf(stderr, "error: hid_get_feature_report: %ls", hid_error(dev));
		return -1;
	}
	return 0;
}

	int usbgeiger_calc_dev_count(struct usbgeiger_value* value)
{
	int dev_count;

	dev_count = (value->dev_count[3]) << 24 | (value->dev_count[2]) << 16 | (value->dev_count[1]) << 8 | value->dev_count[0];

	return dev_count;
}

int usbgeiger_calc_time(struct usbgeiger_value* value)
{
	int time;

	time = (value->time[2]) << 16 | (value->time[1]) << 8 | value->time[0];

	return time;
}

void usage()
{
	printf("USB-GEIGER with HDAPI 1.0\n");
	printf("Usage: usbgeiger [-dlfVRGH]\n");
	printf("  -d  : Enable debugging\n");
	printf("  -h  : Show usage\n");
	printf("  -l  : Show device list\n");
	printf("  -sn : Specify device number (n=0:all)\n");
	printf("  -V  : Show firmware version\n");
	printf("  -C  : Clear dev_counter\n");
}

int main(int argc, char* argv[])
{
	unsigned int opt;

	int result;

	int show_devlist;
	int show_version;

	int dev_number;
	int dev_count;
	int proc_count;

	int clear_flag;

	time_t now;
	struct tm *tm_now;

	hid_device* dev;
	struct hid_device_info* devinfo;
	struct hid_device_info* current;

	struct usbgeiger_value value;
	struct usbgeiger_firmware_version version;

	show_devlist   = 0;
	show_version   = 0;
	dev_number     = 0;
	clear_flag     = 0;

	while((opt = getopt(argc, argv,"ls:VhdC")) != -1){
		switch(opt){
			case 'l':
				show_devlist = 1;
				break;
			case 's':
				dev_number = atoi(optarg);
				break;
			case 'V':
				show_version = 1;
				break;
			case 'h':
				usage();
				exit(0);
				break;
			case 'd':
				debug = 1;
				break;
			case 'C':
				clear_flag = 1;
				break;
			default:
				fprintf(stderr, "error: invalid option\n");
				exit(1);
				break;
		}
	}

	result = hid_init();
	if (result != 0) {
		fprintf(stderr, "error: hid_init: %d\n", result);
 		exit(1);
	}

	result = 0;
	dev_count = 0;
	proc_count = 0;
	devinfo = hid_enumerate(USBGEIGER_VENDOR_ID, USBGEIGER_PRODUCT_ID);
	current = devinfo;
	while (current) {
		dev_count++;
		if (!show_devlist) {
			if (debug) {
				fprintf(stderr, "debug: devicenumber: %d\n", dev_count);
				fprintf(stderr, "debug: path: %s\n", current->path);
			}
			if (dev_number == 0 || dev_count == dev_number) {
				if (proc_count == 0) {
					now = time(NULL);
					tm_now = localtime(&now);
					printf("tm:%04d/%02d/%02d-%02d:%02d:%02d ", tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
								    tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
				}

				dev = hid_open_path(current->path);
				proc_count++;
				if (!dev) {
					fprintf(stderr, "error: hid_open_path: %ls\n", hid_error(dev));
					result = 1;
				}

				if(show_version){
					if(usbgeiger_get_version(dev, &version) != 0) {
						fprintf(stderr, "error: USBGEIGER_get_version\n");
						result = 1;
					} else {
						printf("v%d:%02d/%02d/%02d ", dev_count, version.year, version.month, version.date);
					}
				}

				if (usbgeiger_read_sensor(dev, &value, clear_flag) != 0) {
					fprintf(stderr, "error: usbgeiger_read_sensor\n");
					result = 1;
				} else {
					printf("ec%d:%d et%d:%d ", dev_count, usbgeiger_calc_dev_count(&value), dev_count, usbgeiger_calc_time(&value));
				}

				hid_close(dev);
			}
		} else {
			if (debug) {
				fprintf(stderr, "debug: DeviceNumber      : %d\n",    dev_count);
				fprintf(stderr, "debug: Path              : %s\n",    current->path);
				fprintf(stderr, "debug: VendorID          : %04hx\n", current->vendor_id);
				fprintf(stderr, "debug: ProductID         : %04hx\n", current->product_id);
				fprintf(stderr, "debug: SerialNumber      : %ls\n",   current->serial_number);
				fprintf(stderr, "debug: ReleaseNumber     : %hx\n",   current->release_number);
				fprintf(stderr, "debug: ManufacturerString: %ls\n",   current->manufacturer_string);
				fprintf(stderr, "debug: ProductString     : %ls\n",   current->product_string);
				fprintf(stderr, "debug: InterfaceNumber   : %d\n",    current->interface_number);
			}
			printf("%d:%s\n", dev_count, current->path);
		}
		current = current->next;
	}

	if (!show_devlist) {
		if (proc_count == 0) {
			fprintf(stderr, "error: device not found\n");
			result = 1;
		} else {
			printf("\n");
		}
	} else {
		printf("%d device(s) found\n", dev_count);
	}

	hid_free_enumeration(devinfo);
	result = hid_exit();
	if (result != 0) {
		fprintf(stderr, "error: hid_exit: %d\n", result);
	}

	exit(result);
}
