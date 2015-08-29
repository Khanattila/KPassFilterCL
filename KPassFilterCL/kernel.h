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

#ifndef __KERNEL_H__
#define __KERNEL_H__

//////////////////////////////////////////
// OpenCL
static const char* source_code =
"																												  \n" \
"__kernel																										  \n" \
"void KPassFilter(__read_only image2d_t U0, __read_only image2d_t U1, __write_only image2d_t U2) {				  \n" \
"																												  \n" \
"	int x = get_global_id(0);																				      \n" \
"	int y = get_global_id(1);																				      \n" \
"	sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;					      \n" \
"	float4 u0 = read_imagef(U0, smp, (int2) (x, y));														      \n" \
"	float4 u1 = (PFK_FILTER >= 3) ? read_imagef(U1, smp, (int2) (x, y)) : 0.0f; 							      \n" \
"																												  \n" \
"	float mgn_u0 = native_sqrt(u0.x * u0.x + u0.y * u0.y);											              \n" \
"	float phs_u0 = atan2(u0.y, u0.x);																		      \n" \
"	float mgn_u1 = (PFK_FILTER >= 3) ? native_sqrt(u1.x * u1.x + u1.y * u1.y) : 0.0f;						      \n" \
"	float phs_u1 = (PFK_FILTER >= 3) ? atan2(u1.y, u1.x) : 0.0f;											      \n" \
"																												  \n" \
"	float mgn = 0.0f, phs = 0.0f;																				  \n" \
"	if (PFK_MODE == 0) {																						  \n" \
"		if (PFK_FILTER == 0) { mgn = fmin(mgn_u0, PFK_LCUTOFF);													  \n" \
"							   phs = phs_u0; }																	  \n" \
"		if (PFK_FILTER == 1) { mgn = fmax(mgn_u0, PFK_HCUTOFF);													  \n" \
"							   phs = phs_u0; }																	  \n" \
"		if (PFK_FILTER == 2) { mgn = clamp(mgn_u0, PFK_LCUTOFF, PFK_HCUTOFF);									  \n" \
"							   phs = phs_u0; }																	  \n" \
"		if (PFK_FILTER == 3) { mgn = (mgn_u1 < PFK_LCUTOFF) ?  mgn_u1 : mgn_u0;									  \n" \
"							   phs = (mgn_u1 < PFK_LCUTOFF) ?  phs_u1 : phs_u0;	}								  \n" \
"		if (PFK_FILTER == 4) { mgn = (mgn_u1 > PFK_HCUTOFF) ?  mgn_u1 : mgn_u0;									  \n" \
"							   phs = (mgn_u1 > PFK_HCUTOFF) ?  phs_u1 : phs_u0; }								  \n" \
"		if (PFK_FILTER == 5) { mgn = (mgn_u1 > PFK_LCUTOFF && mgn_u1 < PFK_HCUTOFF) ?  mgn_u1 : mgn_u0;			  \n" \
"							   phs = (mgn_u1 > PFK_LCUTOFF && mgn_u1 < PFK_HCUTOFF) ?  phs_u1 : phs_u0; }		  \n" \
"	}																											  \n" \
"	if (PFK_MODE == 1) {																						  \n" \
"		if (PFK_FILTER == 0) { mgn = mgn_u0;  																	  \n" \
"							   phs = fmin(phs_u0, PFK_LCUTOFF); }												  \n" \
"		if (PFK_FILTER == 1) { mgn = mgn_u0;  																	  \n" \
"							   phs = fmax(phs_u0, PFK_HCUTOFF);	}												  \n" \
"		if (PFK_FILTER == 2) { mgn = mgn_u0; 																	  \n" \
"							   phs = clamp(phs_u0, PFK_LCUTOFF, PFK_HCUTOFF); }									  \n" \
"		if (PFK_FILTER == 3) { mgn = (phs_u1 < PFK_LCUTOFF) ?  mgn_u1 : mgn_u0; 								  \n" \
"							   phs = (phs_u1 < PFK_LCUTOFF) ?  phs_u1 : phs_u0; }								  \n" \
"		if (PFK_FILTER == 4) { mgn = (phs_u1 > PFK_HCUTOFF) ?  mgn_u1 : mgn_u0; 								  \n" \
"		                       phs = (phs_u1 > PFK_HCUTOFF) ?  phs_u1 : phs_u0; }								  \n" \
"		if (PFK_FILTER == 5) { mgn = (phs_u1 > PFK_LCUTOFF && phs_u1 < PFK_HCUTOFF) ?  mgn_u1 : mgn_u0; 	 	  \n" \
"							   phs = (phs_u1 > PFK_LCUTOFF && phs_u1 < PFK_HCUTOFF) ?  phs_u1 : phs_u0; }		  \n" \
"	}																											  \n" \
"																												  \n" \
"	const float rel = mgn * native_cos(phs);																	  \n" \
"	const float img = mgn * native_sin(phs);																	  \n" \
"	const float4 val = native_divide((float4) (rel, img, 0.0f, 1.0f), (float4) PFK_N);							  \n" \
"	write_imagef(U2, (int2) (x, y), val);																		  \n" \
"}																												     ";

#endif //__KERNEL_H__
