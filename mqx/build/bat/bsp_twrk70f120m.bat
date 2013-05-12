@echo off

set bat_file_dir=%~dp0

rem 1st parameter is target path (in the <mqx>/lib/<board>/<dir>)
rem or it may be also a name of the library file created by the linker

rem 2nd parameter is compiler name (cw, iar, ewarm, ewcf, keil, uv4), default is cw
set compiler=%~2
if /I "%compiler%" == "" set compiler=cw 

rem 3rd parameter is target name (debug, release) - so fas for Keil compiler only
set target=%~3

rem 4th parameter is board name - so far for Keil compiler only


rem Make sure we are in target directory  (create if needed)
call "%bat_file_dir%\verify_dir.bat" "%~f1"
if %ERRORLEVEL% NEQ 0 exit 1

attrib -R * >NUL 2>&1

rem  Configuration header files

for %%F in (..\..\..\config\common\*.h)      do copy /Y /B ..\..\..\mqx\source\include\dontchg.h + %%F ..\%%~nF%%~xF
for %%F in (..\..\..\config\twrk70f120m\*.h)  do copy /Y /B ..\..\..\mqx\source\include\dontchg.h + %%F ..\%%~nF%%~xF

rem  Board-specific BSP files

copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\bsp.h .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\bsp_rev.h .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\twrk70f120m.h .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\init_lpm.h .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\bsp_cm.h .
if NOT EXIST TSS mkdir TSS 
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\TSS_SystemSetup.h .\TSS

rem  Board-specific IO driver files

copy /Y ..\..\..\mqx\source\io\adc\adc.h .
copy /Y ..\..\..\mqx\source\io\adc\kadc\adc_kadc.h .
copy /Y ..\..\..\mqx\source\io\adc\kadc\adc_mk70.h .
copy /Y ..\..\..\mqx\source\io\io_dun\io_dun.h .
copy /Y ..\..\..\mqx\source\io\io_mem\io_mem.h .
copy /Y ..\..\..\mqx\source\io\io_mem\iomemprv.h .
copy /Y ..\..\..\mqx\source\io\pcb\io_pcb.h .
copy /Y ..\..\..\mqx\source\io\pcb\mqxa\pcb_mqxa.h .
copy /Y ..\..\..\mqx\source\io\io_null\io_null.h .
copy /Y ..\..\..\mqx\source\io\pipe\io_pipe.h .
copy /Y ..\..\..\mqx\source\io\serial\serial.h  .
copy /Y ..\..\..\mqx\source\io\serial\serl_kuart.h .
copy /Y ..\..\..\mqx\source\io\lwgpio\lwgpio.h .
copy /Y ..\..\..\mqx\source\io\lwgpio\lwgpio_kgpio.h .
copy /Y ..\..\..\mqx\source\io\gpio\io_gpio.h .
copy /Y ..\..\..\mqx\source\io\gpio\kgpio\io_gpio_kgpio.h .
copy /Y ..\..\..\mqx\source\io\spi\spi.h .
copy /Y ..\..\..\mqx\source\io\spi\spi_dspi.h .
copy /Y ..\..\..\mqx\source\io\can\flexcan\kflexcan.h .
copy /Y ..\..\..\mqx\source\io\usb\if_dev_khci.h .
copy /Y ..\..\..\mqx\source\io\usb\if_host_khci.h .
copy /Y ..\..\..\mqx\source\io\usb\if_dev_ehci.h .
copy /Y ..\..\..\mqx\source\io\usb\if_host_ehci.h .
copy /Y ..\..\..\mqx\source\io\usb\usb_bsp.h .
copy /Y ..\..\..\mqx\source\io\usb\if_dev_ehci.h .
copy /Y ..\..\..\mqx\source\io\usb\if_dev_khci.h .
copy /Y ..\..\..\mqx\source\io\usb\if_host_ehci.h .
copy /Y ..\..\..\mqx\source\io\usb\if_host_khci.h .
copy /Y ..\..\..\mqx\source\io\pccard\apccard.h .
copy /Y ..\..\..\mqx\source\io\pccard\pccardflexbus.h .
copy /Y ..\..\..\mqx\source\io\pcflash\pcflash.h .
copy /Y ..\..\..\mqx\source\io\pcflash\ata.h .
copy /Y ..\..\..\mqx\source\io\rtc\krtc.h .
copy /Y ..\..\..\mqx\source\io\rtc\krtc_ext.h .
copy /Y ..\..\..\mqx\source\io\rtc\rtc.h .
copy /Y ..\..\..\mqx\source\io\i2c\i2c.h  .
copy /Y ..\..\..\mqx\source\io\i2c\i2c_ki2c.h .
copy /Y ..\..\..\mqx\source\io\tfs\tfs.h .
copy /Y ..\..\..\mqx\source\io\flashx\flashx.h .
copy /Y ..\..\..\mqx\source\io\flashx\freescale\flash_ftfe.h .
copy /Y ..\..\..\mqx\source\io\flashx\freescale\flash_mk70.h .
copy /Y ..\..\..\mqx\source\io\esdhc\esdhc.h .
copy /Y ..\..\..\mqx\source\io\sdcard\sdcard.h .
copy /Y ..\..\..\mqx\source\io\sdcard\sdcard_spi\sdcard_spi.h .
copy /Y ..\..\..\mqx\source\io\sdcard\sdcard_esdhc\sdcard_esdhc.h .
copy /Y ..\..\..\mqx\source\io\enet\ethernet.h .
copy /Y ..\..\..\mqx\source\io\enet\enet.h .
copy /Y ..\..\..\mqx\source\io\enet\enet_wifi.h .
copy /Y ..\..\..\mqx\source\io\enet\macnet\macnet_mk70.h .
copy /Y ..\..\..\mqx\source\io\enet\macnet\macnet_1588.h .
copy /Y ..\..\..\mqx\source\io\enet\phy\phy_dp83xxx.h .
copy /Y ..\..\..\mqx\source\io\tchres\tchres.h .
copy /Y ..\..\..\mqx\source\io\lpm\lpm.h .
copy /Y ..\..\..\mqx\source\io\lpm\lpm_kinetis.h .
copy /Y ..\..\..\mqx\source\io\cm\cm.h .
copy /Y ..\..\..\mqx\source\io\cm\cm_kinetis.h .
copy /Y ..\..\..\mqx\source\io\debug\iodebug.h .
copy /Y ..\..\..\mqx\source\io\nandflash\nandflash.h .
copy /Y ..\..\..\mqx\source\io\nandflash\nfc\nfc.h .
copy /Y ..\..\..\mqx\source\io\timer\timer_lpt.h .
copy /Y ..\..\..\mqx\source\io\hmi\hmi_uids.h .
copy /Y ..\..\..\mqx\source\io\hmi\hmi_tss_provider.h .
copy /Y ..\..\..\mqx\source\io\hmi\hmi_lwgpio_provider.h .
copy /Y ..\..\..\mqx\source\io\hmi\hmi_client.h .
copy /Y ..\..\..\mqx\source\io\hmi\btnled.h .
copy /Y ..\..\..\mqx\source\io\hmi\hmi_provider.h .
copy /Y ..\..\..\mqx\source\io\hmi\derivative.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_API.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_DataTypes.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_GPIO.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_Sensor.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_SensorCTS.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_SensorGPIO.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_SensorKBI.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_SensorPTI.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_SensorTIC.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_SensorTSI.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_StatusCodes.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_SystemSetupVal.h .\TSS
copy /Y ..\..\..\mqx\source\io\hmi\TSS\shared\TSS_Timer.h .\TSS

if NOT EXIST Generated_Code mkdir Generated_Code

rem Deleting old *.h files in bsp\Generated_Code directory
attrib -R Generated_Code\* >NUL 2>&1
del /Q Generated_Code\*.h  >NUL 2>&1

rem Copy default PE_LDD.h to bsp\Generated_Code
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\PE_LDD.h .\Generated_Code

rem  Compiler-specific BSP files
goto comp_%compiler%

rem  CodeWarrior Compiler
:comp_cw
copy /Y ..\..\..\mqx\source\io\hmi\TSS\lib_cw\TSS_KXX.a .\TSS
mkdir dbg
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\cw\intram.lcf .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\cw\intflash_sramdata.lcf .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\cw\intflash_ddrdata.lcf .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\cw\dbg\twrk70f120m.mem .\dbg
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\cw\dbg\init_kinetis.tcl .\dbg
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\cw\dbg\mass_erase_kinetis.tcl .\dbg

rem Copy Processor Expert generated files into Generated_Code directory
if NOT EXIST ..\..\..\mqx\build\cw10\bsp_twrk70f120m\Generated_Code\PE_LDD.h goto comp_done
copy /Y ..\..\..\mqx\build\cw10\bsp_twrk70f120m\Generated_Code\*.h .\Generated_Code
copy /Y ..\..\..\mqx\build\cw10\bsp_twrk70f120m\Sources\*.h .\Generated_Code
goto comp_done

rem  IAR Compiler
:comp_iar
:comp_ewarm
:comp_ewcf
copy /Y ..\..\..\mqx\source\io\hmi\TSS\lib_iar\TSS_KXX.a .\TSS
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\iar\intram.icf .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\iar\intflash_sramdata.icf .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\iar\intflash_ddrdata.icf .
goto comp_done

rem  Keil Compiler
:comp_keil
:comp_uv4
rem Keil is not able to build directly into lib folder. Need to copy
copy /Y ..\..\..\mqx\source\io\hmi\TSS\lib_uv4\TSS_KXX.lib .\TSS

if /I "%target%" == "debug"   copy /Y ..\..\..\mqx\build\uv4\twrk70f120m_IntFlashDebug\bsp_twrk70f120m_d.lib .
if /I "%target%" == "release" copy /Y ..\..\..\mqx\build\uv4\twrk70f120m_IntFlashRelease\bsp_twrk70f120m.lib .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\uv4\intflash.scf .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\uv4\intflash_sramdata.scf .
copy /Y ..\..\..\mqx\source\bsp\twrk70f120m\uv4\intflash_ddrdata.scf .
goto comp_done

rem  End of Compiler-specific files

:comp_done

rem Generate library readme file
call "%bat_file_dir%\write_readme.bat" > ..\twrk70f120m_lib.txt
echo BSP Post-build operations done