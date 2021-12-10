
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <chrono>
#include <bits/stdc++.h>
// This extension file is required for stream APIs
//#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
//#include "xcl2.hpp"
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include "event_timer.hpp"
#include "xilinx_ocl.hpp"
#include <memory>
#include <iostream>

//# define KRNL_NAME wide_vadd
int min(int a, int b)
{
	if(a<b)
		return a ;
	return b;
}


unsigned long long *generatePatternBitmasksACGT(char* pattern, int m)
{
    int count = ceil(m/64.0);
        
    int len = 4*count; // A,C,G,T
    //std:: cout << "length is" <<len << "\n";
    unsigned long long *patternBitmasks = (unsigned long long *) malloc(len * sizeof(unsigned long long));
        
    unsigned long long max = ULLONG_MAX;

    // Initialize the pattern bitmasks
    for (int i=0; i < len; i++)
    {
        patternBitmasks[i] = max;
    }

    // Update the pattern bitmasks
    int index;
    for (int i=0; i < m; i++)
    {
        index = count - ((m-i-1) / 64) - 1;
        if ((pattern[i] == 'A') || (pattern[i] == 'a'))
        {
            patternBitmasks[0*count + index] &= ~(1ULL << ((m-i-1) % 64));
        }
        else if ((pattern[i] == 'C') || (pattern[i] == 'c'))
        {
            patternBitmasks[1*count + index] &= ~(1ULL << ((m-i-1) % 64));
        }
        else if ((pattern[i] == 'G') || (pattern[i] == 'g'))
        {
            patternBitmasks[2*count + index] &= ~(1ULL << ((m-i-1) % 64));
        }
        else if ((pattern[i] == 'T') || (pattern[i] == 't'))
        {
            patternBitmasks[3*count + index] &= ~(1ULL << ((m-i-1) % 64));
        }
    }
   
    return patternBitmasks;
}


int genasmDC(char *text, char *pattern, int k)
{
    //EventTimer et;
    int m = strlen(pattern);
    int n = strlen(text);
    
    unsigned long long max = ULLONG_MAX;

    int count = ceil(m/64.0);
    //Initialise pattern bitmasks
    //std::cout << "Generating Pattern bit masks\n";
    unsigned long long *patternBitmasks = generatePatternBitmasksACGT(pattern, m);
    //send pattern bit masks to kernel and bind them 
    //std::cout << "entered DC\n";
    // Initialize the bit arrays R
    //std :: cout  << "k is" << k << "count" <<count << "\n";
    int len1 = (k+1) * count;
    //std::cout << "length in DC before R: "<< len1 << "\n";
    unsigned long long R[len1];
   //std::cout << "length in DC : "<< len1 << "\n";
    for (int i=0; i < len1; i++)
    {
       R[i] = max;  
    }

    for (int x=1; x < (k+1); x++)
    {
        if ((x % 64) == 0)
        {
            int ind = count - (x/64);
            for (int y = (count-1); y >= ind; y--)
            {
                R[x*count+y] = 0ULL;
            }
        }
        else
        {
            int ind = count - 1 - (x/64);
            for (int y = (count-1); y > ind; y--)
            {
                R[x*count+y] = 0ULL;
            }
            R[x*count + ind] = max << (x%64);
        }
    }
        
    unsigned long long oldR[len1];

    unsigned long long substitution[count], insertion[count], match[count];
    unsigned long long deletion[count], curBitmask[count];

    int rem = m % 64;

    unsigned long long max1;
    if (rem == 0)
    {
        max1 = 1ULL << 63;
    }
    else
    {
        max1 = 1ULL << (rem-1);
    }
    int minError = INT_MAX;
    for (int i=n-1; i >= 0; i--)
    {
        char c = text[i];
        
        if ((c == 'A') || (c == 'a') || (c == 'C') || (c == 'c') || (c == 'G') || (c == 'g') || (c == 'T') || (c == 't'))
        {
            // copy the content of R to oldR
            for (int itR=0; itR<len1; itR++)
            {
                oldR[itR] = R[itR];
            }

            if ((c == 'A') || (c == 'a'))
            {
                for (int a=0; a<count; a++)
                {
                    curBitmask[a] = patternBitmasks[0*count + a];
                }
            }
            else if ((c == 'C') || (c == 'c'))
            {
                for (int a=0; a<count; a++)
                {
                    curBitmask[a] = patternBitmasks[1*count + a];
                }
            }
            else if ((c == 'G') || (c == 'g'))
            {
                for (int a=0; a<count; a++)
                {
                    curBitmask[a] = patternBitmasks[2*count + a];
                }
            }
            else if ((c == 'T') || (c == 't'))
            {
                for (int a=0; a<count; a++)
                {
                    curBitmask[a] = patternBitmasks[3*count + a];
                }
            }

            // update R[0] by first shifting oldR[0] and then ORing with pattern bitmask
            R[0] = oldR[0] << 1;
            for (int a=1; a<count; a++)
            {
                R[a-1] |= (oldR[a] >> 63);
                R[a] = oldR[a] << 1;
            }
            for (int a=0; a<count; a++)
            {
                R[a] |= curBitmask[a];
            }
           
            for (int d=1; d <= k; d++)
            {
                int index = (d-1) * count;
                
                for (int a=0; a<count; a++)
                {
                    deletion[a] = oldR[index + a];
                }
                
                //deletion = &oldR[index];

                substitution[0] = deletion[0] << 1;
                for (int a=1; a<count; a++)
                {
                    substitution[a-1] |= (deletion[a] >> 63);
                    substitution[a] = deletion[a] << 1;
                }

                insertion[0] = R[index] << 1;
                for (int a=1; a<count; a++)
                {
                    insertion[a-1] |= (R[index+a] >> 63);
                    insertion[a] = R[index+a] << 1;
                }

                index += count;
                
                match[0] = oldR[index] << 1;
                for (int a=1; a<count; a++)
                {
                    match[a-1] |= (oldR[index+a] >> 63);
                    match[a] = oldR[index+a] << 1;
                }
                
                for (int a=0; a<count; a++)
                {
                    match[a] |= curBitmask[a];
                }
                
                for (int a=0; a<count; a++)
                {
                    R[index+a] = deletion[a] & substitution[a] & insertion[a] & match[a];
                }
            }
        }
    


    unsigned long long mask = max1;
    
    for (int t=0; t <= k; t++)
    {
        if ((R[t*count] & mask) == 0)
        {
            minError = min(minError,t);
        }
    }}

    //free(patternBitmasks);
    //std::cout << "DC return value" << minError;
    //fprintf(stdout, "%d\n", minError);

    return minError;
}


int genasm_filter(char *text, char *pattern, int k)
{
	//std::cout << "entered filter"<<"\n";
    int minError = genasmDC(text, pattern, k);
    return minError;
}

int main(int argc, char * argv[])
{
    //genasm_filter("AAAGAAAAGAATTTTCAACCCAGAATTTCATATCCAGCCAAACTACGCTTCATAAGTGAAGGAGAAATAAAATCCTTTACAGACAAGCAAATGCTGAGAG", "AAAGAAAAGAATTTTCAACCCAGAATTTCATATCCAGCCAAACAAAGCTTCATAAGTGAAGGAGAAATAAATCCTTTACAGAGAAGCAAATGCTGAGAGA", 6);
    char *binname= argv[1];	
    EventTimer et; //event timer we will use for monitoring
    
    std::cout << "-----Accelerating bitap algorithm-----------\n";
    et.add("OpenCL Initialization");
    std::cout << "Loading" << binname << "to program\n";
    // This application will use the first Xilinx device found in the system
    swm::XilinxOcl xocl;
    xocl.initialize(binname);

    cl::CommandQueue q = xocl.get_command_queue();
    cl::Kernel krnl = xocl.get_kernel("wide_vadd");
    et.finish();
    
    int m = strlen(argv[2]);
    int n = strlen(argv[3]);
    std::cout << "length of pattern is "<< m << "length of text is "<< n << "\n";
    unsigned long long max = ULLONG_MAX;

    int count = ceil(m/64.0);
    int len = 4*count;

  
    std::cout << "started mapping buffers\n";
    //for (int i=0;i<=len(patternBitmasks);i++)
    //std::cout << patternBitmasks[1] << "\n";

    //  std::cout << genasmDC(pattern,text,2);
    
    //send pattern bit masks to kernel and bind them 
    et.add("Map pm buffer");
    cl::Buffer buffer_pm(xocl.get_context(),
                        static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                        len*sizeof(unsigned long long),
                        NULL,
                        NULL);
    cl::Buffer buffer_pattern(xocl.get_context(),
                              static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                              m*sizeof(char),
                              NULL,
                              NULL
                              );
    cl::Buffer buffer_text(xocl.get_context(),
                              static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                              n*sizeof(char),
                              NULL,
                              NULL
                              );
    cl::Buffer buffer_k(xocl.get_context(),
                                 static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                                 sizeof(int),
                                 NULL,
                                 NULL
                                 );
    cl::Buffer buffer_m(xocl.get_context(),
                                 static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                                 sizeof(int),
                                 NULL,
                                 NULL
                                 );
    cl::Buffer buffer_n(xocl.get_context(),
                                 static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                                 sizeof(int),
                                 NULL,
                                 NULL
                                 );
    cl::Buffer buffer_out(xocl.get_context(),
                            static_cast<cl_mem_flags>(CL_MEM_READ_WRITE),                   
                             sizeof(int),
                            NULL,
                            NULL
                            );

    krnl.setArg(0, buffer_pm);
    krnl.setArg(1,buffer_pattern);
    krnl.setArg(2,buffer_text);
    krnl.setArg(3,buffer_k);
    krnl.setArg(4,buffer_m);
    krnl.setArg(5,buffer_n);
    krnl.setArg(6,buffer_out);
    unsigned long long *patternBitmasks = (unsigned long long *)q.enqueueMapBuffer(buffer_pm,
                                                     CL_TRUE,
                                                     CL_MAP_WRITE_INVALIDATE_REGION,
                                                     0,
                                                     len * sizeof(unsigned long long));
    char *pattern  =  (char *)q.enqueueMapBuffer(buffer_pattern,
                                                     CL_TRUE,
                                                     CL_MAP_WRITE_INVALIDATE_REGION,
                                                     0,
                                                     m * sizeof(char));
    char *text = (char*)q.enqueueMapBuffer(buffer_text,
                                                     CL_TRUE,
                                                     CL_MAP_WRITE_INVALIDATE_REGION,
                                                     0,
                                                     n*sizeof(char));
    int* k = (int*)q.enqueueMapBuffer(buffer_k,
                                                     CL_TRUE,
                                                     CL_MAP_WRITE_INVALIDATE_REGION,
                                                     0,
                                                     sizeof(int));
    int* m1 = (int*)q.enqueueMapBuffer(buffer_m,
                                                        CL_TRUE,
                                                        CL_MAP_WRITE_INVALIDATE_REGION,
                                                        0,
                                                        sizeof(int));
    int* n1= (int*)q.enqueueMapBuffer(buffer_n,
                                                        CL_TRUE,
                                                        CL_MAP_WRITE_INVALIDATE_REGION,
                                                        0,
                                                        sizeof(int));
    et.finish();
   //Initialise pattern bitmasks
    std::cout << "Generating Pattern bit masks\n";
    et.add("generating pattern bitmasks");
    unsigned long long *patternmask = generatePatternBitmasksACGT(argv[2], m);
//    patternBitmasks[0]=patternBitmask[0];
//    patternBitmasks[1]=patternBitmask[1];
//    patternBitmasks[2]=patternBitmask[2];
//    patternBitmasks[3]=patternBitmask[3];
    for (int i=0 ;i <len;i++)
    {
    	patternBitmasks[i]=patternmask[i];
    }
    //std::cout << patternBitmasks[0]<<"\n";
    et.finish();
    char *pat=argv[2];
    char *tex=argv[3];
    //std::cout << "text in host"<< *tex << "\n";
    for (int i=0;i<m;i++)
    	pattern[i]=pat[i];

    for (int j=0;j<n;j++)
    {    text[j]= tex[j];
     }
    k[0]=std::stoi(argv[4]);
    m1[0]=m;
    n1[0]=n;
    //std :: cout << "offset" << k[0] ;
	int out_put=0;
	et.add("cpu time");
    std::cout << "CPU result"  << "\n";
    out_put = genasm_filter(text,pat,k[0]);
    std::  cout << out_put << "\n";
    et.finish();
     // Send the buffers down to the  card
    std::cout<< "Sending buffers\n";
    et.add("Memory object migration enqueue");
    cl::Event event_sp;
    q.enqueueMigrateMemObjects({buffer_pm, buffer_pattern, buffer_text,buffer_k, buffer_m, buffer_n}, 0, NULL, &event_sp);
    clWaitForEvents(1, (const cl_event *)&event_sp);

    et.add("OCL Enqueue task");
    std::cout <<"starting kernel\n";
    q.enqueueTask(krnl, NULL, &event_sp);
    et.add("Wait for kernel to complete");
    clWaitForEvents(1, (const cl_event *)&event_sp);

    // Migrate memory back from device
    et.add("Read back computation results");
    int *c = (int *)q.enqueueMapBuffer(buffer_out,
                                                 CL_TRUE,
                                                 CL_MAP_READ,
                                                 0,
                                                sizeof(int));

    if (c[0]!=out_put)
    std::cout << "cpu result" << out_put << " donot match with fpga result" << c[0] << "\n";
    else if (c[0]==-2)
	std::cout << "length exceeded\n";
    else
    std:: cout << "success, edit distance is" << c[0];
    et.finish();


                                  
    
    return 0;
}
