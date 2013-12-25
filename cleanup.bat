@echo off

rem =================================================================
rem Copyright (C) 2013 Hiromitsu Sakamoto
rem PCI Device Test Framework
rem =================================================================

set MODCLEAN=..\miscutil\win\modclean.bat

rem ***** Delete intermediate file *****
cmd /c %MODCLEAN% .
cmd /c %MODCLEAN% win
cmd /c %MODCLEAN% win\common
cmd /c %MODCLEAN% win\wdm
cmd /c %MODCLEAN% win\kmdf
cmd /c %MODCLEAN% api
cmd /c %MODCLEAN% testapp

rem ***** Delete release package *****
if exist pcidtf rd /s/q pcidtf
if exist pcidtf-sdk rd /s/q pcidtf-sdk

set MODCLEAN=
