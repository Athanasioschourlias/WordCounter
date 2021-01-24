##Warnig!!!

If you wish to compile it yourself outside the project it is important to change the variable inside the fopen() from "../output.txt" TO "./output.txt" 
so it will generate the file at the right place since in order to run the executable with cmake you have to be inside the 
cmake-build-debug directory.


## Usage

To install c-make, linux

```
https://vitux.com/how-to-install-cmake-on-ubuntu-18-04/
```

Compile an existing build
--

while you are at the ./it219113/cmake-build-debug compile using

```
cmake --build .
```

Run the build
--
```
./it219113 <Directory> 
```

New build
--

To make a new build, create a folder with the desired name and cd in to that folder and  run CMake to configure the project and generate a native build system:
```
mkdir <folder name>

cd <folder name>

cmake ../<exec name>
```

Finally to run, while you are in the build folder
```
./<exec name> arg1 arg2 ..
```