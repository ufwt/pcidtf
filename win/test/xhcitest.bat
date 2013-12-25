@echo off

pcidtf_testapp dev info 0
if errorlevel 1 goto end

pcidtf_testapp cfg read 0 0 4
pcidtf_testapp cfg read 0 4 4
pcidtf_testapp cfg read 0 8 4

echo ***** Disable memory I/O *****
pcidtf_testapp cfg write 0 4 4 0
pcidtf_testapp reg read 0 0 0 4
echo ***** Enable memory I/O *****
pcidtf_testapp cfg write 0 4 4 0x100106

pcidtf_testapp reg read 0 0 0 4
pcidtf_testapp reg read 0 0 4 4
pcidtf_testapp reg read 0 0 8 4
pcidtf_testapp reg read 0 0 12 4
pcidtf_testapp reg read 0 0 16 4

echo ***** Reset host controller *****
pcidtf_testapp reg write 0 0 0x20 4 2
pause
pcidtf_testapp reg read 0 0 0x20 4
pcidtf_testapp reg read 0 0 0x24 4
echo ***** Start host controller *****
pcidtf_testapp reg write 0 0 0x20 4 1
pause
pcidtf_testapp reg read 0 0 0x20 4
pcidtf_testapp reg read 0 0 0x24 4
echo ***** Stop host controller *****
pcidtf_testapp reg write 0 0 0x20 4 0
pause
pcidtf_testapp reg read 0 0 0x20 4
pcidtf_testapp reg read 0 0 0x24 4

rem ===== DMA buffer test =====
pcidtf_testapp dma alloc 0 64
pcidtf_testapp dma info 0 1
pcidtf_testapp dma write 0 1 0 4 0x12345678
pcidtf_testapp dma read 0 1 0 4
pcidtf_testapp dma free 0 1

:end
