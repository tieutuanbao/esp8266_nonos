@echo off
./make.exe clean && ./make.exe -j12 && ./make.exe buildfs && ./make.exe flash && ./make.exe uploadfs && ./make.exe monitor