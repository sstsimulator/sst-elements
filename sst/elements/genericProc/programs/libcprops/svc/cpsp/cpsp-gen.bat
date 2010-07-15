@echo off

call setpath.bat

set MAKE=NMAKE

if exist CWD del /q CWD
cd > CWD
for /f "tokens=*" %%i in (CWD) do set CWD=%%i
del /q CWD
set logfile=%CWD%\cpsp-gen.log

set makefile=Makefile.cpsp
if not "%1" == "" set makefile="%2\%makefile%"

if exist DATE del /q DATE
date /t > DATE
for /f "tokens=*" %%i in (DATE) do set DATE=%%i
if exist TIME del /q TIME
time /t > TIME
for /f "tokens=*" %%i in (TIME) do set TIME=%%i
del /q DATE TIME

newline >> %logfile%
echo %DATE% %TIME% %0 starting >> %logfile%

if exist FNAME del /q FNAME
echo %1 > FNAME
strsubst FNAME "./" ""
for /f "tokens=*" %%i in (FNAME.strsubst) do set filename=%%i
del /q FNAME FNAME.strsubst
echo compiling %filename% >> %logfile%

if not exist "%filename%" (
	echo %filename% not found >> %logfile%
	goto :EOF
)

set CPSP_SRC_lit=%filename%

set HAYSTACK=%1
set NEEDLE="\\"
set REVERSE="-r"
call :match

set x=%PREFIX%
set y=%SUFFIX%

if exist TARGET del /q TARGET
if exist TARGET.strsubst del /q TARGET.strsubst
if "%1" == "%y%" (
  set _prefix=
  set fname=%1
  set cpsp_target="_cpsp_%y%"
)
if not "%1" == "%y%" (
  set _prefix=%%x
  echo using prefix %_prefix%
  set fname=%y%
  set cpsp_target=%x%\_cpsp_%y%
)

echo %cpsp_target% > TARGET
strsubst TARGET ".cpsp" ".dll"
for /f "tokens=*" %%i in (TARGET.strsubst) do set end_target=%%i

del /q TARGET TARGET.strsubst
set gen_target="%x%%end_target%"

echo %cpsp_target% > TARGET
strsubst TARGET ".cpsp" ".c"
for /f "tokens=*" %%i in (TARGET.strsubst) do set CPSP_TARGET_lit=%%i
del /q TARGET TARGET.strsubst

rem if the c file exists but is 0 bytes long, delete it
rem if [ -r $CPSP_TARGET_lit -a ! -s $CPSP_TARGET_lit ] ; then
rem   rm -f $CPSP_TARGET_lit
rem fi

set CPSP_TARGET=%CPSP_TARGET_lit%
set CPSP_TARGET_DLL=%end_target%
set CPSP_SRC=%CPSP_SRC_lit%

set CPSPCFLAGS=
set CPSPLDFLAGS=
set CPSPLIBS=

if exist __curr del /q __curr
echo _make_%CPSP_SRC% | strsubst "-" "." "" | strsubst "-" "\\" "" > __curr
for /f "tokens=*" %%i in (__curr) do set CPSP_TMP=%%i
if exist __curr del /q __curr
if exist %CPSP_TMP% del /q %CPSP_TMP%
newline

find "CPSPCFLAGS" %CPSP_SRC% | filter "---" | strsubst - "<%%@" "" | strsubst "-" ">" "" | strsubst - "CPSPCFLAGS" "" | strsubst - " " "" > %CPSP_TMP%
for /f "tokens=*" %%i in (%CPSP_TMP%) do set CPSPCFLAGS=%%i
del /q %CPSP_TMP%
find "CPSPLDFLAGS" test.cpsp | filter "---" | strsubst - "<%%@" "" | strsubst - ">" "" | strsubst - CPSPLDFLAGS "" | strsubst - " " "" > %CPSP_TMP%
for /f "tokens=*" %%i in (%CPSP_TMP%) do set CPSPLDFLAGS=%%i
del /q %CPSP_TMP%
find "CPSPLIBS" test.cpsp | filter "---" | strsubst - "<%%@" "" | strsubst - ">" "" | strsubst - CPSPLIBS "" | strsubst - " " "" > %CPSP_TMP%
for /f "tokens=*" %%i in (%CPSP_TMP%) do set CPSPLIBS=%%i
del /q %CPSP_TMP%

%MAKE% /f %makefile% cpsp-page >> %logfile% 2>&1

if errorlevel 1 (
  echo can't make %CPSP_TARGET_DLL% from cpsp-page %CPSP_SRC% >> %logfile%
  goto :EOF
)

echo target: %end_target% >> %logfile%
if exist %end_target% copy /y %end_target% %end_target%~ > nul: 2>&1

move %gen_target% %end_target% >> %logfile% 2>&1

goto :EOF

:match
set MATCH=
set PREFIX=
set SUFFIX=
match %HAYSTACK% %NEEDLE% %REVERSE%
if errorlevel 1 goto :EOF

for /f "tokens=*" %%i in (__prefix) do set PREFIX=%%i
for /f "tokens=*" %%i in (__match) do set MATCH=%%i
for /f "tokens=*" %%i in (__suffix) do set SUFFIX=%%i
if exist __prefix del __prefix
if exist __match del __match
if exist __suffix del __suffix
goto :EOF

