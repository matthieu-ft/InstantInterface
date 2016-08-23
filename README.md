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

## Author

Matthieu Fraissinet-Tachet (www.matthieu-ft.com)
