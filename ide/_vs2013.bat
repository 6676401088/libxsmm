@ECHO OFF
SETLOCAL

cd ..
bash -c "make realclean ; make ALIGNED_STORES=1 header source main"
cd ide

SET LIBXSMMROOT=%~d0%~p0\..

CALL %~d0"%~p0"_vs.bat vs2013
START %~d0"%~p0"_vs2013.sln

ENDLOCAL