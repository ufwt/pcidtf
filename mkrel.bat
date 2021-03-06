@echo off

rem ===================================================================
rem Copyright (C) 2013-2014 Hiromitsu Sakamoto
rem PCI Device Test Framework
rem ===================================================================

if "%WDKDIR%" == "" set WDKDIR=C:\WinDDK\7600.16385.1
set RELDIR=pcidtf_test
set ARCH=i386
if "%AMD64%" == "1" set ARCH=amd64
set OBJDIR=obj%BUILD_ALT_DIR%\%ARCH%
set WDM_INF=win\wdm\%OBJDIR%\pcidtf_wdm.inf
set WDM_SYS=win\wdm\%OBJDIR%\pcidtf_wdm.sys
set KMDF_INF=win\kmdf\%OBJDIR%\pcidtf_kmdf.inf
set KMDF_SYS=win\kmdf\%OBJDIR%\pcidtf_kmdf.sys
set WDKCOINST=%WDKDIR%\redist\wdf\x86\WdfCoInstaller01009.dll
set DLL=api\%OBJDIR%\pcidtf.dll
set TESTAPP=testapp\%OBJDIR%\pcidtf_testapp.exe

if not exist %RELDIR% mkdir %RELDIR%

if exist %WDM_INF% copy %WDM_INF% %RELDIR%
if exist %WDM_SYS% copy %WDM_SYS% %RELDIR%

if exist %KMDF_INF% copy %KMDF_INF% %RELDIR%
if exist %KMDF_SYS% copy %KMDF_SYS% %RELDIR%

if exist %DLL% copy %DLL% %RELDIR%
if exist %TESTAPP% copy %TESTAPP% %RELDIR%

copy %MISCUTIL_DIR%\bin\%ARCH%\miscutil_xpcf.dll %RELDIR%
copy %MISCUTIL_DIR%\sys\%ARCH%\miscutil_xpcf.sys %RELDIR%
copy %MISCUTIL_DIR%\sys\%ARCH%\miscutil_drvbase.sys %RELDIR%
copy %MISCUTIL_DIR%\sys\%ARCH%\miscutil_drvutil.sys %RELDIR%

copy %WDKCOINST% %RELDIR%

copy win\test %RELDIR%

:end
set RELDIR=
set ARCH=
set OBJDIR=
set WDM_INF=
set WDM_SYS=
set KMDF_INF=
set KMDF_SYS=
set WDKCOINST=
set DLL=
set TESTAPP=
