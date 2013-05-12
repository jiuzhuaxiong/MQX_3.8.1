/**HEADER********************************************************************
* 
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved
*
* Copyright (c) 2004-2008 Embedded Access Inc.;
* All Rights Reserved
*
* Copyright (c) 1989-2008 ARC International;
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
* $FileName: s29ws128.c$
* $Version : 3.8.4.1$
* $Date    : Feb-13-2012$
*
* Comments:
*
*   The file contains the block definitions for the
*   flash device.
*
*END************************************************************************/

#include "mqx.h" 
#include "bsp.h"
#include "flashx.h"

#define S28WS128_SECTOR_SIZE_32K    (0x8000)
#define S28WS128_NUM_SECTORS_1      (4)
#define S28WS128_SECTOR_SIZE_128K   (0x20000)
#define S28WS128_NUM_SECTORS_2      (126)
#define S28WS128_NUM_SECTORS_3      (4)
#define S28WS128_BANK0_START        (0)

#define S28WS128_BANK1_START        (S28WS128_BANK0_START + S28WS128_NUM_SECTORS_1 * S28WS128_SECTOR_SIZE_32K)
#define S28WS128_BANK2_START        (S28WS128_BANK1_START + S28WS128_NUM_SECTORS_2 * S28WS128_SECTOR_SIZE_128K)

const FLASHX_BLOCK_INFO_STRUCT _S28WS128_block_map_16bit[] =  {
    { S28WS128_NUM_SECTORS_1, S28WS128_BANK0_START, S28WS128_SECTOR_SIZE_32K},
    { S28WS128_NUM_SECTORS_2, S28WS128_BANK1_START, S28WS128_SECTOR_SIZE_128K},
    { S28WS128_NUM_SECTORS_3, S28WS128_BANK2_START, S28WS128_SECTOR_SIZE_32K},
    { 0, 0, 0 }
};

/* EOF*/
