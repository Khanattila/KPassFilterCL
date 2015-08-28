/*
*	This file is part of KPassFilterCL,
*	Copyright(C) 2015  Khanattila.
*
*	KPassFilterCL is free software: you can redistribute it and/or modify
*	it under the terms of the GNU Lesser General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	KPassFilterCL is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*	GNU Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public License
*	along with KPassFilterCL. If not, see <http://www.gnu.org/licenses/>.
*/

#define VERSION "1.0.0"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define snprintf sprintf_s
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <fstream>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <fftw3.h>
#include "kernel.h"
#include "startchar.h"

#ifdef _WIN32
#include "avisynth.h"
#endif

enum mode_t { MAGNITUDE, PHASE };
enum filter_t { LOW_PASS, HIGH_PASS, BAND_PASS, MERGE_LOW, MERGE_HIGH, MERGE_BAND };

#ifdef __AVISYNTH_6_H__
class KPassFilterClass : public GenericVideoFilter {
private:
	PClip baby;
	const double lcutoff, hcutoff;
	const char *mode, *ocl_device;
	const bool lsb, info;
	const filter_t filter;
	fftwf_complex *in, *out;
	fftwf_plan p, q;
    cl_uint idmn[2];
	cl_platform_id platformID;
	cl_device_id deviceID;
	cl_context context;
	cl_program program;
	cl_kernel kernel;
	cl_mem mem_in[2], mem_out;
	bool avs_equals(VideoInfo *v, VideoInfo *w);
    cl_uint readBuffer(PVideoFrame &frm);
    cl_uint writeBuffer(PVideoFrame &frm);
public:
	KPassFilterClass(PClip _child, PClip _baby, const double _lcutoff, const double _hcutoff, const char *_mode, 
		const char *_ocl_device, const bool _lsb, const bool _info, filter_t filter, IScriptEnvironment* env);
	~KPassFilterClass();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};
#endif //__AVISYNTH_6_H__

template <typename T>T clamp(const T& value, const T& low, const T& high) {
	return value < low ? low : (value > high ? high : value);
}