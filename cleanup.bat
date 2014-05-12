@echo off

rem =================================================================
rem Copyright (C) 2013-2014 Hiromitsu Sakamoto
rem PCI Device Test Framework
rem =================================================================

set MODCLEAN=%MISCUTIL_DIR%\etc\modclean.bat

rem ***** Delete intermediate file *****
cmd /c %MODCLEAN% .
cmd /c %MODCLEAN% win
cmd /c %MODCLEAN% win\common
cmd /c %MODCLEAN% win\wdm
cmd /c %MODCLEAN% win\kmdf
cmd /c %MODCLEAN% api
cmd /c %MODCLEAN% testapp

rem ***** Delete release package *****
if exist pcidtf_test rd /s/q pcidtf_test
if exist pcidtf_sdk rd /s/q pcidtf_sdk

set MODCLEAN=
