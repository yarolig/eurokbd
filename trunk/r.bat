SET PATH=c:\bin\rapi;%PATH%

rem pput c:\bin\rapi\itsutils.dll \Windows
c:\bin\rapi\pkill.exe kbdtest
c:\bin\rapi\pdel.exe "\Program Files\Euro\Keyboard\eurokbd.dll"
c:\bin\rapi\pput.exe eurokbd.dll "\Program Files\Euro\Keyboard"
c:\bin\rapi\prun.exe "\Program Files\kbdtest\kbdtest.exe"
