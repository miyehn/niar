## SIMD Parallelization of a Path Tracer

Changes on this branch are part of our CMU 15-418F20 final project. Project members include Michelle Ma and I (Rain Du). See our [project site](https://miyehn.me/SIMD-pathtracer/) for more information.

<img src="img/ispc-render.jpg" width=540>

### Build instructions

Because I'm dumb and don't know how to use cmake to build ispc yet. Currently ispc is built with a separate Makefile.
```
cd src
make
cd ../build
cmake ..
make
```

### Usage

Something like:
```
./niar -w 200 -h 150 -o output.ppm
```
Or see `README.md` on master branch for using `./niar` interactively.

### Configuration

See the Pathtracer section in `config.ini`.
