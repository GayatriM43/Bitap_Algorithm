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
unsigned long long* shift(unsigned long long number0,unsigned long long number1)
{
unsigned long long* out;
int msb_rightpart=findmsb(number1,63);
out[1]=number1<<1;
out[0]=number0<<1+msb_rightpart;
return out;
}
int findmsb(unsigned long long number,int shiftsize)
{
unsigned long long mask=1<<shiftsize;
if(mask&number==0)
{
return 1;
}
else{
return 0;
}
}
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
  for (int i=0;i<count;i++)
	  oldR[i]=ULLONG_MAX;

 uint64_t R[110][25];
#pragma HLS BIND_OP variable=R op=mul impl=DSP latency=2
#pragma  HLS BIND_STORAGE variable=P type=RAM_2P impl=BRAM latency=2
  if (num==1)
  {
  for (int j=0;j<k+text_len;j++)
	  for (int i=min(j,k) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm;
		  if(tex[text_len-j+i]=='A' or tex[text_len-j+i]=='a')
		  {
			  currpm=patternbitmasks[0];
		  }
		  else if(tex[text_len-j+i]=='C' or tex[text_len-j+i]=='c')
		  {
		  	  currpm=patternbitmasks[1];
		  }
		  else if(tex[text_len-j+i]=='G' or tex[text_len-j+i]=='g')
		  {
			  currpm=patternbitmasks[2];
		  }
		  else if(tex[text_len-j+i]=='T' or tex[text_len-j+i]=='t')
		  {
		 	 currpm=patternbitmasks[3];
		  }
			  //code to get curr_pr
		  if(i==0)
		  {
			  //compute single R with range
			  if((text_len-j+i)==text_len-1)
			  {	  R[text_len-j+i][0]= oldR[0] << 1;
			      R[text_len+j+i][0]|=currpm;
			  }
			  else
			  {
				  R[text_len-j+i][0]= R[text_len-j+i+1][0] << 1;
				  R[text_len-j+i][0]|=currpm;
			  }

		  }
		  else
		  {
			  //compute all 4 (in parallel by default) and And it
			  uint64_t sub;
			  uint64_t ins;
			  uint64_t match;
			  uint64_t del;
			  if (j-i!=1)
			  {
				   del=R[text-j+i+1][i-1];
				   ins=R[text_len-j+i][i-1]<< 1; //cross dependency from prev level in graph
				   sub=R[text_len-j+i+1][i-1] << 1;
				   match=R[text_len-j+i+1][i] << 1;
				   match |=currpm;

			  }
			  else
			  {
				  del=oldR[0];
				  ins=R[text_len-j+i][i-1] << 1;
				  sub=oldR[0]<<1;
				  match=oldR[0]<<1;
				  match|=currpm;
			  }
			  }

		  }
  }
  else if (num==2)
  {
      for (int j=0;j<k+text_len;j++)
	  for (int i=min(j,k) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm;
		  if(tex[text_len-j+i]=='A' or tex[text_len-j+i]=='a')
		  {
			  currpm=patternbitmasks[0];
		  }
		  else if(tex[text_len-j+i]=='C' or tex[text_len-j+i]=='c')
		  {
		  	  currpm=patternbitmasks[1];
		  }
		  else if(tex[text_len-j+i]=='G' or tex[text_len-j+i]=='g')
		  {
			  currpm=patternbitmasks[2];
		  }
		  else if(tex[text_len-j+i]=='T' or tex[text_len-j+i]=='t')
		  {
		 	 currpm=patternbitmasks[3];
		  }
			  //code to get curr_pr
		  if(i==0)
		  {
			  //compute single R with range
			  if((text_len-j+i)==text_len-1)
			  {	  R[text_len-j+i][0]= shift(R[text_len-j+i][0],R[text_len-j+i][1])[0]
			      R[text_len+j+i][0]|=currpm[0];
                  R[text_len-j+i][1]= shift(R[text_len-j+i][0],R[text_len-j+i][1])[1]
			      R[text_len+j+i][1]|=currpm[1];
			  }
			  else
			  {
				  R[text_len-j+i][0]= shift(R[text_len-j+i+1][0],R[text_len-j+1+i][1])[0];
				  R[text_len-j+i][0]|=currpm[0];
                  R[text_len-j+i][1]= shift(R[text_len-j+i+1][0],R[text_len-j+1+i][1])[1];
				  R[text_len-j+i][1]|=currpm[1];
			  }

		  }
		  else
		  {
			  //compute all 4 (in parallel by default) and And it
			  uint64_t sub[2];
			  uint64_t ins[2];
			  uint64_t match[2];
			  uint64_t del[2];
			  if (j-i!=1)
			  {
				   del[0]=R[text-j+i+1][i-1];
                   del[1]=R[text-j+i+1][i]
				   ins=R[text_len-j+i][i-1]<< 1; //cross dependency from prev level in graph
				   sub=R[text_len-j+i+1][i-1] << 1;
				   match=R[text_len-j+i+1][i] << 1;
				   match |=currpm;

			  }
			  else
			  {
				  del=oldR[0];
				  ins=R[text_len-j+i][i-1] << 1;
				  sub=oldR[0]<<1;
				  match=oldR[0]<<1;
				  match|=currpm;
			  }
			  }

		  }
  }
  else
  {
	  std :: cout << "Not supported\n";
  }
