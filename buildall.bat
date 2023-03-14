rem In Visual Studio open pcbuild.sln and build

cd PCBuild

call build -p Win32 -t Rebuild
call build -p x64 -t Rebuild

cd ..

