@echo off

set RELDIR=pcidtf_sdk
set OBJX86=objfre_wxp_x86\i386
set OBJX64=objfre_wnet_amd64\amd64

if not exist %RELDIR% mkdir %RELDIR%
if not exist %RELDIR%\include mkdir %RELDIR%\include
if not exist %RELDIR%\lib mkdir %RELDIR%\lib
if not exist %RELDIR%\lib\i386 mkdir %RELDIR%\lib\i386
if not exist %RELDIR%\lib\amd64 mkdir %RELDIR%\lib\amd64
if not exist %RELDIR%\bin mkdir %RELDIR%\bin
if not exist %RELDIR%\bin\i386 mkdir %RELDIR%\bin\i386
if not exist %RELDIR%\bin\amd64 mkdir %RELDIR%\bin\amd64

copy include\pcidtf_api.h %RELDIR%\include
copy api\%OBJX86%\*.lib %RELDIR%\lib\i386
copy api\%OBJX64%\*.lib %RELDIR%\lib\amd64
copy api\%OBJX86%\*.dll %RELDIR%\bin\i386
copy api\%OBJX64%\*.dll %RELDIR%\bin\amd64

set RELDIR=
set OBJX86=
set OBJX64=
