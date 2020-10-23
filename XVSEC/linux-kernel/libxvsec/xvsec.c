/*
 * This file is part of the XVSEC userspace library which provides the
 * userspace APIs to enable the XSEC driver functionality
 *
 * Copyright (c) 2018-2020  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#include "version.h"
#include "xvsec.h"
#include "xvsec_int.h"
#include "xvsec_drv.h"  /* kernel character Driver layer API's*/
#include "xvsec_mcap.h" /* kernel layer MCAP API's */



#define XVSEC_MAGIC_NO		0x10EE
#define INVALID_DEVICE_INDEX	0xFFFF

const static char version[] =
	XVSEC_LIB_MODULE_DESC "\t: v" XVSEC_LIB_VERSION "\n";

xvsec_user_context_t	*xvsec_user_ctx = NULL;
int no_of_devs;

const char *error_codes[] = {
	"XVSEC Success",
	"XVSEC Operation not permitted",
	"XVSEC Failure",
	"XVSEC NULL Pointer",
	"XVSEC Invalid Parameters",
	"XVSEC Mutex Lock Fail",
	"XVSEC Mutex Unlock Fail",
	"XVSEC MEM Allocation Fail",
	"XVSEC Operation Not Supported",
	"XVSEC Bitstream Program Fail",
	"XVSEC Linux System Call Fail",
	"XVSEC Max Devices Limit Reached",
	"XVSEC Invalid File Format",
	"XVSEC Invalid Combo of Offset and Access Type",
	"XVSEC Missing Capability ID",
	"XVSEC Capability ID Not Supported",
	"XVSEC Invalid FPGA CFG Register",
	"XVSEC Invalid VSEC Offset"
};

int xvsec_lib_init(int max_devices)
{
	int			ret = XVSEC_SUCCESS;
	int			status;
	int			index;
	uint16_t		failed_index;
	pthread_mutexattr_t	attr;

	no_of_devs = max_devices;
	xvsec_user_ctx = malloc(sizeof(xvsec_user_context_t) * no_of_devs);
	if(xvsec_user_ctx == NULL)
	{
		fprintf(stderr, "[XVSEC] : Failed to Allocate "
			"memory for user context\n");
		return XVSEC_ERR_MEM_ALLOC_FAILED;
	}

	memset(xvsec_user_ctx, 0, sizeof(xvsec_user_context_t));

	status = pthread_mutexattr_init(&attr);
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : Failed to initialize an attribute"
			" object: %s\n", strerror(status));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP0;
	}

	status = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : Failed to set attribute "
			"ERRORCHECK MUTEX: %s\n", strerror(status));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP1;
	}

	for(index = 0; index < no_of_devs; index++)
	{
		status = pthread_mutex_init(&xvsec_user_ctx[index].mutex, &attr);
		if (status < 0)
		{
			fprintf(stderr, "[XVSEC] : mutex init failed for "
				"device %d\n", index);
			ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
			failed_index = index;
			goto CLEANUP2;
		}
	}

	pthread_mutexattr_destroy(&attr);

	return XVSEC_SUCCESS;

CLEANUP2:
	for(index = 0; index <= failed_index; index++)
	{
		pthread_mutex_destroy(&xvsec_user_ctx[index].mutex);
	}
CLEANUP1:
	pthread_mutexattr_destroy(&attr);
CLEANUP0:
	free(xvsec_user_ctx);
	xvsec_user_ctx = NULL;
	return ret;
}

int xvsec_lib_deinit(void)
{
	int		ret = XVSEC_SUCCESS;
	int		status;
	uint32_t	index;

	for(index = 0; index < no_of_devs; index++)
	{
		status = pthread_mutex_destroy(&xvsec_user_ctx[index].mutex);
		if(status < 0)
		{
			fprintf(stderr, "[XVSEC] : pthread_mutex_destroy "
				"failed with error %d(%s) for index %u\n",
				status, strerror(errno), index);
			ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		}
	}

	free(xvsec_user_ctx);
	xvsec_user_ctx = NULL;

	return ret;
}

int xvsec_open(uint16_t bus_no, uint16_t dev_no, xvsec_handle_t *handle, char* dev_str)
{
	int        ret = XVSEC_SUCCESS;
	int        status;
	char       device_name[20];
	int        fd;
	uint32_t   index;
	uint32_t   device_index = INVALID_DEVICE_INDEX;

	/* Parameter checking */
	if((bus_no > 255) || (dev_no > 255) || (handle == NULL) || (dev_str == NULL))
	{
		fprintf(stderr, "[XVSEC] : %s : Invalid Parameters "
			"bus_no:0x%04X, device no : 0x%04X, handle : %p, dev_str:%p\n",
			__func__, bus_no, dev_no, handle, dev_str);

		return XVSEC_ERR_INVALID_PARAM;
	}

	for(index = 0; index < no_of_devs; index++)
	{
		if(xvsec_user_ctx[index].handle == NULL)
		{
			device_index = index;
			index = no_of_devs;
		}
	}

	if(device_index == INVALID_DEVICE_INDEX)
	{
		fprintf(stderr, "[XVSEC] : %s : There is no room to "
			"open a device\n", __func__);
		return XVSEC_MAX_DEVICES_LIMIT_REACHED;
	}

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = snprintf(device_name, sizeof(device_name),
		"/dev/xvsec%02X%02X%s", bus_no, dev_no, dev_str);
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : snprintf returned error "
			": %d\n", __func__, status);
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}
	else if((status == sizeof(device_name)) ||
		(status >= sizeof(device_name)))
	{
		fprintf(stderr, "[XVSEC] : %s : snprintf output truncated :"
			" %d\n", __func__, status);
		ret = XVSEC_ERR_OPERATION_NOT_SUPPORTED;
		goto CLEANUP;
	}

	fd = open(device_name, O_SYNC|O_RDWR, S_IRWXU);
	if(fd < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : open call failed on device %s "
			"with error %d(%s)\n", __func__, device_name, errno,
			strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}


	xvsec_user_ctx[device_index].fd		= fd;
	xvsec_user_ctx[device_index].handle	= handle;

	((handle_t *)handle)->xvsec_magic_no	= XVSEC_MAGIC_NO;
	((handle_t *)handle)->bus_no		= bus_no;
	((handle_t *)handle)->dev_no		= dev_no;
	((handle_t *)handle)->index		= device_index;
	((handle_t *)handle)->mrev		= 0xFF;
	((handle_t *)handle)->valid		= true;

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_close(xvsec_handle_t *handle)
{
	int ret = XVSEC_SUCCESS;
	int status;
	uint16_t device_index;

	if(handle == NULL)
	{
	fprintf(stderr, "[XVSEC] : %s : Invalid Handle : "
		"handle = %p\n", __func__, handle);
	return XVSEC_ERR_NULL_POINTER;
	}

	device_index = ((handle_t *)handle)->index;

	if(device_index >= no_of_devs)
	{
		fprintf(stderr, "[XVSEC] : %s : handle corrupted,"
			" handle = 0x%lX\n", __func__, *handle);
		return XVSEC_ERR_OPERATION_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = close(xvsec_user_ctx[device_index].fd);
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : close() failed with "
			"error %d(%s)\n", __func__, errno, strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
	}

	xvsec_user_ctx[device_index].fd = -1;
	xvsec_user_ctx[device_index].handle = NULL;
	memset(handle, 0, sizeof(xvsec_handle_t));

	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_get_cap_list(xvsec_handle_t *handle, xvsec_cap_list_t *cap_list)
{
	int				ret = XVSEC_SUCCESS;
	int				status;
	uint16_t			index;
	uint16_t			device_index;
	struct xvsec_capabilities	xvsec_caps;

	/* Parameter Validation */
	if((handle == NULL) || (cap_list == NULL))
	{
		return XVSEC_ERR_INVALID_PARAM;
	}

	status = xvsec_validate_handle(handle);
	if(status < 0)
	{
		return status;
	}

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(&xvsec_caps, 0, sizeof(struct xvsec_capabilities));

	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_XVSEC_GET_CAP_LIST, &xvsec_caps);
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_XVSEC_GET_CAP_LIST failed with error %d(%s)\n",
			__func__, errno, strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}

	cap_list->no_of_caps = (xvsec_caps.no_of_caps > MAX_CAPS_SUPPORTED) ?
		MAX_CAPS_SUPPORTED : xvsec_caps.no_of_caps;

	for(index = 0; index < cap_list->no_of_caps; index++)
	{
		cap_list->cap_info[index].is_supported = xvsec_caps.vsec_info[index].is_supported;
		cap_list->cap_info[index].cap_id = xvsec_caps.vsec_info[index].cap_id;
		cap_list->cap_info[index].rev_id = xvsec_caps.vsec_info[index].rev_id;

		snprintf(cap_list->cap_info[index].cap_name, XVSEC_MAX_VSEC_STR_LEN,
			"%s", xvsec_caps.vsec_info[index].name);
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_show_device(xvsec_handle_t *handle)
{
	int			ret = XVSEC_SUCCESS;
	int			status;
	int			device_index;
	union device_info	dev_info;

	/* Parameter Validation */
	if(handle == NULL)
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_XVSEC_GET_DEVICE_INFO, &dev_info);
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_GET_DEVICE_INFO failed with error %d(%s)\n",
			__func__, errno, strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}

	fprintf(stdout, "%s", version);
	fprintf(stdout, "-----------------------------------\n");

	fprintf(stdout, "vendor_id        = 0x%X\n", dev_info.vendor_id       );
	fprintf(stdout, "device_id        = 0x%X\n", dev_info.device_id       );
	fprintf(stdout, "device_no        = 0x%X\n", dev_info.device_no       );
	fprintf(stdout, "device_fn        = 0x%X\n", dev_info.device_fn       );
	fprintf(stdout, "subsystem_vendor = 0x%X\n", dev_info.subsystem_vendor);
	fprintf(stdout, "subsystem_device = 0x%X\n", dev_info.subsystem_device);
	fprintf(stdout, "class_id         = 0x%X\n", dev_info.class_id        );
	fprintf(stdout, "cfg_size         = 0x%X\n", dev_info.cfg_size        );
	fprintf(stdout, "is_msi_enabled   = 0x%X\n", dev_info.is_msi_enabled  );
	fprintf(stdout, "is_msix_enabled  = 0x%X\n", dev_info.is_msix_enabled );

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_validate_handle(xvsec_handle_t *handle)
{
	uint16_t device_index;

	device_index = ((handle_t *)handle)->index;
	if(device_index >= no_of_devs)
	{
		fprintf(stderr, "[XVSEC] : %s : corrupted Handle\n", __func__);
		return XVSEC_ERR_OPERATION_NOT_SUPPORTED;
	}

	if(xvsec_user_ctx[device_index].handle != handle)
	{
		fprintf(stderr, "[XVSEC] : %s : Invalid Handle\n", __func__);
		return XVSEC_ERR_OPERATION_NOT_SUPPORTED;
	}
	return XVSEC_SUCCESS;
}
