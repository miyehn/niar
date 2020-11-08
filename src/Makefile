all:
	ispc -O2 --target=avx1-i32x8 --arch=x86-64 Pathtracer/pathtracer_kernel.ispc -h Pathtracer/pathtracer_kernel_ispc.h
	ispc -O2 --target=avx1-i32x8 --arch=x86-64 Pathtracer/pathtracer_kernel.ispc -o ../build/pathtracer_kernel.o 