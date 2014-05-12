@echo off

if "%WDKDIR%" == "" set WDKDIR=C:\WinDDK\7600.16385.1

set RELDIR=pcidtf_sdk
set OBJX86=objfre_wxp_x86\i386
set OBJX64=objfre_wnet_amd64\amd64
set MISCUTIL=..\miscutil
set WDFREDIST=%WDKDIR%\redist\wdf
set WDFCOINST=WdfCoInstaller01009

rem ***** Create directories *****
if not exist %RELDIR% mkdir %RELDIR%
if not exist %RELDIR%\include mkdir %RELDIR%\include
if not exist %RELDIR%\include\xpcf mkdir %RELDIR%\include\xpcf
if not exist %RELDIR%\lib mkdir %RELDIR%\lib
if not exist %RELDIR%\lib\i386 mkdir %RELDIR%\lib\i386
if not exist %RELDIR%\lib\amd64 mkdir %RELDIR%\lib\amd64
if not exist %RELDIR%\bin mkdir %RELDIR%\bin
if not exist %RELDIR%\bin\i386 mkdir %RELDIR%\bin\i386
if not exist %RELDIR%\bin\amd64 mkdir %RELDIR%\bin\amd64
if not exist %RELDIR%\sys mkdir %RELDIR%\sys
if not exist %RELDIR%\sys\i386 mkdir %RELDIR%\sys\i386
if not exist %RELDIR%\sys\amd64 mkdir %RELDIR%\sys\amd64

rem ***** Copy header files *****
copy include\pcidtf_api.h %RELDIR%\include
copy %MISCUTIL%\include\xpcf\inttypes.h %RELDIR%\include\xpcf

rem ***** Copy DLL files *****
copy api\%OBJX86%\*.lib %RELDIR%\lib\i386
copy api\%OBJX64%\*.lib %RELDIR%\lib\amd64
copy api\%OBJX86%\*.dll %RELDIR%\bin\i386
copy api\%OBJX64%\*.dll %RELDIR%\bin\amd64

rem ***** Copy driver files *****
copy win\wdm\%OBJX86%\*.inf %RELDIR%\sys\i386
copy win\wdm\%OBJX86%\*.sys %RELDIR%\sys\i386
copy win\wdm\%OBJX86%\pcidtf*.pdb %RELDIR%\sys\i386
copy win\wdm\%OBJX64%\*.inf %RELDIR%\sys\amd64
copy win\wdm\%OBJX64%\*.sys %RELDIR%\sys\amd64
copy win\wdm\%OBJX64%\pcidtf*.pdb %RELDIR%\sys\amd64
copy win\kmdf\%OBJX86%\*.inf %RELDIR%\sys\i386
copy win\kmdf\%OBJX86%\*.sys %RELDIR%\sys\i386
copy win\kmdf\%OBJX86%\pcidtf*.pdb %RELDIR%\sys\i386
copy win\kmdf\%OBJX64%\*.inf %RELDIR%\sys\amd64
copy win\kmdf\%OBJX64%\*.sys %RELDIR%\sys\amd64
copy win\kmdf\%OBJX64%\pcidtf*.pdb %RELDIR%\sys\amd64
copy %WDFREDIST%\x86\%WDFCOINST%.dll %RELDIR%\sys\i386
copy %WDFREDIST%\amd64\%WDFCOINST%.dll %RELDIR%\sys\amd64

set RELDIR=
set OBJX86=
set OBJX64=
set MISCUTIL=
set WDFREDIST=
set WDFCOINST=
