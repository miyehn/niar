all:
	g++ -c -m64 -O3 -Wall ./tasksys.cpp -o ../build/tasksys.o -lm -lpthread
	ispc -O2 --target=avx2-i32x8 --arch=x86-64 Pathtracer/pathtracer_kernel.ispc -h Pathtracer/pathtracer_kernel_ispc.h -o ../build/pathtracer_kernel.o
# all:
# 	CXX=g++ -m64
# 	CXXFLAGS=-I../common -Iobjs/ -O3 -Wall
# 	ISPC=ispc
# 	# note: change target to sse4 for SSE capable machines
# 	ISPCFLAGS=-O2 --target=avx1-i32x8 --arch=x86-64

# 	OBJDIR=objs
# 	COMMONDIR=.

# 	PPM_OBJ=$(addprefix $(OBJDIR)/, $(subst $(COMMONDIR)/,, $(PPM_CXX:.cpp=.o)))

# 	TASKSYS_CXX=$(COMMONDIR)/tasksys.cpp
# 	TASKSYS_LIB=-lpthread
# 	TASKSYS_OBJ=$(addprefix $(OBJDIR)/, $(subst $(COMMONDIR)/,, $(TASKSYS_CXX:.cpp=.o)))

# 	default: $(APP_NAME)

# 	.PHONY: dirs clean

# 	dirs:
# 	        /bin/mkdir -p $(OBJDIR)/

# 	clean:
# 	        /bin/rm -rf $(OBJDIR) *.ppm *~ $(APP_NAME)

# 	OBJS=$(OBJDIR)/main.o $(OBJDIR)/mandelbrotSerial.o $(OBJDIR)/mandelbrot_ispc.o          $(PPM_OBJ) $(TASKSYS_OBJ)

# 	$(APP_NAME): dirs $(OBJS)
# 	        $(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lm $(TASKSYS_LIB)

# 	$(OBJDIR)/%.o: %.cpp
# 	        $(CXX) $< $(CXXFLAGS) -c -o $@

# 	$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp
# 	    $(CXX) $< $(CXXFLAGS) -c -o $@

# 	$(OBJDIR)/main.o: $(OBJDIR)/mandelbrot_ispc.h $(COMMONDIR)/CycleTimer.h

# 	$(OBJDIR)/%_ispc.h $(OBJDIR)//%_ispc.o: %.ispc
# 	        $(ISPC) $(ISPCFLAGS) $< -o $(OBJDIR)/$*_ispc.o -h $(OBJDIR)/$*_ispc.h