/**HEADER********************************************************************
* 
* Copyright (c) 2011 Freescale Semiconductor;
* All Rights Reserved
*
*************************************************************************** 
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR 
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: audio_microphone.c$
* $Version : 
* $Date    : 
*
* Comments:
*
*   This file is an example of device drivers for the Audio host class. This example
*   demonstrates the audio transfer capability of audio host class with audio devices.
*
*END************************************************************************/

#include "audio_microphone.h"
#include "usb_host_audio.h"

#include "bsp.h"
#include "mqx_host.h"
#include "fio.h"
#include <usb_host_hub_sm.h>
#include "usbprv_host.h"
#include "audio_timer.h"
#include "mfs.h"
#include "sh_mfs.h"
#include "sh_audio.h"
#include "shell.h"
#include "sdcard.h"
#include <lwevent.h>
#include <spi.h>
#include <part_mgr.h>

#if ! SHELLCFG_USES_MFS
#error This application requires SHELLCFG_USES_MFS \
    defined non-zero in user_config.h. Please recompile libraries with this option.
#endif

#if defined BSP_SDCARD_ESDHC_CHANNEL
#if ! BSPCFG_ENABLE_ESDHC
#error This application requires BSPCFG_ENABLE_ESDHC \
    defined non-zero in user_config.h. Please recompile libraries with this option.
#endif

#elif defined BSP_SDCARD_SDHC_CHANNEL

#if ! BSPCFG_ENABLE_SDHC
#error This application requires BSPCFG_ENABLE_SDHC \
    defined non-zero in user_config.h. Please recompile libraries with this option.
#endif

#else
#if ! BSPCFG_ENABLE_SPI0
#error This application requires BSPCFG_ENABLE_SPI0 \
    defined non-zero in user_config.h. Please recompile libraries with this option.
#endif

#endif

#if defined (BSP_SDCARD_SPI_CHANNEL)
#define SDCARD_COM_CHANNEL BSP_SDCARD_SPI_CHANNEL
#elif defined (BSP_SDCARD_ESDHC_CHANNEL)
#define SDCARD_COM_CHANNEL BSP_SDCARD_ESDHC_CHANNEL
#elif defined (BSP_SDCARD_SDHC_CHANNEL)
#define SDCARD_COM_CHANNEL BSP_SDCARD_SDHC_CHANNEL
#else
#error "SDCARD low level communication device not defined!"
#endif
/***************************************
**
** Macros
****************************************/
#define USB_EVENT_CTRL 0x01
#define USB_EVENT_DATA 0x02

/***************************************
**
** Global functions
****************************************/

/***************************************
**
** Local functions
****************************************/
static USB_STATUS check_device_type (
                                        USB_AUDIO_CTRL_DESC_IT_PTR,
                                        USB_AUDIO_CTRL_DESC_OT_PTR,
                                        char_ptr _PTR_,
                                        char_ptr
                                    );
static void Audio_Task(uint_32);
static void Sdcard_task(uint_32 temp);
static void Shell_task(uint_32 temp);

/***************************************
**
** Global variables
****************************************/
/* microphone or speaker */
char                                    device_direction = UNDEFINE_DEVICE;
/* buffer for receiving an audio data packet */
char                                    wav_recv_buff[MAX_ISO_PACKET_SIZE];
/* buffer for accumulating audio data packets */
char                                    wav_recv_buff_tmp[REC_BLOCK_SIZE];
/* buffer for writing audio data to SD card */
char                                    wav_write_buff_tmp[REC_BLOCK_SIZE];
/* audio packet size */
uint_32                                 packet_size;
/* audio control device */
volatile AUDIO_CONTROL_DEVICE_STRUCT    audio_control = { 0 };
/* audio stream device */
volatile AUDIO_CONTROL_DEVICE_STRUCT    audio_stream  = { 0 };
/* check device type error */
static uint_8                           error_state   =  0;
/* audio stream descriptor */
USB_AUDIO_STREAM_DESC_FORMAT_TYPE_PTR   frm_type_desc = NULL;
/* record flag */
boolean                                 record_flag;
/* block record ready event */
extern LWEVENT_STRUCT_PTR block_rec;
/* bytes remain for record */
extern uint_32                          remainder;
/* final packet flag*/
extern boolean                          final_packet;
/* end point descriptor */
ENDPOINT_DESCRIPTOR_PTR                 endp;

/* MQX template list */
TASK_TEMPLATE_STRUCT  MQX_template_list[] =
{
    {10,    Audio_Task,         1500L,    10L, "Audio Task", MQX_AUTO_START_TASK},
    {11,    Sdcard_task,        1000L,    11L, "Sdcard Task",MQX_AUTO_START_TASK},
    {12,    Shell_task,         2000L,    12L, "Shell Task", MQX_AUTO_START_TASK},
    {0L,    0L,                 0L,       0L,  0L,           0L }
};

/* Shell list */
const SHELL_COMMAND_STRUCT Shell_commands[] = {
    { "cd",        Shell_cd },
    { "del",       Shell_del },
    { "dir",       Shell_dir },
    { "help",      Shell_help },
    { "read",      Shell_read },
    { "mkdir",     Shell_mkdir },
    { "ren",       Shell_rename },
    { "rmdir",     Shell_rmdir },
    { "record",    Shell_record },
    { "mute",      Shell_mute },
    { "?",         Shell_command_list },
    { NULL,        NULL }
};

/***************************************
**
** Local variables
****************************************/
/* device type string*/
static char_ptr                         device_string;
/* audio command */
static AUDIO_COMMAND                    audio_com;

/* Transfer Types */
static const char_ptr TransferType[TRANSFER_TYPE_NUM] = 
{
    "Control",
    "Isochronous",
    "Bulk",
    "Interrupt"
};
/* Sync Types */
static const char_ptr SyncType[SYNC_TYPE_NUM] = 
{
    "No synchronization",
    "Asynchronous",
    "Adaptive",
    "Synchrounous"
};
/* Data Types */
static const char_ptr DataType[DATA_TYPE_NUM] = 
{
    "Data endpoint",
    "Feedback endpoint",
    "Implicit feedback",
    "Reserved"
};

/* Table of driver capabilities this application wants to use */
static  USB_HOST_DRIVER_INFO DriverInfoTable[] =
{
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer      */
      USB_CLASS_AUDIO,              /* Class code                       */
      USB_SUBCLASS_AUD_CONTROL,     /* Sub-Class code                   */
      0xFF,                         /* Protocol                         */
      0,                            /* Reserved                         */
      usb_host_audio_control_event  /* Application call back function   */
   },
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer      */
      USB_CLASS_AUDIO,              /* Class code                       */
      USB_SUBCLASS_AUD_STREAMING,   /* Sub-Class code                   */
      0xFF,                         /* Protocol                         */
      0,                            /* Reserved                         */
      usb_host_audio_stream_event   /* Application call back function   */
   },
   /* USB 1.1 hub */
   {

      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer      */
      USB_CLASS_HUB,                /* Class code                       */
      USB_SUBCLASS_HUB_NONE,        /* Sub-Class code                   */
      USB_PROTOCOL_HUB_LS,          /* Protocol                         */
      0,                            /* Reserved                         */
      usb_host_hub_device_event     /* Application call back function   */
   },
   {
      {0x00,0x00},                  /* All-zero entry terminates        */
      {0x00,0x00},                  /* driver info list.                */
      0,
      0,
      0,
      0,
      NULL
   }
};
/* Input Terminal types */
static char_ptr it_type_string[NUMBER_OF_IT_TYPE] = 
{
    "Input Undefined",
    "Microphone",
    "Desktop Microphone",
    "Personal Microphone",
    "Omni directional Microphone",
    "Microphone array",
    "Processing Microphone array",
};

/* Output Terminal types */
static char_ptr ot_type_string[NUMBER_OF_OT_TYPE] = 
{
    "Output Undefined",
    "Speaker",
    "Headphones",
    "Head Mounted Display Audio",
    "Desktop Speaker",
    "Room Speaker",
    "Communication Speaker",
    "Low Frequency Effects Speaker",
};

/* Input Terminal types */
static const char_ptr microphone_type_string[NUMBER_OF_IT_TYPE] = 
{
    "Input Undefined",
    "Microphone",
    "Desktop Microphone",
    "Personal Microphone",
    "Omni directional Microphone",
    "Microphone array",
    "Processing Microphone array",
};

#ifdef BSP_SDCARD_GPIO_CS
/*FUNCTION*---------------------------------------------------------------
*
* Function Name : set_CS
* Comments  : This function sets chip select signal to enable/disable memory
*
*END*----------------------------------------------------------------------*/

static void set_CS (uint_32 cs_mask, uint_32 logic_level, pointer user_data)
{
    LWGPIO_STRUCT_PTR gpiofd = user_data;

    if (cs_mask & BSP_SDCARD_SPI_CS)
    {
        lwgpio_set_value(gpiofd, logic_level ? LWGPIO_VALUE_HIGH : LWGPIO_VALUE_LOW);
    }
}
#endif
/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : check_device_type
* Returned Value : None
* Comments       :
*    This function check whether the attached device is out-device or in-device. 
*
*END*--------------------------------------------------------------------*/
static USB_STATUS check_device_type
    (
    /* [IN] Input terminal descriptor */
    USB_AUDIO_CTRL_DESC_IT_PTR      it_desc,

    /* [IN] Output terminal descriptor */
    USB_AUDIO_CTRL_DESC_OT_PTR      ot_desc,

    /* [OUT] Terminal type name */
    char_ptr _PTR_                  device_type,

    /* [OUT] device direction */
    char_ptr                        direction
    )
{
    uchar it_type_high, it_type_low, ot_type_high, ot_type_low;

    it_type_high = it_desc->wTerminalType[0];
    it_type_low  = it_desc->wTerminalType[1];
    ot_type_high = ot_desc->wTerminalType[0];
    ot_type_low  = ot_desc->wTerminalType[1];

    /* Input terminal associates with audio streaming */
    if (USB_TERMINAL_TYPE == it_type_low)
    {
        *direction = IN_DEVICE;
    }
    /* Input terminal type */
    else if (INPUT_TERMINAL_TYPE == it_type_low)
    {
        /* get type device name */
        *device_type = it_type_string[it_type_high];
    }
    else
    {
        return USBERR_ERROR;
    }

    /* Output terminal associates with audio streaming */
    if (USB_TERMINAL_TYPE == ot_type_low)
    {
        if (IN_DEVICE == (*direction))
        {
            *direction = UNDEFINE_DEVICE;
            return USBERR_ERROR;
        }
        else
        {
            *direction = OUT_DEVICE;
        }
    }
    /* Output terminal type */
    else if (OUTPUT_TERMINAL_TYPE == ot_type_low)
    {
        /* get type device name */
        *device_type = ot_type_string[it_type_high];
    }
    else
    {
        return USBERR_ERROR;
    }
    return USB_OK;
}

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : USB_Audio_Get_Packet_Size
* Returned Value : None
* Comments       :
*     This function gets the frequency supported by the attached device.
*
*END*--------------------------------------------------------------------*/
static uint_32 USB_Audio_Get_Packet_Size
(
/* [IN] Point to format type descriptor */
USB_AUDIO_STREAM_DESC_FORMAT_TYPE_PTR format_type_desc
)
{
    uint_32 packet_size_tmp;
    /* calculate packet size to send to the device each mS.*/
    /* packet_size = (sample frequency (Hz) /1000) * 
     * (bit resolution/8) * number of channels */
    packet_size_tmp = (((format_type_desc->tSamFreq[2] << 16)|
    (format_type_desc->tSamFreq[1] << 8) |
    (format_type_desc->tSamFreq[0] << 0) )
    *(format_type_desc->bBitResolution/8)
    *(format_type_desc->bNrChannels)
    /1000);
    return (packet_size_tmp);
}



/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : Audio_Task
* Returned Value : none
* Comments       :
*     Execution starts here
*
*END*--------------------------------------------------------------------*/
static void Audio_Task ( uint_32 param )
{ /* Body */
   USB_STATUS           status = USB_OK;
   _usb_host_handle     host_handle;
   
   /* _usb_otg_init needs to be done with interrupts disabled */
   _int_disable();

   _int_install_unexpected_isr();
   if (MQX_OK != _usb_host_driver_install(USBCFG_DEFAULT_HOST_CONTROLLER)) {
      printf("\n Driver installation failed");
      _task_block();
   }

   /*
   ** It means that we are going to act like host, so we initialize the
   ** host stack. This call will allow USB system to allocate memory for
   ** data structures, it uses later (e.g pipes etc.).
   */
   status = _usb_host_init (USBCFG_DEFAULT_HOST_CONTROLLER, &host_handle);

   if (USB_OK != status) 
   {
      printf("\nUSB Host Initialization failed. STATUS: %x", status);
      _task_block();
   }

   /*
   ** since we are going to act as the host driver, register the driver
   ** information for wanted class/subclass/protocols
   */
   status = _usb_host_driver_info_register (
                                    host_handle,
                                    DriverInfoTable
                                    );
   if (USB_OK != status) {
      printf("\nDriver Registration failed. STATUS: %x", status);
      _task_block();
   }
   _int_enable();
   
   printf("USB Host Audio Demo\n"
           "Waiting for USB Audio Microphone Device to be attached...\n");
   
   audio_timer_init();     
   _task_block();
} /* Endbody */

/*TASK*-----------------------------------------------------------------
*
* Function Name  : Sdcard_task
* Returned Value : void
* Comments       :
*
*END------------------------------------------------------------------*/
static void Sdcard_task(uint_32 temp)
{
    boolean      inserted = TRUE, readonly = FALSE, last = FALSE;
    _mqx_int     error_code;
    _mqx_uint    param;
    MQX_FILE_PTR com_handle, sdcard_handle, filesystem_handle, partman_handle;
    char         filesystem_name[] = "a:";
    char         partman_name[] = "pm:";
#if defined BSP_SDCARD_GPIO_DETECT
    LWGPIO_STRUCT      sd_detect;
#endif
#if defined BSP_SDCARD_GPIO_PROTECT
    LWGPIO_STRUCT      sd_protect;
#endif
#ifdef BSP_SDCARD_GPIO_CS

    LWGPIO_STRUCT          sd_cs;
    SPI_CS_CALLBACK_STRUCT callback;

#endif

    /* Open low level communication device */
    com_handle = fopen (SDCARD_COM_CHANNEL, NULL);

    if (NULL == com_handle)
    {
        printf("Error installing communication handle.\n");
        _task_block();
    }

#ifdef BSP_SDCARD_GPIO_CS

    /* Open GPIO file for SPI CS signal emulation */
    error_code = lwgpio_init(
                                &sd_cs,
                                BSP_SDCARD_GPIO_CS,
                                LWGPIO_DIR_OUTPUT,
                                LWGPIO_VALUE_NOCHANGE
                             );
    if (!error_code)
    {
        printf("Initializing GPIO with associated pins failed.\n");
        _task_block();
    }
    lwgpio_set_functionality(&sd_cs,BSP_SDCARD_CS_MUX_GPIO);
    lwgpio_set_attribute(&sd_cs, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
    /* Set CS call back */
    callback.MASK = BSP_SDCARD_SPI_CS;
    callback.CALLBACK = set_CS;
    callback.USERDATA = &sd_cs;
    if (SPI_OK != ioctl (com_handle, IO_IOCTL_SPI_SET_CS_CALLBACK, &callback))
    {
        printf ("Setting CS call back failed.\n");
        _task_block();
    }

#endif

#if defined BSP_SDCARD_GPIO_DETECT
    /* Init GPIO pins for other SD card signals */
    error_code = lwgpio_init(&sd_detect, BSP_SDCARD_GPIO_DETECT, \
                             LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE);
    if (!error_code)
    {
        printf("Initializing GPIO with sdcard detect pin failed.\n");
        _task_block();
    }
    /*Set detect and protect pins as GPIO Function */
    lwgpio_set_functionality(&sd_detect,BSP_SDCARD_DETECT_MUX_GPIO);
    lwgpio_set_attribute(&sd_detect, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
#endif

#if defined BSP_SDCARD_GPIO_PROTECT
    /* Init GPIO pins for other SD card signals */
    error_code = lwgpio_init(&sd_protect, BSP_SDCARD_GPIO_PROTECT, \
                             LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE);
    if (!error_code)
    {
        printf("Initializing GPIO with sdcard protect pin failed.\n");
        _task_block();
    }
    /*Set detect and protect pins as GPIO Function */
    lwgpio_set_functionality(&sd_protect,BSP_SDCARD_PROTECT_MUX_GPIO);
    lwgpio_set_attribute(&sd_protect, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
#endif

    /* Install SD card device */
    error_code = _io_sdcard_install("sdcard:", (pointer)&_bsp_sdcard0_init, com_handle);
    if ( MQX_OK != error_code)
    {
        printf("Error installing SD card device (0x%x)\n", error_code);
        _task_block();
    }

    for (;;)
    {

#if defined BSP_SDCARD_GPIO_DETECT
        #ifdef BSP_MPC8308RDB
        /* Set function as GPIO to detect sdcard */
        lwgpio_set_functionality(&sd_detect,BSP_SDCARD_DETECT_MUX_GPIO);
        lwgpio_set_attribute(&sd_detect, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
        #endif
        inserted = !lwgpio_get_value(&sd_detect);
#endif

#if defined BSP_SDCARD_GPIO_PROTECT
        /* Get value of protect pin */
        readonly = lwgpio_get_value(&sd_protect);
#endif
#ifdef BSP_MPC8308RDB
        /* Set function as SD_CD which indicate that card is present 
         * in Present State Register */
        lwgpio_set_functionality(&sd_detect,BSP_SDCARD_DETECT_MUX_SD_CD);
        lwgpio_set_attribute(&sd_detect, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
#endif
        if (last != inserted)
        {
            if (inserted)
            {
                _time_delay (200);
                /* Open the device which MFS will be installed on */
                sdcard_handle = fopen("sdcard:", 0);
                if ( sdcard_handle == NULL )
                {
                    printf("Unable to open SD card device.\n");
                    _task_block();
                }

                /* Set read only flag as needed */
                param = 0;
                if (readonly)
                {
                    param = IO_O_RDONLY;
                }
                if (IO_OK != ioctl(sdcard_handle, IO_IOCTL_SET_FLAGS, (char_ptr) &param))
                {
                    printf("Setting device read only failed.\n");
                    _task_block();
                }

                /* Install partition manager over SD card driver */
                error_code = _io_part_mgr_install(sdcard_handle, partman_name, 0);
                if (MFS_NO_ERROR != error_code)
                {
                    printf("Error installing partition manager: %s\n", \
                            MFS_Error_text((uint_32)error_code));
                    _task_block();
                }

                /* Open partition manager */
                partman_handle = fopen(partman_name, NULL);
                if (NULL == partman_handle)
                {
                    error_code = ferror(partman_handle);
                    printf("Error opening partition manager: %s\n", \
                            MFS_Error_text((uint_32)error_code));
                    _task_block();
                }

                /* Validate partition 1 */
                param = 1;
                error_code = _io_ioctl(partman_handle, IO_IOCTL_VAL_PART, &param);
                if (MQX_OK == error_code)
                {

                    /* Install MFS over partition 1 */
                    error_code = _io_mfs_install(partman_handle, filesystem_name, param);
                    if (MFS_NO_ERROR != error_code)
                    {
                        printf("Error initializing MFS over partition: %s\n", \
                                MFS_Error_text((uint_32)error_code));
                        _task_block();
                    }

                } else {

                    /* Install MFS over SD card driver */
                    error_code = _io_mfs_install(
                                                    sdcard_handle,
                                                    filesystem_name,
                                                    (_file_size)0
                                                 );
                    if (MFS_NO_ERROR != error_code)
                    {
                        printf("Error initializing MFS: %s\n",\
                                MFS_Error_text((uint_32)error_code));
                        _task_block();
                    }

                }

                /* Open file system */
                filesystem_handle = fopen(filesystem_name, NULL);
                error_code = ferror (filesystem_handle);
                if ((MFS_NO_ERROR != error_code) && (MFS_NOT_A_DOS_DISK != error_code))
                {
                    printf("Error opening filesystem: %s\n",\
                            MFS_Error_text((uint_32)error_code));
                    _task_block();
                }
                if ( MFS_NOT_A_DOS_DISK ==  error_code)
                {
                    printf("NOT A DOS DISK! You must format to continue.\n");
                }

                printf ("SD card installed to %s\n", filesystem_name);
                if (readonly)
                {
                    printf ("SD card is locked (read only).\n");
                }
            }
            else
            {
                /* Close the filesystem */
                if (MQX_OK != fclose (filesystem_handle))
                {
                    printf("Error closing filesystem.\n");
                    _task_block();
                }
                filesystem_handle = NULL;

                /* Uninstall MFS  */
                error_code = _io_dev_uninstall(filesystem_name);
                if (MFS_NO_ERROR != error_code)
                {
                    printf("Error uninstalling filesystem.\n");
                    _task_block();
                }

                /* Close partition manager */
                if (MQX_OK != fclose (partman_handle))
                {
                    printf("Unable to close partition manager.\n");
                    _task_block();
                }
                partman_handle = NULL;

                /* Uninstall partition manager  */
                error_code = _io_dev_uninstall(partman_name);
                if (MFS_NO_ERROR != error_code)
                {
                    printf("Error uninstalling partition manager.\n");
                    _task_block();
                }

                /* Close the SD card device */
                if (MQX_OK != fclose (sdcard_handle))
                {
                    printf("Unable to close SD card device.\n");
                    _task_block();
                }
                sdcard_handle = NULL;

                printf ("SD card uninstalled.\n");
            }
        }
        last = inserted;
        _time_delay (200);
    }
}

/*TASK*-----------------------------------------------------------------
*
* Function Name  : Shell_task
* Returned Value : void
* Comments       :
*
*END------------------------------------------------------------------*/
static void Shell_task(uint_32 temp)
{
    /* Run the shell on the serial port */
    Shell(Shell_commands, NULL);
    _task_block();
}

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_audio_mute_ctrl_callback
* Returned Value : None
* Comments       :
*     Called when a mute request is sent successfully.
*
*END*--------------------------------------------------------------------*/
void usb_host_audio_mute_ctrl_callback
   (
      /* [IN] pointer to pipe */
      _usb_pipe_handle  pipe_handle,

      /* [IN] user-defined parameter */
      pointer           user_parm,

      /* [IN] buffer address */
      uchar_ptr         buffer,

      /* [IN] length of data transferred */
      uint_32           buflen,

      /* [IN] status, hopefully USB_OK or USB_DONE */
      uint_32           status
   )
{ 
   /* Body */
   if(record_flag)
   _audio_timer_unmask_int(AUDIO_TIMER); 
   printf("Set Mute successfully\n");
   fflush(stdout);
   
} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_audio_tr_callback
* Returned Value : None
* Comments       :
*     Called when a ISO packet is sent/received successfully.
*
*END*--------------------------------------------------------------------*/
void usb_host_audio_tr_callback(
    /* [IN] pointer to pipe */
    _usb_pipe_handle pipe_handle,
    
    /* [IN] user-defined parameter */
    pointer user_parm,
    
    /* [IN] buffer address */
    uchar_ptr buffer,
    
    /* [IN] length of data transferred */
    uint_32 buflen,
    
    /* [IN] status, hopefully USB_OK or USB_DONE */
    uint_32 status)
{
    uint_32 i;
   static uint_16 packnum = 0;
   if(OUT_DEVICE == device_direction)
   {
       uint_8 k;
       /* copy each packet data to the received buffer*/
       for(k=0; k < packet_size; k++)
       {
          wav_recv_buff_tmp[packnum++] = wav_recv_buff[k];
       }
       if ((REC_BLOCK_SIZE == packnum)||\
          ((TRUE == final_packet)&& (packnum == remainder)))
       {
           /* copy the received buffer to write buffer */
           for(i = 0; i < packnum; i++)
           wav_write_buff_tmp[i] = wav_recv_buff_tmp[i];
           packnum =0;
           if (MQX_OK != _lwevent_set(block_rec, EV_BLOCK_W_READY))
           {
              printf("  Error: Failed to set BLOCK_W_READY event.\n");
           }
       }
   }
}

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_audio_control_event
* Returned Value : None
* Comments       :
*     Called when control interface has been attached, detached, etc.
*END*--------------------------------------------------------------------*/
void usb_host_audio_control_event
   (
      /* [IN] pointer to device instance */
      _usb_device_instance_handle      dev_handle,

      /* [IN] pointer to interface descriptor */
      _usb_interface_descriptor_handle intf_handle,

      /* [IN] code number for event causing callback */
      uint_32                          event_code
   )
{
   INTERFACE_DESCRIPTOR_PTR   intf_ptr =
      (INTERFACE_DESCRIPTOR_PTR)intf_handle;
    
   fflush(stdout);
   switch (event_code) {
      case USB_CONFIG_EVENT:
      /* Drop through into attach, same processing */
      case USB_ATTACH_EVENT: 
      {
         USB_AUDIO_CTRL_DESC_HEADER_PTR   header_desc = NULL;
         USB_AUDIO_CTRL_DESC_IT_PTR       it_desc  = NULL;
         USB_AUDIO_CTRL_DESC_OT_PTR       ot_desc  = NULL;
         USB_AUDIO_CTRL_DESC_FU_PTR       fu_desc  = NULL;

         if((USB_DEVICE_IDLE == audio_stream.DEV_STATE) || \
            (USB_DEVICE_DETACHED == audio_stream.DEV_STATE)) 
         {
            audio_control.DEV_HANDLE  = dev_handle;
            audio_control.INTF_HANDLE = intf_handle;
            audio_control.DEV_STATE   = USB_DEVICE_ATTACHED;
         }
         else
         {
            printf("Audio device already attached\n");
            fflush(stdout);
         }

         /* finds all the descriptors in the configuration */
         if (USB_OK != usb_class_audio_control_get_descriptors(  
                                                                 dev_handle,
                                                                 intf_handle,
                                                                 &header_desc,
                                                                 &it_desc,
                                                                 &ot_desc,
                                                                 &fu_desc)
                                                              )
         {
            break;
         };

           /* initialize new interface members and select this interface */
         if (USB_OK != _usb_hostdev_select_interface(dev_handle,
            intf_handle, (pointer)&audio_control.CLASS_INTF))
         {
            break;
         }

         /* set all info got from descriptors to the class interface struct */
         usb_class_audio_control_set_descriptors((pointer)&audio_control.CLASS_INTF,
            header_desc, it_desc, ot_desc, fu_desc);

         if(USB_OK != check_device_type(
                                         it_desc,
                                         ot_desc,
                                         &device_string,
                                         &device_direction)
                                        )
         {
            error_state=1;
            break;
         }

         printf("----- Audio control interface: attach event -----\n");
         fflush(stdout);
         printf("State = attached");
         printf("  Class = %d", intf_ptr->bInterfaceClass);
         printf("  SubClass = %d", intf_ptr->bInterfaceSubClass);
         printf("  Protocol = %d\n", intf_ptr->bInterfaceProtocol);
         break;
      }
      case USB_INTF_EVENT: 
      {
         USB_STATUS status;

         status = usb_class_audio_init_ipipe(
                                               (CLASS_CALL_STRUCT_PTR)\
                                               &audio_control.CLASS_INTF,
                                               NULL,NULL
                                             );

         if ((USB_OK != status) && (USBERR_OPEN_PIPE_FAILED != status))
             break;

         printf("----- Audio control interface: interface event -----\n");
         audio_control.DEV_STATE = USB_DEVICE_INTERFACED;
         break;
      }

      case USB_DETACH_EVENT:
      {
         AUDIO_CONTROL_INTERFACE_STRUCT_PTR if_ptr;
   
         if_ptr = (AUDIO_CONTROL_INTERFACE_STRUCT_PTR) \
                   audio_control.CLASS_INTF.class_intf_handle;
         
         _lwevent_destroy(&if_ptr->control_event);

         printf("----- Audio control interface: detach event -----\n");
         fflush(stdout);
         printf("State = detached");
         printf("  Class = %d", intf_ptr->bInterfaceClass);
         printf("  SubClass = %d", intf_ptr->bInterfaceSubClass);
         printf("  Protocol = %d\n", intf_ptr->bInterfaceProtocol);
         fflush(stdout);
         audio_control.DEV_HANDLE = NULL;
         audio_control.INTF_HANDLE = NULL;
         audio_control.DEV_STATE = USB_DEVICE_DETACHED;
         error_state=0;
         device_direction = UNDEFINE_DEVICE;
         break;
      }
      default:
         printf("Audio Device: unknown control event\n");
         fflush(stdout);
         break;
   } /* EndSwitch */
   fflush(stdout);
} /* Endbody */
   
/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_audio_stream_event
* Returned Value : None
* Comments       :
*     Called when stream interface has been attached, detached, etc.
*END*--------------------------------------------------------------------*/
void usb_host_audio_stream_event
   (
      /* [IN] pointer to device instance */
      _usb_device_instance_handle      dev_handle,

      /* [IN] pointer to interface descriptor */
      _usb_interface_descriptor_handle intf_handle,

      /* [IN] code number for event causing callback */
      uint_32                          event_code
   )
{ /* Body */
   INTERFACE_DESCRIPTOR_PTR   intf_ptr =
      (INTERFACE_DESCRIPTOR_PTR)intf_handle;
   
   /* Check audio stream interface alternating 0 */
   if (0 == intf_ptr->bNumEndpoints)
        return;
   
   switch (event_code) {
      case USB_CONFIG_EVENT:
      /* Drop through into attach, same processing */
      case USB_ATTACH_EVENT: 
      {
         USB_AUDIO_STREAM_DESC_SPEPIFIC_AS_IF_PTR     as_itf_desc = NULL;
         USB_AUDIO_STREAM_DESC_SPECIFIC_ISO_ENDP_PTR  iso_endp_spec_desc = NULL;

        if((USB_DEVICE_IDLE == audio_stream.DEV_STATE) ||\
           (USB_DEVICE_DETACHED == audio_stream.DEV_STATE)) 
         {
            audio_stream.DEV_HANDLE  = dev_handle;
            audio_stream.INTF_HANDLE = intf_handle;
            audio_stream.DEV_STATE      = USB_DEVICE_ATTACHED;
         }
         else
         {
            printf("Audio device already attached\n");
            fflush(stdout);
         }

         /* finds all the descriptors in the configuration */
         if (USB_OK != usb_class_audio_stream_get_descriptors(
                                                                  dev_handle,
                                                                  intf_handle,
                                                                  &as_itf_desc,
                                                                  &frm_type_desc,
                                                                  &iso_endp_spec_desc)
                                                              )
         {
            break;
         };

         /* initialize new interface members and select this interface */
         if (USB_OK != _usb_hostdev_select_interface(dev_handle,
            intf_handle, (pointer)&audio_stream.CLASS_INTF))
         {
              break;
         }

         packet_size = USB_Audio_Get_Packet_Size(frm_type_desc);

        _audio_timer_init_freq(AUDIO_TIMER,
             AUDIO_MICROPHONE_FREQUENCY,AUDIO_TIMER_CLOCK, FALSE);


         /* set all info got from descriptors to the class interface struct */
         usb_class_audio_stream_set_descriptors(
                                                (pointer)&audio_stream.CLASS_INTF,
                                                as_itf_desc,
                                                frm_type_desc,
                                                iso_endp_spec_desc
                                               );
         printf("----- Audio stream interface: attach event -----\n");
         fflush(stdout);
         printf("State = attached");
         printf("  Class = %d", intf_ptr->bInterfaceClass);
         printf("  SubClass = %d", intf_ptr->bInterfaceSubClass);
         printf("  Protocol = %d\n", intf_ptr->bInterfaceProtocol);
         fflush(stdout);

         break;
      }   
      case USB_INTF_EVENT: 
      {
         if(0==error_state)
         {
            audio_stream.DEV_STATE = USB_DEVICE_INTERFACED;
            if (USB_OK != _usb_hostdev_get_descriptor( 
                                                       dev_handle,
                                                       intf_handle,
                                                       /* Functional descriptor 
                                                        * for interface */
                                                       USB_DESC_TYPE_EP,  
                                                       /* Index of descriptor */
                                                       1,      
                                                       /* Index of interface alternate */
                                                       intf_ptr->bAlternateSetting, 
                                                       (pointer _PTR_)&endp)
                                                      )
            printf("Get end point descriptor error!"); 
            printf("----- Audio stream interface: interface event-----\n");
            printf("Audio device information:\n");
            printf("   - Device type    : %s\n", device_string);
            printf("   - Frequency      : %d Hz\n", (frm_type_desc->tSamFreq[2] << BYTE2_SHIFT) |
                                                    (frm_type_desc->tSamFreq[1] << BYTE1_SHIFT)  |
                                                    (frm_type_desc->tSamFreq[0] << BYTE0_SHIFT));
            printf("   - Bit resolution : %d bits\n", frm_type_desc->bBitResolution);
            printf("   - Number of channels : %d channels\n",\
                    frm_type_desc->bNrChannels);
            printf("   - Transfer type : %s\n",\
                    TransferType[(endp->bmAttributes>>TRANSFER_TYPE_SHIFT)&EP_TYPE_MASK]);
            printf("   - Sync type : %s\n", \
                    SyncType[(endp->bmAttributes>>SYNC_TYPE_SHIFT)&EP_TYPE_MASK]);
            printf("   - Usage type : %s\n",\
                    DataType[(endp->bmAttributes>>DATA_TYPE_SHIFT)&EP_TYPE_MASK]);
            if (ISOCH_NOSYNC!=((endp->bmAttributes>>SYNC_TYPE_SHIFT)&EP_TYPE_MASK))
            {
                printf("This sync type of the device is not supported!\n");
            }
            else if (IN_DEVICE == device_direction)
            {
                printf("The device is unsupported!\n");
            } 
            else
            {
                printf("This audio device supports record audio files with these"
                        " properties:\n");
                printf("   - Sample rate    : %d Hz\n",
                        (frm_type_desc->tSamFreq[2] << BYTE2_SHIFT) |
                        (frm_type_desc->tSamFreq[1] << BYTE1_SHIFT)  |
                        (frm_type_desc->tSamFreq[0] << BYTE0_SHIFT));
                printf("   - Sample size    : %d bits\n", 
                        frm_type_desc->bBitResolution);
                printf("   - Number of channels : %d channels\n",\
                        frm_type_desc->bNrChannels);
                printf("Type \"record a:\\filename.wav 5\" to record the file"\
                        " with 5 (s) length \n");
            } 
            fflush(stdout);
         }
         else
         {
            printf("The device is unsupported!\n"); 
            fflush(stdout);
         }
         break;
      }
      case USB_DETACH_EVENT: 
      {
         audio_stream.DEV_HANDLE = NULL;
         audio_stream.INTF_HANDLE = NULL;
         audio_stream.DEV_STATE = USB_DEVICE_DETACHED;
         printf("----- Audio stream interface: detach event-----\n");
         fflush(stdout);
         record_flag = FALSE;
         _audio_timer_mask_int(AUDIO_TIMER);
         break;
      }
      default:
         printf("Audio device: unknown data event\n");
         fflush(stdout);
         break;
   } /* EndSwitch */
} /* Endbody */



/* EOF */
