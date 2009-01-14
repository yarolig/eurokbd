call m.bat

SET PATH=c:\bin\rapi;%PATH%

rem pput c:\bin\rapi\itsutils.dll \Windows
c:\bin\rapi\pkill.exe ktest-arm.exe
c:\bin\rapi\pdel.exe "\Program Files\EuroKeyboard\eurokbd.dll"
c:\bin\rapi\pdel.exe "\Program Files\EuroKeyboard\ktest-arm.exe"
c:\bin\rapi\pput.exe eurokbd.dll "\Program Files\EuroKeyboard"
c:\bin\rapi\pput.exe ktest-arm.exe "\Program Files\EuroKeyboard"
c:\bin\rapi\prun.exe "\Program Files\EuroKeyboard\ktest-arm.exe"
