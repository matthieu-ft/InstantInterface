The MIT License (MIT)

Copyright (c) 2016 Matthieu Fraissinet-Tachet (www.matthieu-ft.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# InstantInterface
cpp library for easily creating web interfaces for float, int, bool parameters and lambda functions, accessible on local network.

##Requirements
- [cmake](https://cmake.org/)
- c++14 support
- [websocketpp](https://github.com/zaphoyd/websocketpp)
- [boost](http://www.boost.org/) (system, thread)

## Build the library

```
mkdir build && cd build
cmake ..
make -j
```

## Run the examples

The compilation generates two executables `basic_interface` and `dynamic_configurations`.
They show how to use the different functionnalities of the libraries.
Before running them, go to the folder where the binaries for the executables have been generated and **create a text 
file called** `pathToWebInterface.txt` in which you write the full path to the folder containing 
the html, css and css of the web interface, which is to say: `INSTANT_INTERFACE_ROOT/app/dist/` (don't forget to add
the slash at the end) where `INSTANT_INTERFACE_ROOT` is the path to root directory of the InstantInterface project.
Then you are ready to run one of the exectubles. The web interface that is generated is then accessible under `localhost:9000`.
