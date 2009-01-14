nmake clean
del \1.log
nmake all cab release=1

del ..\eurokbd.CAB ..\eurokbd.cab.zip ..\eurokbd-src.zip

copy eurokbd.CAB ..
nmake clean

cd ..
zip eurokbd.cab.zip eurokbd.CAB
zip -r eurokbd-src.zip eurokbd