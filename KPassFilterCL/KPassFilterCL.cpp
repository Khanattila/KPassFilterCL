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

#define DFT_lcutoff FLT_MIN
#define DFT_hcutoff FLT_MAX
#define DFT_mode "MAGNITUDE"
#define DFT_ocl_device "DEFAULT"
#define DFT_lsb false
#define DFT_info false

#define GET_MSB(val) (val >> 8 & 0xFF)
#define GET_LSB(val) (val & 0xFF)
#define BIT_16(msb, lsb) (msb << 8 | lsb)

#include "KPassFilterCL.h"

//////////////////////////////////////////
// AviSynthEquals
#ifdef __AVISYNTH_6_H__
inline bool KPassFilterClass::avs_equals(VideoInfo *v, VideoInfo *w) {
	return v->width == w->width && v->height == w->height && v->fps_numerator == w->fps_numerator
		&& v->fps_denominator == w->fps_denominator && v->num_frames == w->num_frames;
}
#endif //__AVISYNTH_6_H__

//////////////////////////////////////////
// AviSynthMemoryManagement 
#ifdef __AVISYNTH_6_H__
inline cl_uint KPassFilterClass::readBuffer(PVideoFrame &frm) {

    cl_uint ret = CL_SUCCESS;
    if (lsb) {
        int dstY_s = frm->GetPitch(PLANAR_Y);
        uint8_t *dstYmp = frm->GetWritePtr(PLANAR_Y);
        uint8_t *dstYlp = dstY_s * idmn[1] + dstYmp;
        int value;
        #pragma omp parallel for private(value);
        for (int y = 0; y < (int) idmn[1]; y++) {
            for (int x = 0; x < (int) idmn[0]; x++) {
                value = (int) clamp(out[y * idmn[0] + x][0], 0.0f, 65025.0f);
                dstYmp[y * dstY_s + x] = GET_MSB(value);
                dstYlp[y * dstY_s + x] = GET_LSB(value);
            }
        }
    } else {
        int dstY_s = frm->GetPitch(PLANAR_Y);
        uint8_t *dstY = frm->GetWritePtr(PLANAR_Y);
        for (int y = 0; y < (int) idmn[1]; y++) {
            for (int x = 0; x < (int) idmn[0]; x++) {
                dstY[y * dstY_s + x] = (uint8_t) (clamp(in[x + y * idmn[0]][0], 0.0f, 255.0f));
            }
        }
    }   
    return ret;
}

inline cl_uint KPassFilterClass::writeBuffer(PVideoFrame &frm) {

    cl_uint ret = CL_SUCCESS;
    if (lsb) {
        int srcY_s = frm->GetPitch(PLANAR_Y);
        const uint8_t *srcYmp = frm->GetReadPtr(PLANAR_Y);
        const uint8_t *srcYlp = srcY_s * idmn[1] + srcYmp;
        #pragma omp parallel for
        for (int y = 0; y < (int) idmn[1]; y++) {
            for (int x = 0; x < (int) idmn[0]; x++) {
                in[y * idmn[0] + x][0] = (float) BIT_16(srcYmp[y * srcY_s + x], srcYlp[y * srcY_s + x]);
                in[y * idmn[0] + x][1] = 0.0f;
            }
        }
    } else {
        int srcY_s = frm->GetPitch(PLANAR_Y);
        const uint8_t *srcY = frm->GetReadPtr(PLANAR_Y);
        #pragma omp parallel for
        for (int y = 0; y < (int) idmn[1]; y++) {
            for (int x = 0; x < (int) idmn[0]; x++) {
                in[y * idmn[0] + x][0] = srcY[y * srcY_s + x];
                in[y * idmn[0] + x][1] = 0.0f;
            }
        }
    }
    return ret;
}
#endif //__AVISYNTH_6_H__

//////////////////////////////////////////
// AviSynthInit
#ifdef __AVISYNTH_6_H__
KPassFilterClass::KPassFilterClass(PClip _child, PClip _baby, const double _lcutoff, const double _hcutoff, 
	const char *_mode, const char  *_ocl_device, const bool _lsb, const bool _info, filter_t _filter, 
	IScriptEnvironment* env) : GenericVideoFilter(_child), baby(_baby), lcutoff(_lcutoff), hcutoff(_hcutoff), 
	mode(_mode), ocl_device(_ocl_device), lsb(_lsb), info(_info), filter(_filter) {

	// Checks AviSynth Version.
	env->CheckVersion(6);

	// Checks user value.
	VideoInfo rvi = baby->GetVideoInfo();
	if (!avs_equals(&vi, &rvi))
		env->ThrowError("KPassFilterCL: clip2 do not math clip1!");
	if (!vi.IsPlanar() || !vi.IsYUV()) 
		env->ThrowError("KPassFilterCL: planar YUV data only!");
	if (lcutoff < FLT_MIN || hcutoff < FLT_MIN || lcutoff > FLT_MAX || hcutoff > FLT_MAX) {
		if (filter == BAND_PASS || filter == MERGE_BAND)
			env->ThrowError("KPassFilterCL: lcutoff/hcutoff must be in range [FLT_MIN, FLT_MAX]!");
		else
			env->ThrowError("KPassFilterCL: cutoff must be in range [FLT_MIN, FLT_MAX]!");
	}
    mode_t mode_i = MAGNITUDE;
	if (!strcasecmp(mode, "MAGNITUDE")) {
		mode_i = MAGNITUDE;
	} else if (!strcasecmp(mode, "PHASE")) {
		mode_i = PHASE;
	} else {
		env->ThrowError("KPassFilterCL: mode must be magnitude or phase!");
	}
	cl_device_type device = NULL;
	if (!strcasecmp(ocl_device, "CPU")) {
		device = CL_DEVICE_TYPE_CPU;
	} else if (!strcasecmp(ocl_device, "GPU")) {
		device = CL_DEVICE_TYPE_GPU;
	} else if (!strcasecmp(ocl_device, "ACCELERATOR")) {
		device = CL_DEVICE_TYPE_ACCELERATOR;
	} else if (!strcasecmp(ocl_device, "DEFAULT")) {
		device = CL_DEVICE_TYPE_DEFAULT;
	} else {
		env->ThrowError("KPassFilterCL: device_type must be cpu, gpu, accelerator or default!");
	}

    // Gets PlatformID and DeviceID.
    cl_uint num_platforms = 0, num_devices = 0;
    cl_bool img_support = CL_FALSE, device_aviable = CL_FALSE;
    cl_int ret = clGetPlatformIDs(0, NULL, &num_platforms);
    if (num_platforms == 0) {
        env->ThrowError("KPassFilterCL: no opencl platforms available!");
    }
    cl_platform_id *temp_platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * num_platforms);
    ret |= clGetPlatformIDs(num_platforms, temp_platforms, NULL);
    if (ret != CL_SUCCESS) {
        env->ThrowError("KPassFilterCL: AviSynthCreate error (clGetPlatformIDs)!");
    }
    for (cl_uint i = 0; i < num_platforms; i++) {
        ret |= clGetDeviceIDs(temp_platforms[i], device, 0, 0, &num_devices);
        cl_device_id *temp_devices = (cl_device_id*) malloc(sizeof(cl_device_id) * num_devices);
        ret |= clGetDeviceIDs(temp_platforms[i], device, num_devices, temp_devices, NULL);
        for (cl_uint j = 0; j < num_devices; j++) {
            ret |= clGetDeviceInfo(temp_devices[j], CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &img_support, NULL);
            if (device_aviable == CL_FALSE && img_support == CL_TRUE) {
                platformID = temp_platforms[i];
                deviceID = temp_devices[j];
                device_aviable = CL_TRUE;
            }
        }
        free(temp_devices);
    }
    free(temp_platforms);
    if (ret != CL_SUCCESS && ret != CL_DEVICE_NOT_FOUND) {
        env->ThrowError("KPassFilter: AviSynthCreate error (clGetDeviceIDs)!");
    } else if (device_aviable == CL_FALSE) {
        env->ThrowError("KPassFilter: opencl device not available!");
    }

	// Creates an OpenCL context and 2D images.
	context = clCreateContext(NULL, 1, &deviceID, NULL, NULL, NULL);
	const cl_image_format image_format = { CL_RG, CL_FLOAT };
	idmn[0] = vi.width;
	idmn[1] = lsb ? (vi.height / 2) : (vi.height);
	mem_in[0] = clCreateImage2D(context, CL_MEM_READ_ONLY, &image_format, idmn[0], idmn[1], 0, NULL, NULL);
    mem_in[1] = clCreateImage2D(context, CL_MEM_READ_ONLY, &image_format, idmn[0], idmn[1], 0, NULL, NULL);
    mem_out = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &image_format, idmn[0], idmn[1], 0, NULL, NULL);

	// Allocates FFTW data.
    in = fftwf_alloc_complex(idmn[0] * idmn[1]);
    out = fftwf_alloc_complex(idmn[0] * idmn[1]);
    p = fftwf_plan_dft_2d(idmn[1], idmn[0], in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    q = fftwf_plan_dft_2d(idmn[1], idmn[0], out, in, FFTW_BACKWARD, FFTW_ESTIMATE);
	
	// Creates and Build a program executable from the program source.
	program = clCreateProgramWithSource(context, 1, &source_code, NULL, NULL);
	char options[4048];
	snprintf(options, 4048, "-cl-single-precision-constant -cl-denorms-are-zero  -cl-fast-relaxed-math -Werror \
        -D PFK_N=%lf -D PFK_MODE=%i -D PFK_FILTER=%i -D PFK_LCUTOFF=%lf -D PFK_HCUTOFF=%lf",
        (double) (idmn[0] * idmn[1]), mode_i, filter, lcutoff, hcutoff);
	ret = clBuildProgram(program, 1, &deviceID, options, NULL, NULL);
	if (ret != CL_SUCCESS) {
		size_t log_size;
		clGetProgramBuildInfo(program, deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char *log = (char*) malloc(log_size);
		clGetProgramBuildInfo(program, deviceID, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		std::ofstream outfile("KPassFilterCL.txt", std::ofstream::binary);
		outfile.write(log, log_size);
		outfile.close();
		free(log);
		env->ThrowError("KPassFilterCL: AviSynthCreate error (clBuildProgram)!");
	}

	// Creates and sets kernel arguments.
	kernel = clCreateKernel(program, "KPassFilter", NULL);
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_in[0]);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (filter >= 3) ? &mem_in[1] : &mem_in[0]);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &mem_out);
    if (ret != CL_SUCCESS) env->ThrowError("KPassFilterCL: AviSynthCreate error (clSetKernelArg)!");
}
#endif //__AVISYNTH_6_H__

//////////////////////////////////////////
// AviSynthGetFrame
#ifdef __AVISYNTH_6_H__
PVideoFrame __stdcall KPassFilterClass::GetFrame(int n, IScriptEnvironment* env) {
	// Variables.
	PVideoFrame src0 = child->GetFrame(n, env);
	PVideoFrame src1 = baby->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);
	cl_int ret = CL_SUCCESS;
	const size_t origin[3] = { 0, 0, 0 };
    const size_t region[3] = { idmn[0], idmn[1], 1 };
    const size_t global_work[2] = { idmn[0], idmn[1] };
	cl_command_queue command_queue = clCreateCommandQueue(context, deviceID, 0, NULL);

	// Copy chroma.
	if (!vi.IsY8()) {
		env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src0->GetReadPtr(PLANAR_U),
			src0->GetPitch(PLANAR_U), src0->GetRowSize(PLANAR_U), src0->GetHeight(PLANAR_U));
		env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src0->GetReadPtr(PLANAR_V),
			src0->GetPitch(PLANAR_V), src0->GetRowSize(PLANAR_V), src0->GetHeight(PLANAR_V));
	}
	
	// Processing.
    ret |= writeBuffer(src0);
	fftwf_execute(p);
	ret |= clEnqueueWriteImage(command_queue, mem_in[0], CL_TRUE, origin, region, 0, 0, out, 0, NULL, NULL);
	if (filter >= 3) {
        ret |= writeBuffer(src1);
		fftwf_execute(p);
		ret |= clEnqueueWriteImage(command_queue, mem_in[1], CL_TRUE, origin, region, 0, 0, out, 0, NULL, NULL);
	}
    ret |= clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_work, NULL, 0, NULL, NULL);
	ret |= clEnqueueReadImage(command_queue, mem_out, CL_TRUE, origin, region, 0, 0, out, 0, NULL, NULL);
	fftwf_execute(q);
    ret |= readBuffer(dst);
    if (ret != CL_SUCCESS) env->ThrowError("KPassFilterCL: AviSynthGetFrame error!");

	// Info.
	if (info) {
		uint8_t y = 0, *frm = dst->GetWritePtr(PLANAR_Y);
		int pitch = dst->GetPitch(PLANAR_Y);
		char buffer[2048], str[2048], str1[2048];
		DrawString(frm, pitch, 0, y++, "KPassFilter");
		DrawString(frm, pitch, 0, y++, " Version " VERSION);
		DrawString(frm, pitch, 0, y++, " Copyright(C) Khanattila");
        snprintf(buffer, 2048, " Global work size: %lux%lu", (unsigned long) idmn[0], (unsigned long) idmn[1]);
		DrawString(frm, pitch, 0, y++, buffer);
		DrawString(frm, pitch, 0, y++, "Platform info");
		ret |= clGetPlatformInfo(platformID, CL_PLATFORM_NAME, sizeof(char) * 2048, str, NULL);
		snprintf(buffer, 2048, " Name: %s", str);
		DrawString(frm, pitch, 0, y++, buffer);
		ret |= clGetPlatformInfo(platformID, CL_PLATFORM_VENDOR, sizeof(char) * 2048, str, NULL);
		snprintf(buffer, 2048, " Vendor: %s", str);
		DrawString(frm, pitch, 0, y++, buffer);
		ret |= clGetPlatformInfo(platformID, CL_PLATFORM_VERSION, sizeof(char) * 2048, str, NULL);
		snprintf(buffer, 2048, " Version: %s", str);
		DrawString(frm, pitch, 0, y++, buffer);
		DrawString(frm, pitch, 0, y++, "Device info");
		ret |= clGetDeviceInfo(deviceID, CL_DEVICE_NAME, sizeof(char) * 2048, str, NULL);
		snprintf(buffer, 2048, " Name: %s", str);
		DrawString(frm, pitch, 0, y++, buffer);
		ret |= clGetDeviceInfo(deviceID, CL_DEVICE_VENDOR, sizeof(char) * 2048, str, NULL);
		snprintf(buffer, 2048, " Vendor: %s", str);
		DrawString(frm, pitch, 0, y++, buffer);
		ret |= clGetDeviceInfo(deviceID, CL_DEVICE_VERSION, sizeof(char) * 2048, str, NULL);
		ret |= clGetDeviceInfo(deviceID, CL_DRIVER_VERSION, sizeof(char) * 2048, str1, NULL);
		snprintf(buffer, 2048, " Version: %s %s", str, str1);
		DrawString(frm, pitch, 0, y++, buffer);
		if (ret != CL_SUCCESS) env->ThrowError("KPassFilterCL: AviSynthInfo error!");
	}
	return dst;
}
#endif //__AVISYNTH_6_H__

//////////////////////////////////////////
// AviSynthFree
#ifdef __AVISYNTH_6_H__
KPassFilterClass::~KPassFilterClass() {
	clReleaseMemObject(mem_out);
	if (filter >= 3) clReleaseMemObject(mem_in[1]);
	clReleaseMemObject(mem_in[0]);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseContext(context);
	fftwf_destroy_plan(p);
	fftwf_destroy_plan(q);
	fftwf_free(in); 
	fftwf_free(out);
}
#endif //__AVISYNTH_6_H__

//////////////////////////////////////////
// AviSynthCreate
#ifdef __AVISYNTH_6_H__
AVSValue __cdecl AviSynthPluginCreateKLowPass(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new KPassFilterClass(args[0].AsClip(), args[0].AsClip(), 
		args[1].AsFloat(DFT_hcutoff), args[1].AsFloat(DFT_hcutoff), args[2].AsString(DFT_mode),
		args[3].AsString(DFT_ocl_device), args[4].AsBool(DFT_lsb), args[5].AsBool(DFT_info), LOW_PASS, env);
}
AVSValue __cdecl AviSynthPluginCreateKHighPass(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new KPassFilterClass(args[0].AsClip(), args[0].AsClip(), 
		args[1].AsFloat(DFT_lcutoff), args[1].AsFloat(DFT_lcutoff), args[2].AsString(DFT_mode),
		args[3].AsString(DFT_ocl_device), args[4].AsBool(DFT_lsb), args[5].AsBool(DFT_info), HIGH_PASS, env);
}
AVSValue __cdecl AviSynthPluginCreateKBandPass(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new KPassFilterClass(args[0].AsClip(), args[0].AsClip(), 
        args[1].AsFloat(DFT_lcutoff), args[2].AsFloat(DFT_hcutoff), args[3].AsString(DFT_mode),
		args[4].AsString(DFT_ocl_device), args[5].AsBool(DFT_lsb), args[6].AsBool(DFT_info), BAND_PASS, env);
}
AVSValue __cdecl AviSynthPluginCreateKMergeLow(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new KPassFilterClass(args[0].AsClip(), args[1].AsClip(), 
		args[2].AsFloat(DFT_hcutoff), args[2].AsFloat(DFT_hcutoff), args[3].AsString(DFT_mode),
		args[4].AsString(DFT_ocl_device), args[5].AsBool(DFT_lsb), args[6].AsBool(DFT_info), MERGE_LOW, env);
}
AVSValue __cdecl AviSynthPluginCreateKMergeHigh(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new KPassFilterClass(args[0].AsClip(), args[1].AsClip(), 
		args[2].AsFloat(DFT_lcutoff), args[2].AsFloat(DFT_lcutoff), args[3].AsString(DFT_mode),
		args[4].AsString(DFT_ocl_device), args[5].AsBool(DFT_lsb), args[6].AsBool(DFT_info), MERGE_HIGH, env);
}
AVSValue __cdecl AviSynthPluginCreateKMergeBand(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new KPassFilterClass(args[0].AsClip(), args[1].AsClip(), 
		args[2].AsFloat(DFT_lcutoff), args[3].AsFloat(DFT_hcutoff), args[4].AsString(DFT_mode),
		args[5].AsString(DFT_ocl_device), args[6].AsBool(DFT_lsb), args[7].AsBool(DFT_info), MERGE_BAND, env);
}
#endif //__AVISYNTH_6_H__

//////////////////////////////////////////
// AviSynthPluginInit
#ifdef __AVISYNTH_6_H__
const AVS_Linkage *AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(
	IScriptEnvironment* env, const AVS_Linkage* const vectors) {

	AVS_linkage = vectors;
	env->AddFunction("KLowPass", "c[cutoff]f[mode]s[device_type]s[lsb_inout]b[info]b", 
		AviSynthPluginCreateKLowPass, 0);
	env->AddFunction("KHighPass", "c[cutoff]f[mode]s[device_type]s[lsb_inout]b[info]b", 
		AviSynthPluginCreateKHighPass, 0);
	env->AddFunction("KBandPass", "c[lcutoff]f[hcuttoff]f[mode]s[device_type]s[lsb_inout]b[info]b", 
		AviSynthPluginCreateKBandPass, 0);
	env->AddFunction("KMergeLow", "cc[cutoff]f[mode]s[device_type]s[lsb_inout]b[info]b",
		AviSynthPluginCreateKLowPass, 0);
	env->AddFunction("KMergeHigh", "cc[cutoff]f[mode]s[device_type]s[lsb_inout]b[info]b",
		AviSynthPluginCreateKHighPass, 0);
	env->AddFunction("KMergeBand", "cc[lcutoff]f[hcuttoff]f[mode]s[device_type]s[lsb_inout]b[info]b",
		AviSynthPluginCreateKBandPass, 0);
	return "KPassFilter for AviSynth";
}
#endif //__AVISYNTH_6_H__
