#ifndef USBIPCMD_H
#define  USBIPCMD_H

#include "type.h"

#define __attribute__(x)
#pragma pack(1)

#ifndef SYSFS_BUS_ID_SIZE
#define SYSFS_BUS_ID_SIZE 32 
#endif
/*******************************************************
*
*							copy from usbip
*
********************************************************/
struct op_common {
	uint16_t version;

#define OP_REQUEST	(0x80 << 8)
#define OP_REPLY	(0x00 << 8)
	uint16_t code;

	/* add more error code */
#define ST_OK	0x00
#define ST_NA	0x01
	uint32_t status; /* op_code status (for reply) */
	uint32_t magic;

} __attribute__((packed));
/* ---------------------------------------------------------------------- */
/* Retrieve USB device information. (still not used) */
#define OP_DEVINFO	0x02
#define OP_REQ_DEVINFO	(OP_REQUEST | OP_DEVINFO)
#define OP_REP_DEVINFO	(OP_REPLY   | OP_DEVINFO)


struct op_devinfo_request {
	char busid[SYSFS_BUS_ID_SIZE];
} __attribute__((packed));
struct op_devinfo_reply {
	struct usb_device_info udev;
	struct usb_interface_info uinf[];
} __attribute__((packed));


/* ---------------------------------------------------------------------- */
/* Import a remote USB device. */
#define OP_IMPORT	0x03
#define OP_REQ_IMPORT	(OP_REQUEST | OP_IMPORT)
#define OP_REP_IMPORT   (OP_REPLY   | OP_IMPORT)

struct op_import_request{
	int	 portIndx; 
	char busid[SYSFS_BUS_ID_SIZE];
} __attribute__((packed));

struct op_import_reply {
	struct usb_device_info udev;
	//	struct usb_interface uinf[];
} __attribute__((packed));

#define PACK_OP_IMPORT_REQUEST(pack, request)  do {\
} while (0)

#define PACK_OP_IMPORT_REPLY(pack, reply)  do {\
	pack_usb_device(pack, &(reply)->udev);\
} while (0)

/* ---------------------------------------------------------------------- */
/* Retrieve the list of exported USB devices. */
#define OP_DEVLIST	0x05
#define OP_REQ_DEVLIST	(OP_REQUEST | OP_DEVLIST)
#define OP_REP_DEVLIST	(OP_REPLY   | OP_DEVLIST)


struct op_devlist_request {
} __attribute__((packed));

struct op_devlist_reply {
	uint32_t ndev;
	/* followed by reply_extra[] */
} __attribute__((packed));

struct op_devlist_reply_extra {
	struct usb_device_info    udev;
	struct usb_interface_info uinf[];
} __attribute__((packed));



#define OP_CONNET				0x40
#define OP_ON					0x1
#define OP_OFF					0x0
#define OP_CONNECT_ON			(OP_CONNET| OP_ON)
#define OP_CONNECT_OFF			(OP_CONNET| OP_OFF)

#define OP_REQ_CONNECT_ON		(OP_REQUEST|OP_CONNECT_ON)
#define OP_REQ_CONNECT_OFF		(OP_REQUEST|OP_CONNECT_OFF)
#define OP_REP_CONNECT_ON		(OP_REPLY | OP_CONNECT_ON)
#define OP_REP_CONNECT_OFF		(OP_REPLY | OP_CONNECT_OFF)

#define OP_DEVICE_ATTACH		(0x50)
#define OP_REP_DEVICE_ATTACH	(OP_REPLY | OP_DEVICE_ATTACH)
#define OP_REQ_DEVICE_ATTACH	(OP_REQUEST | OP_DEVICE_ATTACH)

#define OP_DEVICE_DETACH		(0x60)
#define OP_REP_DEVICE_DETACH	(OP_REPLY | OP_DEVICE_DETACH)
#define OP_REQ_DEVICE_DETACH	(OP_REQUEST | OP_DEVICE_DETACH)


struct op_deviceattach_request{
	uint16_t vid;
	uint16_t pid;
}__attribute__((packed));

struct op_devicedetach_request{
	uint16_t vid;
	uint16_t pid;
} __attribute__((packed));
#pragma pack()
#endif
