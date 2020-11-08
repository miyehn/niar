### Build instruction

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
Or see README.md on master branch for using `./niar` interactively.

### Configuration

Currently the only relevant configuration is `Pathtracer.ISPC` inside `config.ini`.

### Notes

[This](https://ingowald.blog/2018/06/25/ispc-bag-of-tricks-part-2-on-the-calling-back-and-forth-between-ispc-and-c-c/) seems useful.