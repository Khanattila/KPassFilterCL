## KPassFilterCL ##

#### What ####
**KPassFilterCL** is a set of tools in the frequency domain.

#### Why ####
Paranoid encoders.

#### How ####
```
KLowPass(clip,  float cutoff, string mode ("magnitude"), 
	string device_type ("default"), bool lsb_inout (false), bool info(false))

KHighPass(clip, float cutoff, string mode ("magnitude"), 
	string device_type ("default"), bool lsb_inout (false), bool info(false))

KBandPass(clip, float lcutoff, float hcuttoff, string mode ("magnitude"), 
	string device_type ("default"), bool lsb_inout (false), bool info(false))
	
KMergeLow(clip, clip, float cutoff, string mode ("magnitude"), 
	string device_type ("default"), bool lsb_inout (false), bool info(false))
	
KMergeHigh(clip, clip, float cutoff, string mode ("magnitude"), 
	string device_type ("default"), bool lsb_inout (false), bool info(false))
	
KMergeBand(clip, clip, float lcutoff, float hcuttoff, string mode ("magnitude"),
	string device_type ("default"), bool lsb_inout (false), bool info(false))      
```
