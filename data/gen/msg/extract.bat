@echo off
set BTOOL=C:\Software\BinderTool\BinderTool.exe
mkdir ..\text 2>NUL
for /D %%v in (*) do (
    echo %%v
    echo   Unpacking...
    %BTOOL% "%%v\item.msgbnd.dcx"
    %BTOOL% "%%v\item.msgbnd"
    %BTOOL% "%%v\menu.msgbnd.dcx"
    %BTOOL% "%%v\menu.msgbnd"
    del /q %%v\item.msgbnd %%v\menu.msgbnd
    echo   Extracting text strings from fmg...
    %BTOOL% "%%v\item\msg\%%v\NpcName.fmg"
    %BTOOL% "%%v\item\msg\%%v\PlaceName.fmg"
    %BTOOL% "%%v\item\msg\%%v\AccessoryName.fmg"
    %BTOOL% "%%v\item\msg\%%v\GemName.fmg"
    %BTOOL% "%%v\item\msg\%%v\GoodsName.fmg"
    %BTOOL% "%%v\item\msg\%%v\ProtectorName.fmg"
    %BTOOL% "%%v\item\msg\%%v\WeaponName.fmg"
    %BTOOL% "%%v\menu\msg\%%v\GR_MenuText.fmg"
    mkdir "..\text\%%v" 2>NUL
    echo   Copying...
    copy /y "%%v\item\msg\%%v\NpcName.txt" "..\text\%%v\" >NUL
    copy /y "%%v\item\msg\%%v\PlaceName.txt" "..\text\%%v\" >NUL
    copy /y "%%v\item\msg\%%v\AccessoryName.txt" "..\text\%%v\" >NUL
    copy /y "%%v\item\msg\%%v\GemName.txt" "..\text\%%v\" >NUL
    copy /y "%%v\item\msg\%%v\GoodsName.txt" "..\text\%%v\" >NUL
    copy /y "%%v\item\msg\%%v\ProtectorName.txt" "..\text\%%v\" >NUL
    copy /y "%%v\item\msg\%%v\WeaponName.txt" "..\text\%%v\" >NUL
    copy /y "%%v\menu\msg\%%v\GR_MenuText.txt" "..\text\%%v\" >NUL
    echo   Cleaning up...
    rmdir /s /q %%v\item %%v\menu
    echo  Done.
)