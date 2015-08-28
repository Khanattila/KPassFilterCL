## KPassFilterCL ## 
#### What ####
**KPassFilterCL** is a set of tools in the frequency domain.

#### Why ####
Paranoid encoders.

#### How ####
```
KLowPass(clip,  float cutoff, string mode, 
	string device_type, bool lsb_inout, bool info)

KHighPass(clip, float cutoff, string mode, 
	string device_type, bool lsb_inout, bool info)

KBandPass(clip, float lcutoff, float hcuttoff, string mode, 
	string device_type, bool lsb_inout, bool info)
	
KMergeLow(clip, clip, float cutoff, string mode, 
	string device_type, bool lsb_inout, bool info)
	
KMergeHigh(clip, clip, float cutoff, string mode, 
	string device_type, bool lsb_inout, bool info)
	
KMergeBand(clip, clip, float lcutoff, float hcuttoff, string mode,
	string device_type, bool lsb_inout, bool info)      
```
