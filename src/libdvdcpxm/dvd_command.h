#pragma once

#pragma warning(disable:4200)

#include <stdint.h>
#include <windows.h>
#include <ntddscsi.h>

#include "b2n.h"
#include "dvd_device.h"

#define DVD_KEY_SIZE           5
#define DVD_CHALLENGE_SIZE    10
#define DVD_DISCKEY_SIZE    2048

#define GPCMD_READ_DISC_STRUCTURE 0xad
#define GPCMD_REPORT_KEY          0xa4
#define GPCMD_SEND_KEY            0xa3

#define DVD_STRUCT_PHYSICAL      0x00
#define DVD_STRUCT_COPYRIGHT     0x01
#define DVD_STRUCT_DISCKEY       0x02
#define DVD_STRUCT_BCA           0x03
#define DVD_STRUCT_MANUFACT      0x04

#define DVD_REPORT_AGID_CSSCPPM  0x00
#define DVD_REPORT_CHALLENGE     0x01
#define DVD_SEND_CHALLENGE       0x01
#define DVD_REPORT_KEY1          0x02
#define DVD_SEND_KEY2            0x03
#define DVD_REPORT_TITLE_KEY     0x04
#define DVD_REPORT_ASF           0x05
#define DVD_SEND_RPC             0x06
#define DVD_REPORT_RPC           0x08
#define DVD_REPORT_AGID_CPRM     0x11
#define DVD_INVALIDATE_AGID      0x3f

#define DVD_CHALLENGE_KEY_LENGTH        (12 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_BUS_KEY_LENGTH              (8 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_TITLE_KEY_LENGTH            (8 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_DISC_KEY_LENGTH             (2048 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_RPC_KEY_LENGTH              (sizeof(DVD_RPC_KEY) + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_ASF_LENGTH                  (sizeof(DVD_ASF) + sizeof(DVD_COPY_PROTECT_KEY))

#define IOCTL_DVD_START_SESSION         CTL_CODE(FILE_DEVICE_DVD, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_READ_KEY              CTL_CODE(FILE_DEVICE_DVD, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY              CTL_CODE(FILE_DEVICE_DVD, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_END_SESSION           CTL_CODE(FILE_DEVICE_DVD, 0x0403, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_GET_REGION            CTL_CODE(FILE_DEVICE_DVD, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY2             CTL_CODE(FILE_DEVICE_DVD, 0x0406, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DVD_READ_STRUCTURE        CTL_CODE(FILE_DEVICE_DVD, 0x0450, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef uint32_t DVD_SESSION_ID, *PDVD_SESSION_ID;

typedef enum DVD_STRUCTURE_FORMAT
{
	DvdPhysicalDescriptor,
	DvdCopyrightDescriptor,
	DvdDiskKeyDescriptor,
	DvdBCADescriptor,
	DvdManufacturerDescriptor,
	DvdMaxDescriptor
} DVD_STRUCTURE_FORMAT, *PDVD_STRUCTURE_FORMAT;

typedef struct DVD_READ_STRUCTURE
{
	LARGE_INTEGER        BlockByteOffset;
	DVD_STRUCTURE_FORMAT Format;
	DVD_SESSION_ID       SessionId;
	uint8_t              LayerNumber;
} DVD_READ_STRUCTURE, *PDVD_READ_STRUCTURE;

typedef struct _DVD_PHYSICAL_DESCRIPTOR
{
	uint8_t  PartVersion:4;
	uint8_t  BookType:4;
	uint8_t  MaximumRate:4;
	uint8_t  DiscSize:4;
	uint8_t  LayerType:4;
	uint8_t  TrackPath:1;
	uint8_t  NumberOfLayers:2;
	uint8_t  Reserved1:1;
	uint8_t  TrackDensity:4;
	uint8_t  LinearDensity:4;
	uint32_t StartPhysicalSectorOfDataArea;
	uint32_t EndPhysicalSectorOfDataArea;
	uint32_t EndPhysicalSectorOfLayer0;
	uint8_t  Reserved2:7;
	uint8_t  BCAFlag:1;
} DVD_PHYSICAL_DESCRIPTOR, *PDVD_PHYSICAL_DESCRIPTOR;

typedef struct _DVD_COPYRIGHT_DESCRIPTOR
{
	uint8_t CopyrightProtectionType;
	uint8_t RegionManagementInformation;
	USHORT  Reserved;
} DVD_COPYRIGHT_DESCRIPTOR, *PDVD_COPYRIGHT_DESCRIPTOR;

typedef enum
{
	DvdChallengeKey   = 0x01,
	DvdBusKey1,
	DvdBusKey2,
	DvdTitleKey,
	DvdAsf,
	DvdSetRpcKey      = 0x6,
	DvdGetRpcKey      = 0x8,
	DvdDiscKey        = 0x80,
	DvdInvalidateAGID = 0x3f
} DVD_KEY_TYPE;

typedef enum
{
	DvdProtNone,
	DvdProtCSSCPPM,
	DvdProtCPRM
} DVD_PROT_TYPE;

typedef struct _DVD_COPY_PROTECT_KEY
{
	uint32_t        KeyLength;
	DVD_SESSION_ID  SessionId;
	DVD_KEY_TYPE    KeyType;
	uint32_t        KeyFlags;
	union {
		struct {
			uint32_t    FileHandle;
			uint32_t    Reserved; // used for NT alignment
		};
		LARGE_INTEGER TitleOffset;
	}               Parameters;
	uint8_t         KeyData[0];
} DVD_COPY_PROTECT_KEY, *PDVD_COPY_PROTECT_KEY;

typedef struct _DVD_ASF
{
	uint8_t Reserved0[3];
	uint8_t SuccessFlag:1;
	uint8_t Reserved1:7;
} DVD_ASF, *PDVD_ASF;

typedef struct _DVD_RPC_KEY
{
	uint8_t UserResetsAvailable:3;
	uint8_t ManufacturerResetsAvailable:3;
	uint8_t TypeCode:2;
	uint8_t RegionMask;
	uint8_t RpcScheme;
	uint8_t Reserved2[1];
} DVD_RPC_KEY, *PDVD_RPC_KEY;

extern HANDLE h_dvd;
extern SCSI_PASS_THROUGH_DIRECT sptd;
extern uint8_t *sptd_buf;

extern void ioctl_Init           (int i_type, int i_size);
extern int  ioctl_Send           (int *p_bytes);
extern int  ioctl_ReadPhysical   (int i_layer, PDVD_PHYSICAL_DESCRIPTOR  p_physical);
extern int  ioctl_ReadCopyright  (int i_layer, PDVD_COPYRIGHT_DESCRIPTOR p_copyright);
extern int  ioctl_ReadDiscKey    (int *p_agid, uint8_t *p_key);
extern int  ioctl_ReadTitleKey   (int *p_agid, int i_pos, uint8_t *p_key);
extern int  ioctl_ReportAgid     (int *p_protection, int *p_agid);
extern int  ioctl_ReportChallenge(int *p_agid, uint8_t *p_challenge);
extern int  ioctl_ReportASF      (int *p_remove_me, int *p_asf);
extern int  ioctl_ReportKey1     (int *p_agid, uint8_t *p_key);
extern int  ioctl_InvalidateAgid (int *p_agid);
extern int  ioctl_SendChallenge  (int *p_agid, uint8_t *p_challenge);
extern int  ioctl_SendKey2       (int *p_agid, uint8_t *p_key);
extern int  ioctl_ReportRPC      (int *p_type, int *p_mask, int *p_scheme);
extern int  ioctl_ReadCPRMMediaId(int *p_agid, uint8_t *p_media_id);
extern int  ioctl_ReadCPRMMKBPack(int *p_agid, int mkb_pack, uint8_t *p_mkb_pack, int *p_total_packs);
