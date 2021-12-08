//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>
#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;
typedef unsigned __int128 uint128_t;

//TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

/*
    Vector Addition Kernel Implementation using uint512_dt datatype
    Arguments:
        in1   (input)     --> Input Vector1
        in2   (input)     --> Input Vector2
        out   (output)    --> Output Vector
        size  (input)     --> Size of Vector in Integer
   */
extern "C"
{
    void wide_vadd(
        unsigned long long  *pm, // Read-Only Vector 1
        const char *pattern, // Read-Only Vector 2
        const char *text,
		int *k,
		const int *m,
		const int *n,
        int *out       // Output Result
    )
    {
#pragma HLS INTERFACE m_axi port = pm max_read_burst_length = 64  offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = pattern max_read_burst_length = 8  offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = text max_write_burst_length = 8 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = k max_write_burst_length = 64 offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = m max_write_burst_length = 64 offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = n max_write_burst_length = 64 offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = out max_write_burst_length = 64 offset = slave bundle = gmem3
#pragma HLS INTERFACE s_axilite port = pm bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = pattern bundle = control
#pragma HLS INTERFACE s_axilite port = text bundle = control
#pragma HLS INTERFACE s_axilite port =k bundle = control
#pragma HLS INTERFACE s_axilite port = m bundle = control
#pragma HLS INTERFACE s_axilite port = n bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    //create local buffers
   int num= ceil(m[0]/64) *4;
   int patt_len;
   int text_len;
   unsigned long long patternbitmasks[8];
   char patt[100];
   char tex[100];

#pragma HLS BIND_OP variable =patternbitmasks op=mul impl=DSP latency=2
#pragma HLS BIND_OP variable=patt op=add impl=DSP latency=2
#pragma HLS BIND_OP variable=tex op=add impl=DSP latency=2
#pragma HLS BIND_OP variable=patt_len op=add impl=DSP latency=2
#pragma HLS BIND_OP variable=text_len op=add impl=DSP latency=2

// BINDING TO BRAM
#pragma HLS BIND_STORAGE variable= patternbitmasks type=RAM_2P impl=BRAM latency=2
#pragma HLS BIND_STORAGE variable= patt type=RAM_2P impl=BRAM latency=2
#pragma HLS BIND_STORAGE variable= tex type=RAM_2P impl=BRAM latency=2
#pragma HLS BIND_STORAGE variable=patt_len type=RAM_2P impl=BRAM latency=2
#pragma HLS BIND_STORAGE variable =text_len type=RAM_2P impl=BRAM latency=2

  for (int i=0;i<num;i++)
	  patternbitmasks[i]=pm[i];
  patt_len=m[0];
  text_len=n[0];
  for (int i=0;i<patt_len;i++)
	  patt[i]=pattern[i];
  for (int i=0;i<text_len;i++)
  	  tex[i]=text[i];

  uint64_t oldR[num*(k+1)];
  int count =num*(k+1);
  for (int i=0;i<num;i++)
	  oldR[i]=ULLONG_MAX;

  uint64_t R[patt_len][count];
  for (int j=0;j<k+text_len;j++)
	  for (int i=min(j,k) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//computer R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  if(patt[n-j+1]=='A' or patt[n-j+1]=='a')
			  //code to get curr_pr
		  if(i==0)
		  {
			  //compute single R with range
		  }
		  else
		  {
			  //compute all 4 (in parallel by default) and And it
		  }


	  }

//single you will be storing R values , figure out a way to calculate
  //This is base code , we can change bindings and reduce memory storage if we are getting an overhead (Design1)
}}
