#!/usr/bin/env python3

#create GL.hpp / GL.cpp by parsing everything from glcorearb.h (why not the regsistry xml, hmmmm?) and selecting only things that are core through version 3_3.
#get glcorearb.h from https://github.com/KhronosGroup/OpenGL-Registry/raw/master/api/GL/glcorearb.h

import re

filtered = []
lookups = []
fps = []

with open('glcorearb.h', 'r') as f:
	in_version = None
	in_notice = False
	did_notice = False
	for line in f:
		line = line.strip()
		if line == "/*" and not did_notice:
			filtered.append(line)
			in_notice = True
			did_notice = True
			continue
		if in_notice:
			filtered.append(line)
			if line == "*/":
				in_notice = False
			continue
		m = re.match(r"^#define (GL_VERSION_(\d)_(\d)) 1$", line)
		if m != None:
			assert(in_version == None)
			in_version = m.group(1)
			major = int(m.group(2))
			minor = int(m.group(3))
			if (major,minor) <= (1,1):
				filtered.append("\n// from " + in_version + ":")
				mode = "all_proto"
			elif (major,minor) <= (4,5):
				filtered.append("\n// from " + in_version + ":")
				mode = "win_pointer"
				else_block = []
			else:
				mode = "skip"
			continue
		if in_version:
			#check for a "#define GL_SOMETHING_SOMETHING 0xABCD" sorts of lines:
			m = re.match(r"^#define", line)
			if m != None:
				if mode != "skip":
					filtered.append(line)
				continue

			#check for function pointer typedef lines:
			m = re.match(r"^typedef.*APIENTRYP", line)
			if m != None:
				#it's a function pointer typedef line
				#if mode == "win_pointer":
				#	filtered.append(line)
				continue

			#check for other typedef lines:
			m = re.match(r"^typedef", line)
			if m != None:
				if mode != "skip":
					m = re.match(r"^typedef khronos_([^\s]+) ([^\s]+)$", line)
					if m == None:
						filtered.append(line)
					else:
						ty = m.group(1)
						if ty == "ssize_t":
							ty = "khronos_ssize_t";
						elif ty == "float_t":
							ty = "float";
						filtered.append("typedef " + ty + " " + m.group(2))
				continue

			#check for function prototype lines:
			m = re.match(r"GLAPI(.*)APIENTRY ([^\s]+) (.*)$", line)
			if m != None:
				if mode == "all_proto":
					filtered.append(line)
				elif mode == "win_pointer":
					rt = m.group(1)
					fn = m.group(2)
					ag = m.group(3)
					filtered.append("GLAPI" + rt + "(APIENTRYFP " + fn + ") " + ag)
					fps.append(rt + "(APIENTRYFP " + fn + ") " + ag)
					lookups.append("DO(" + fn + ")")
					#filtered.append("extern PFNGL" + uc + "PROC gl" + lc + ";")
					#filtered.append("DO(" + uc + ", " + lc + ")\n")
				continue

			if line == "#ifdef GL_GLEXT_PROTOTYPES":
				continue

			if line == "#endif":
				continue

			#check for version endif line:
			m = re.match(r"^#endif /\* " + in_version + " \*/$", line)
			if m != None:
				in_version = None
				continue
			print("ignoring: " + line)



with open("GL.hpp", "w") as f:
	print("""#pragma once

/*
 *
 * Function prototypes/pointers for OpenGL 4.5 core, with minimal namespace pollution.
 * Call init_GL() after you have created a context.
 *
 * On Windows, OpenGL 1.0 & 1.1 are prototypes, the rest are pointers
 *  initialized by init_GL(). This is because the 1.1/1.0 entries are
 *  the only ones provided directly by OpenGL32.dll 
 *
 * On Linux, all are prototypes.
 *
 * On MacOS, all are prototypes.
 *
 * This file has been automatically generated from glcorearb.h by make-GL.py
 *
 */

void init_GL(); //will throw on failure.

extern "C" {

#include <stdint.h>

#ifdef _WIN32
	#define APIENTRY __stdcall //see: https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types
	#define APIENTRYFP APIENTRY * //these are function pointers on windows
#else
	#define APIENTRY
	#define APIENTRYFP
#endif

//this is how khronos_ssize_t gets defined in khrplatform.h:
#ifdef _WIN64
typedef signed   long long int khronos_ssize_t;
#else
typedef signed   long  int     khronos_ssize_t;
#endif

#define GLAPI extern
""", file=f)

	print("\n".join(filtered), file=f)

	print("""
}""", file=f)


with open("GL.cpp", "w") as f:
	print("""#include "GL.hpp"

#include <SDL.h>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
	#define DO(fn) \\
		fn = (decltype(fn))SDL_GL_GetProcAddress(#fn); \\
		if (!fn) { \\
			throw std::runtime_error("Error binding " #fn); \\
		}
#else
	#define DO(fn)
#endif

void init_GL() {""", file=f)
	print("\t" + "\n\t".join(lookups),file=f)
	print("""}
#ifdef _WIN32""", file=f)
	print("\t" + "\n\t".join(fps),file=f)
	print("""#endif""", file=f)
