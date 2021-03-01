ISPC_TASK_OBJ=../build/tasksys.o
ISPC_KERNEL_OBJ=../build/pathtracer_kernel.o

.PHONY: all
all: $(ISPC_TASK_OBJ) $(ISPC_KERNEL_OBJ)

$(ISPC_TASK_OBJ): tasksys.cpp
	g++ -c -m64 -O3 -Wall ./tasksys.cpp -lm -lpthread -o $@

$(ISPC_KERNEL_OBJ): Pathtracer/pathtracer_kernel.ispc
	ispc -O2 --target=avx2-i32x8 --arch=x86-64 Pathtracer/pathtracer_kernel.ispc -h Pathtracer/pathtracer_kernel_ispc.h -o $@