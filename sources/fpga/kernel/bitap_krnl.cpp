//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>
#include <limits.h>
#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;
typedef unsigned __int128 uint128_t;

//TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

/*
    Bitap kernel implementation

        pm (input)          ---> patternbitmasks of pattern
        pattern   (input)     --> query string
        text   (input)     --> text string
        k   (input)     --> offset value
        m  (input)       -- > pattern length
        n (input)       ---> text length
        out   (output)    --> Output integer, edit distance

   */
  //for shifting two numbers , ie first bit of ind1 should be last bit of ind0
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

unsigned long long* shift(unsigned long long number0,unsigned long long number1)
{
unsigned long long* out;
int msb_rightpart=findmsb(number1,63);
out[1]=number1<<1;
out[0]=number0<<1+msb_rightpart;
return out;
}

int min(int a, int b)
{
	if(a<b)
		return a ;
	return b;
}


extern "C"
{
    void wide_vadd(
        unsigned long long  *pm,
        const char *pattern,
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

   patt_len=m[0];
   text_len=n[0];
   int num= ceil(m[0]/(64*1.0));
     num=num*4;
     int val = ceil (patt_len/(64.0));
     //std::cout << val << "is val\n";
  for (int i=0;i<num;i++)
	  patternbitmasks[i]=pm[i];

  int offset=k[0];
  int count =num*(offset+1);
  uint64_t oldR[2];
  oldR[0]=ULLONG_MAX;
  oldR[1]=ULLONG_MAX;
for (int i=0;i<text_len;i++)
	tex[i]=text[i];
for (int i=0;i<patt_len;i++)
	patt[i]=pattern[i];
 uint64_t R[110][25];
#pragma HLS BIND_OP variable=R op=mul impl=DSP latency=2
#pragma  HLS BIND_STORAGE variable=R type=RAM_2P impl=BRAM latency=2
//bind needs specific value hence specified upper bounds.
  if (val==1)
  {
  for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset);i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm;
		  int iter = text_len-j+i;
		  if (iter<0 or i<0)
			  continue;
		  else
		  {
		  if(tex[iter]=='A' or tex[iter]=='a')
		  {
			  currpm=patternbitmasks[0];
		  }
		  else if(tex[iter]=='C' or tex[iter]=='c')
		  {
		  	  currpm=patternbitmasks[1];
		  }
		  else if(tex[iter]=='G' or tex[iter]=='g')
		  {
			  currpm=patternbitmasks[2];
		  }
		  else if(tex[iter]=='T' or tex[iter]=='t')
		  {
		 	 currpm=patternbitmasks[3];
		  }
			  //code to get curr_pr
//		  std::cout << "text pos "<< iter<<"for j at"<< j << "for i at "<< i<<"\n";
//		  std::cout << "currpm is " << currpm << "\n";
//		  std::cout << "text" << tex[iter]<<" -----------------at --------------"<< iter <<"\n";
//		  //std::cout << "pattern bit mask"<< patternbitmasks[0]<<"\n";
//		  std::cout << "the tuple is("<< iter << ","<< i <<")\n";
		  if(i==0)
		  {
			  //compute single R with range
			  if(iter==text_len-1)
			  {	  R[iter][0]= oldR[0]<<1;
			  //std::cout << "after shift" <<R[iter][0] << "\n";
			      R[iter][0]|=currpm;
			  }
			  else
			  {
				 // std::cout << "before shift"<<R[iter+1][0]<<"\n";
				  R[iter][0]= R[iter+1][0]<<1;
				 // std::cout << "after shift" <<R[iter][0] << "\n";
				  R[iter][0]|=currpm;

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
				   del= R[iter+1][i-1];
				   ins=R[iter][i-1]<< 1; //cross dependency from prev level in graph
				   sub=R[iter+1][i-1] << 1;
				   match=R[iter+1][i] << 1;
				   match |=currpm;
                   R[iter][i]=del & ins & sub & match;

			  }
			  else
			  {
				  del=oldR[0];
				  ins=R[iter][i-1] << 1;
				  sub=oldR[0]<<1;
				  match=oldR[0]<<1;
				  match|=currpm;
                  R[iter][i]=del & ins & sub & match;
			  }
			  }

		  }}
  }
  else if (val==2)
  {
      for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm[2];
          int ind= i*2;
          std:: cout <<"text char"<< tex[text_len-j+i] << "\n";
		  if(tex[text_len-j+i]=='A' or tex[text_len-j+i]=='a')
		  {
			  currpm[0]=patternbitmasks[0];
              currpm[1]=patternbitmasks[1];
		  }
		  else if(tex[text_len-j+i]=='C' or tex[text_len-j+i]=='c')
		  {
		  	  currpm[0]=patternbitmasks[2];
              currpm[1]=patternbitmasks[3];
		  }
		  else if(tex[text_len-j+i]=='G' or tex[text_len-j+i]=='g')
		  {
			  currpm[0]=patternbitmasks[4];
              currpm[1]=patternbitmasks[5];
		  }
		  else if(tex[text_len-j+i]=='T' or tex[text_len-j+i]=='t')
		  {
		 	 currpm[0]=patternbitmasks[6];
             currpm[1]=patternbitmasks[7];
		  }
			  //code to get curr_pr
		  if(text_len-j+i<0 or i<0)
			  continue;
		  else
		  {
		  if(i==0)
		  {
			  //compute single R with range
			  if((text_len-j+i)==text_len-1)
			  {	  R[text_len-j+i][0]= shift(oldR[0],oldR[1])[0];
			      R[text_len+j+i][0]|=currpm[0];
                  R[text_len-j+i][1]= shift(oldR[0],oldR[1])[1];
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
				   del[0]=R[text_len-j+i+1][ind-2];
                   del[1]=R[text_len-j+i+1][ind-1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[0];
                   sub[1]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[1];
				   match[0]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  else
			  {
                   del[0]=oldR[0];
                   del[1]=oldR[1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(oldR[0],oldR[1])[0];
                   sub[1]=shift(oldR[0],oldR[1])[1];
				   match[0]=shift(oldR[0],oldR[1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(oldR[0],oldR[1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  }

		  }}
  }
  else
  {
	  std :: cout << "Not supported since value of num is " << val << "\n";
  }


  std::  cout << "Edit distance calculation started\n";
  //using old method
  int minerror=0;
//  for (int j=text_len-1;j>=0;j--)
//	  for (int i=0;i<=offset;i++)
//	  {
//		  if (msb(R[j][i])==0)
//		  {  minerror=i;
//		     break;
//		  }
//
//	  }
 out[0]=minerror;
  }}
//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>
#include <limits.h>
#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;
typedef unsigned __int128 uint128_t;

//TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

/*
    Bitap kernel implementation

        pm (input)          ---> patternbitmasks of pattern
        pattern   (input)     --> query string
        text   (input)     --> text string
        k   (input)     --> offset value
        m  (input)       -- > pattern length
        n (input)       ---> text length
        out   (output)    --> Output integer, edit distance

   */
  //for shifting two numbers , ie first bit of ind1 should be last bit of ind0
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

unsigned long long* shift(unsigned long long number0,unsigned long long number1)
{
unsigned long long* out;
int msb_rightpart=findmsb(number1,63);
out[1]=number1<<1;
out[0]=number0<<1+msb_rightpart;
return out;
}

int min(int a, int b)
{
	if(a<b)
		return a ;
	return b;
}


extern "C"
{
    void wide_vadd(
        unsigned long long  *pm,
        const char *pattern,
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

   patt_len=m[0];
   text_len=n[0];
   int num= ceil(m[0]/(64*1.0));
     num=num*4;
     int val = ceil (patt_len/(64.0));
     //std::cout << val << "is val\n";
  for (int i=0;i<num;i++)
	  patternbitmasks[i]=pm[i];

  int offset=k[0];
  int count =num*(offset+1);
  uint64_t oldR[2];
  oldR[0]=ULLONG_MAX;
  oldR[1]=ULLONG_MAX;
for (int i=0;i<text_len;i++)
	tex[i]=text[i];
for (int i=0;i<patt_len;i++)
	patt[i]=pattern[i];
 uint64_t R[110][25];
#pragma HLS BIND_OP variable=R op=mul impl=DSP latency=2
#pragma  HLS BIND_STORAGE variable=R type=RAM_2P impl=BRAM latency=2
//bind needs specific value hence specified upper bounds.
  if (val==1)
  {
  for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset);i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm;
		  int iter = text_len-j+i;
		  if (iter<0 or i<0)
			  continue;
		  else
		  {
		  if(tex[iter]=='A' or tex[iter]=='a')
		  {
			  currpm=patternbitmasks[0];
		  }
		  else if(tex[iter]=='C' or tex[iter]=='c')
		  {
		  	  currpm=patternbitmasks[1];
		  }
		  else if(tex[iter]=='G' or tex[iter]=='g')
		  {
			  currpm=patternbitmasks[2];
		  }
		  else if(tex[iter]=='T' or tex[iter]=='t')
		  {
		 	 currpm=patternbitmasks[3];
		  }
			  //code to get curr_pr
//		  std::cout << "text pos "<< iter<<"for j at"<< j << "for i at "<< i<<"\n";
//		  std::cout << "currpm is " << currpm << "\n";
//		  std::cout << "text" << tex[iter]<<" -----------------at --------------"<< iter <<"\n";
//		  //std::cout << "pattern bit mask"<< patternbitmasks[0]<<"\n";
//		  std::cout << "the tuple is("<< iter << ","<< i <<")\n";
		  if(i==0)
		  {
			  //compute single R with range
			  if(iter==text_len-1)
			  {	  R[iter][0]= oldR[0]<<1;
			  //std::cout << "after shift" <<R[iter][0] << "\n";
			      R[iter][0]|=currpm;
			  }
			  else
			  {
				 // std::cout << "before shift"<<R[iter+1][0]<<"\n";
				  R[iter][0]= R[iter+1][0]<<1;
				 // std::cout << "after shift" <<R[iter][0] << "\n";
				  R[iter][0]|=currpm;

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
				   del= R[iter+1][i-1];
				   ins=R[iter][i-1]<< 1; //cross dependency from prev level in graph
				   sub=R[iter+1][i-1] << 1;
				   match=R[iter+1][i] << 1;
				   match |=currpm;
                   R[iter][i]=del & ins & sub & match;

			  }
			  else
			  {
				  del=oldR[0];
				  ins=R[iter][i-1] << 1;
				  sub=oldR[0]<<1;
				  match=oldR[0]<<1;
				  match|=currpm;
                  R[iter][i]=del & ins & sub & match;
			  }
			  }

		  }}
  }
  else if (val==2)
  {
      for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm[2];
          int ind= i*2;
          std:: cout <<"text char"<< tex[text_len-j+i] << "\n";
		  if(tex[text_len-j+i]=='A' or tex[text_len-j+i]=='a')
		  {
			  currpm[0]=patternbitmasks[0];
              currpm[1]=patternbitmasks[1];
		  }
		  else if(tex[text_len-j+i]=='C' or tex[text_len-j+i]=='c')
		  {
		  	  currpm[0]=patternbitmasks[2];
              currpm[1]=patternbitmasks[3];
		  }
		  else if(tex[text_len-j+i]=='G' or tex[text_len-j+i]=='g')
		  {
			  currpm[0]=patternbitmasks[4];
              currpm[1]=patternbitmasks[5];
		  }
		  else if(tex[text_len-j+i]=='T' or tex[text_len-j+i]=='t')
		  {
		 	 currpm[0]=patternbitmasks[6];
             currpm[1]=patternbitmasks[7];
		  }
			  //code to get curr_pr
		  if(text_len-j+i<0 or i<0)
			  continue;
		  else
		  {
		  if(i==0)
		  {
			  //compute single R with range
			  if((text_len-j+i)==text_len-1)
			  {	  R[text_len-j+i][0]= shift(oldR[0],oldR[1])[0];
			      R[text_len+j+i][0]|=currpm[0];
                  R[text_len-j+i][1]= shift(oldR[0],oldR[1])[1];
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
				   del[0]=R[text_len-j+i+1][ind-2];
                   del[1]=R[text_len-j+i+1][ind-1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[0];
                   sub[1]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[1];
				   match[0]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  else
			  {
                   del[0]=oldR[0];
                   del[1]=oldR[1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(oldR[0],oldR[1])[0];
                   sub[1]=shift(oldR[0],oldR[1])[1];
				   match[0]=shift(oldR[0],oldR[1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(oldR[0],oldR[1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  }

		  }}
  }
  else
  {
	  std :: cout << "Not supported since value of num is " << val << "\n";
  }


  std::  cout << "Edit distance calculation started\n";
  //using old method
  int minerror=0;
//  for (int j=text_len-1;j>=0;j--)
//	  for (int i=0;i<=offset;i++)
//	  {
//		  if (msb(R[j][i])==0)
//		  {  minerror=i;
//		     break;
//		  }
//
//	  }
 out[0]=minerror;
  }}
//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>
#include <limits.h>
#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;
typedef unsigned __int128 uint128_t;

//TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

/*
    Bitap kernel implementation

        pm (input)          ---> patternbitmasks of pattern
        pattern   (input)     --> query string
        text   (input)     --> text string
        k   (input)     --> offset value
        m  (input)       -- > pattern length
        n (input)       ---> text length
        out   (output)    --> Output integer, edit distance

   */
  //for shifting two numbers , ie first bit of ind1 should be last bit of ind0
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

unsigned long long* shift(unsigned long long number0,unsigned long long number1)
{
unsigned long long* out;
int msb_rightpart=findmsb(number1,63);
out[1]=number1<<1;
out[0]=number0<<1+msb_rightpart;
return out;
}

int min(int a, int b)
{
	if(a<b)
		return a ;
	return b;
}


extern "C"
{
    void wide_vadd(
        unsigned long long  *pm,
        const char *pattern,
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

   patt_len=m[0];
   text_len=n[0];
   int num= ceil(m[0]/(64*1.0));
     num=num*4;
     int val = ceil (patt_len/(64.0));
     //std::cout << val << "is val\n";
  for (int i=0;i<num;i++)
	  patternbitmasks[i]=pm[i];

  int offset=k[0];
  int count =num*(offset+1);
  uint64_t oldR[2];
  oldR[0]=ULLONG_MAX;
  oldR[1]=ULLONG_MAX;
for (int i=0;i<text_len;i++)
	tex[i]=text[i];
for (int i=0;i<patt_len;i++)
	patt[i]=pattern[i];
 uint64_t R[110][25];
#pragma HLS BIND_OP variable=R op=mul impl=DSP latency=2
#pragma  HLS BIND_STORAGE variable=R type=RAM_2P impl=BRAM latency=2
//bind needs specific value hence specified upper bounds.
  if (val==1)
  {
  for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset);i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm;
		  int iter = text_len-j+i;
		  if (iter<0 or i<0)
			  continue;
		  else
		  {
		  if(tex[iter]=='A' or tex[iter]=='a')
		  {
			  currpm=patternbitmasks[0];
		  }
		  else if(tex[iter]=='C' or tex[iter]=='c')
		  {
		  	  currpm=patternbitmasks[1];
		  }
		  else if(tex[iter]=='G' or tex[iter]=='g')
		  {
			  currpm=patternbitmasks[2];
		  }
		  else if(tex[iter]=='T' or tex[iter]=='t')
		  {
		 	 currpm=patternbitmasks[3];
		  }
			  //code to get curr_pr
//		  std::cout << "text pos "<< iter<<"for j at"<< j << "for i at "<< i<<"\n";
//		  std::cout << "currpm is " << currpm << "\n";
//		  std::cout << "text" << tex[iter]<<" -----------------at --------------"<< iter <<"\n";
//		  //std::cout << "pattern bit mask"<< patternbitmasks[0]<<"\n";
//		  std::cout << "the tuple is("<< iter << ","<< i <<")\n";
		  if(i==0)
		  {
			  //compute single R with range
			  if(iter==text_len-1)
			  {	  R[iter][0]= oldR[0]<<1;
			  //std::cout << "after shift" <<R[iter][0] << "\n";
			      R[iter][0]|=currpm;
			  }
			  else
			  {
				 // std::cout << "before shift"<<R[iter+1][0]<<"\n";
				  R[iter][0]= R[iter+1][0]<<1;
				 // std::cout << "after shift" <<R[iter][0] << "\n";
				  R[iter][0]|=currpm;

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
				   del= R[iter+1][i-1];
				   ins=R[iter][i-1]<< 1; //cross dependency from prev level in graph
				   sub=R[iter+1][i-1] << 1;
				   match=R[iter+1][i] << 1;
				   match |=currpm;
                   R[iter][i]=del & ins & sub & match;

			  }
			  else
			  {
				  del=oldR[0];
				  ins=R[iter][i-1] << 1;
				  sub=oldR[0]<<1;
				  match=oldR[0]<<1;
				  match|=currpm;
                  R[iter][i]=del & ins & sub & match;
			  }
			  }

		  }}
  }
  else if (val==2)
  {
      for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm[2];
          int ind= i*2;
          std:: cout <<"text char"<< tex[text_len-j+i] << "\n";
		  if(tex[text_len-j+i]=='A' or tex[text_len-j+i]=='a')
		  {
			  currpm[0]=patternbitmasks[0];
              currpm[1]=patternbitmasks[1];
		  }
		  else if(tex[text_len-j+i]=='C' or tex[text_len-j+i]=='c')
		  {
		  	  currpm[0]=patternbitmasks[2];
              currpm[1]=patternbitmasks[3];
		  }
		  else if(tex[text_len-j+i]=='G' or tex[text_len-j+i]=='g')
		  {
			  currpm[0]=patternbitmasks[4];
              currpm[1]=patternbitmasks[5];
		  }
		  else if(tex[text_len-j+i]=='T' or tex[text_len-j+i]=='t')
		  {
		 	 currpm[0]=patternbitmasks[6];
             currpm[1]=patternbitmasks[7];
		  }
			  //code to get curr_pr
		  if(text_len-j+i<0 or i<0)
			  continue;
		  else
		  {
		  if(i==0)
		  {
			  //compute single R with range
			  if((text_len-j+i)==text_len-1)
			  {	  R[text_len-j+i][0]= shift(oldR[0],oldR[1])[0];
			      R[text_len+j+i][0]|=currpm[0];
                  R[text_len-j+i][1]= shift(oldR[0],oldR[1])[1];
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
				   del[0]=R[text_len-j+i+1][ind-2];
                   del[1]=R[text_len-j+i+1][ind-1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[0];
                   sub[1]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[1];
				   match[0]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  else
			  {
                   del[0]=oldR[0];
                   del[1]=oldR[1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(oldR[0],oldR[1])[0];
                   sub[1]=shift(oldR[0],oldR[1])[1];
				   match[0]=shift(oldR[0],oldR[1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(oldR[0],oldR[1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  }

		  }}
  }
  else
  {
	  std :: cout << "Not supported since value of num is " << val << "\n";
  }


  std::  cout << "Edit distance calculation started\n";
  //using old method
  int minerror=0;
//  for (int j=text_len-1;j>=0;j--)
//	  for (int i=0;i<=offset;i++)
//	  {
//		  if (msb(R[j][i])==0)
//		  {  minerror=i;
//		     break;
//		  }
//
//	  }
 out[0]=minerror;
  }}
//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>
#include <limits.h>
#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;
typedef unsigned __int128 uint128_t;

//TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

/*
    Bitap kernel implementation

        pm (input)          ---> patternbitmasks of pattern
        pattern   (input)     --> query string
        text   (input)     --> text string
        k   (input)     --> offset value
        m  (input)       -- > pattern length
        n (input)       ---> text length
        out   (output)    --> Output integer, edit distance

   */
  //for shifting two numbers , ie first bit of ind1 should be last bit of ind0
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

unsigned long long* shift(unsigned long long number0,unsigned long long number1)
{
unsigned long long* out;
int msb_rightpart=findmsb(number1,63);
out[1]=number1<<1;
out[0]=number0<<1+msb_rightpart;
return out;
}

int min(int a, int b)
{
	if(a<b)
		return a ;
	return b;
}


extern "C"
{
    void wide_vadd(
        unsigned long long  *pm,
        const char *pattern,
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

   patt_len=m[0];
   text_len=n[0];
   int num= ceil(m[0]/(64*1.0));
     num=num*4;
     int val = ceil (patt_len/(64.0));
     //std::cout << val << "is val\n";
  for (int i=0;i<num;i++)
	  patternbitmasks[i]=pm[i];

  int offset=k[0];
  int count =num*(offset+1);
  uint64_t oldR[2];
  oldR[0]=ULLONG_MAX;
  oldR[1]=ULLONG_MAX;
for (int i=0;i<text_len;i++)
	tex[i]=text[i];
for (int i=0;i<patt_len;i++)
	patt[i]=pattern[i];
 uint64_t R[110][25];
#pragma HLS BIND_OP variable=R op=mul impl=DSP latency=2
#pragma  HLS BIND_STORAGE variable=R type=RAM_2P impl=BRAM latency=2
//bind needs specific value hence specified upper bounds.
  if (val==1)
  {
  for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset);i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm;
		  int iter = text_len-j+i;
		  if (iter<0 or i<0)
			  continue;
		  else
		  {
		  if(tex[iter]=='A' or tex[iter]=='a')
		  {
			  currpm=patternbitmasks[0];
		  }
		  else if(tex[iter]=='C' or tex[iter]=='c')
		  {
		  	  currpm=patternbitmasks[1];
		  }
		  else if(tex[iter]=='G' or tex[iter]=='g')
		  {
			  currpm=patternbitmasks[2];
		  }
		  else if(tex[iter]=='T' or tex[iter]=='t')
		  {
		 	 currpm=patternbitmasks[3];
		  }
			  //code to get curr_pr
//		  std::cout << "text pos "<< iter<<"for j at"<< j << "for i at "<< i<<"\n";
//		  std::cout << "currpm is " << currpm << "\n";
//		  std::cout << "text" << tex[iter]<<" -----------------at --------------"<< iter <<"\n";
//		  //std::cout << "pattern bit mask"<< patternbitmasks[0]<<"\n";
//		  std::cout << "the tuple is("<< iter << ","<< i <<")\n";
		  if(i==0)
		  {
			  //compute single R with range
			  if(iter==text_len-1)
			  {	  R[iter][0]= oldR[0]<<1;
			  //std::cout << "after shift" <<R[iter][0] << "\n";
			      R[iter][0]|=currpm;
			  }
			  else
			  {
				 // std::cout << "before shift"<<R[iter+1][0]<<"\n";
				  R[iter][0]= R[iter+1][0]<<1;
				 // std::cout << "after shift" <<R[iter][0] << "\n";
				  R[iter][0]|=currpm;

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
				   del= R[iter+1][i-1];
				   ins=R[iter][i-1]<< 1; //cross dependency from prev level in graph
				   sub=R[iter+1][i-1] << 1;
				   match=R[iter+1][i] << 1;
				   match |=currpm;
                   R[iter][i]=del & ins & sub & match;

			  }
			  else
			  {
				  del=oldR[0];
				  ins=R[iter][i-1] << 1;
				  sub=oldR[0]<<1;
				  match=oldR[0]<<1;
				  match|=currpm;
                  R[iter][i]=del & ins & sub & match;
			  }
			  }

		  }}
  }
  else if (val==2)
  {
      for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm[2];
          int ind= i*2;
          std:: cout <<"text char"<< tex[text_len-j+i] << "\n";
		  if(tex[text_len-j+i]=='A' or tex[text_len-j+i]=='a')
		  {
			  currpm[0]=patternbitmasks[0];
              currpm[1]=patternbitmasks[1];
		  }
		  else if(tex[text_len-j+i]=='C' or tex[text_len-j+i]=='c')
		  {
		  	  currpm[0]=patternbitmasks[2];
              currpm[1]=patternbitmasks[3];
		  }
		  else if(tex[text_len-j+i]=='G' or tex[text_len-j+i]=='g')
		  {
			  currpm[0]=patternbitmasks[4];
              currpm[1]=patternbitmasks[5];
		  }
		  else if(tex[text_len-j+i]=='T' or tex[text_len-j+i]=='t')
		  {
		 	 currpm[0]=patternbitmasks[6];
             currpm[1]=patternbitmasks[7];
		  }
			  //code to get curr_pr
		  if(text_len-j+i<0 or i<0)
			  continue;
		  else
		  {
		  if(i==0)
		  {
			  //compute single R with range
			  if((text_len-j+i)==text_len-1)
			  {	  R[text_len-j+i][0]= shift(oldR[0],oldR[1])[0];
			      R[text_len+j+i][0]|=currpm[0];
                  R[text_len-j+i][1]= shift(oldR[0],oldR[1])[1];
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
				   del[0]=R[text_len-j+i+1][ind-2];
                   del[1]=R[text_len-j+i+1][ind-1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[0];
                   sub[1]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[1];
				   match[0]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  else
			  {
                   del[0]=oldR[0];
                   del[1]=oldR[1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(oldR[0],oldR[1])[0];
                   sub[1]=shift(oldR[0],oldR[1])[1];
				   match[0]=shift(oldR[0],oldR[1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(oldR[0],oldR[1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  }

		  }}
  }
  else
  {
	  std :: cout << "Not supported since value of num is " << val << "\n";
  }


  std::  cout << "Edit distance calculation started\n";
  //using old method
  int minerror=0;
//  for (int j=text_len-1;j>=0;j--)
//	  for (int i=0;i<=offset;i++)
//	  {
//		  if (msb(R[j][i])==0)
//		  {  minerror=i;
//		     break;
//		  }
//
//	  }
 out[0]=minerror;
  }}
//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>
#include <limits.h>
#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;
typedef unsigned __int128 uint128_t;

//TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

/*
    Bitap kernel implementation

        pm (input)          ---> patternbitmasks of pattern
        pattern   (input)     --> query string
        text   (input)     --> text string
        k   (input)     --> offset value
        m  (input)       -- > pattern length
        n (input)       ---> text length
        out   (output)    --> Output integer, edit distance

   */
  //for shifting two numbers , ie first bit of ind1 should be last bit of ind0
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

unsigned long long* shift(unsigned long long number0,unsigned long long number1)
{
unsigned long long* out;
int msb_rightpart=findmsb(number1,63);
out[1]=number1<<1;
out[0]=number0<<1+msb_rightpart;
return out;
}

int min(int a, int b)
{
	if(a<b)
		return a ;
	return b;
}


extern "C"
{
    void wide_vadd(
        unsigned long long  *pm,
        const char *pattern,
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

   patt_len=m[0];
   text_len=n[0];
   int num= ceil(m[0]/(64*1.0));
     num=num*4;
     int val = ceil (patt_len/(64.0));
     //std::cout << val << "is val\n";
  for (int i=0;i<num;i++)
	  patternbitmasks[i]=pm[i];

  int offset=k[0];
  int count =num*(offset+1);
  uint64_t oldR[2];
  oldR[0]=ULLONG_MAX;
  oldR[1]=ULLONG_MAX;
for (int i=0;i<text_len;i++)
	tex[i]=text[i];
for (int i=0;i<patt_len;i++)
	patt[i]=pattern[i];
 uint64_t R[110][25];
#pragma HLS BIND_OP variable=R op=mul impl=DSP latency=2
#pragma  HLS BIND_STORAGE variable=R type=RAM_2P impl=BRAM latency=2
//bind needs specific value hence specified upper bounds.
  if (val==1)
  {
  for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset);i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm;
		  int iter = text_len-j+i;
		  if (iter<0 or i<0)
			  continue;
		  else
		  {
		  if(tex[iter]=='A' or tex[iter]=='a')
		  {
			  currpm=patternbitmasks[0];
		  }
		  else if(tex[iter]=='C' or tex[iter]=='c')
		  {
		  	  currpm=patternbitmasks[1];
		  }
		  else if(tex[iter]=='G' or tex[iter]=='g')
		  {
			  currpm=patternbitmasks[2];
		  }
		  else if(tex[iter]=='T' or tex[iter]=='t')
		  {
		 	 currpm=patternbitmasks[3];
		  }
			  //code to get curr_pr
//		  std::cout << "text pos "<< iter<<"for j at"<< j << "for i at "<< i<<"\n";
//		  std::cout << "currpm is " << currpm << "\n";
//		  std::cout << "text" << tex[iter]<<" -----------------at --------------"<< iter <<"\n";
//		  //std::cout << "pattern bit mask"<< patternbitmasks[0]<<"\n";
//		  std::cout << "the tuple is("<< iter << ","<< i <<")\n";
		  if(i==0)
		  {
			  //compute single R with range
			  if(iter==text_len-1)
			  {	  R[iter][0]= oldR[0]<<1;
			  //std::cout << "after shift" <<R[iter][0] << "\n";
			      R[iter][0]|=currpm;
			  }
			  else
			  {
				 // std::cout << "before shift"<<R[iter+1][0]<<"\n";
				  R[iter][0]= R[iter+1][0]<<1;
				 // std::cout << "after shift" <<R[iter][0] << "\n";
				  R[iter][0]|=currpm;

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
				   del= R[iter+1][i-1];
				   ins=R[iter][i-1]<< 1; //cross dependency from prev level in graph
				   sub=R[iter+1][i-1] << 1;
				   match=R[iter+1][i] << 1;
				   match |=currpm;
                   R[iter][i]=del & ins & sub & match;

			  }
			  else
			  {
				  del=oldR[0];
				  ins=R[iter][i-1] << 1;
				  sub=oldR[0]<<1;
				  match=oldR[0]<<1;
				  match|=currpm;
                  R[iter][i]=del & ins & sub & match;
			  }
			  }

		  }}
  }
  else if (val==2)
  {
      for (int j=0;j<offset+text_len+1;j++)
	  for (int i=min(j-1,offset) ;i>=0;i--)
	  {
#pragma HLS loop unroll
//compute R[n-j+1][i]
		  //get current pattern bit mask
		  //how do you get for generalised ? , please see cpu for limits and do
		  uint64_t currpm[2];
          int ind= i*2;
          std:: cout <<"text char"<< tex[text_len-j+i] << "\n";
		  if(tex[text_len-j+i]=='A' or tex[text_len-j+i]=='a')
		  {
			  currpm[0]=patternbitmasks[0];
              currpm[1]=patternbitmasks[1];
		  }
		  else if(tex[text_len-j+i]=='C' or tex[text_len-j+i]=='c')
		  {
		  	  currpm[0]=patternbitmasks[2];
              currpm[1]=patternbitmasks[3];
		  }
		  else if(tex[text_len-j+i]=='G' or tex[text_len-j+i]=='g')
		  {
			  currpm[0]=patternbitmasks[4];
              currpm[1]=patternbitmasks[5];
		  }
		  else if(tex[text_len-j+i]=='T' or tex[text_len-j+i]=='t')
		  {
		 	 currpm[0]=patternbitmasks[6];
             currpm[1]=patternbitmasks[7];
		  }
			  //code to get curr_pr
		  if(text_len-j+i<0 or i<0)
			  continue;
		  else
		  {
		  if(i==0)
		  {
			  //compute single R with range
			  if((text_len-j+i)==text_len-1)
			  {	  R[text_len-j+i][0]= shift(oldR[0],oldR[1])[0];
			      R[text_len+j+i][0]|=currpm[0];
                  R[text_len-j+i][1]= shift(oldR[0],oldR[1])[1];
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
				   del[0]=R[text_len-j+i+1][ind-2];
                   del[1]=R[text_len-j+i+1][ind-1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[0];
                   sub[1]=shift(R[text_len-j+i+1][ind-2],R[text_len-j+i+1][ind-1])[1];
				   match[0]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(R[text_len-j+i+1][ind],R[text_len-j+i+1][ind+1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  else
			  {
                   del[0]=oldR[0];
                   del[1]=oldR[1];
				   ins[0]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[0]; //cross dependency from prev level in graph
                   ins[1]=shift(R[text_len-j+i][ind-2],R[text_len-j+i][ind-1])[1];
				   sub[0]=shift(oldR[0],oldR[1])[0];
                   sub[1]=shift(oldR[0],oldR[1])[1];
				   match[0]=shift(oldR[0],oldR[1])[0];
				   match[0] |=currpm[0];
                   match[1]=shift(oldR[0],oldR[1])[1];
				   match[1] |=currpm[1];

                   R[text_len-j+i][ind]= del[0] & ins[0] & sub[0] & match[0];
                   R[text_len-j+i][ind+1]= del[1] & ins[1] & sub[1] & match[1];

			  }
			  }

		  }}
  }
  else
  {
	  std :: cout << "Not supported since value of num is " << val << "\n";
  }


  std::  cout << "Edit distance calculation started\n";
  //using old method
  int minerror=0;
//  for (int j=text_len-1;j>=0;j--)
//	  for (int i=0;i<=offset;i++)
//	  {
//		  if (msb(R[j][i])==0)
//		  {  minerror=i;
//		     break;
//		  }
//
//	  }
 out[0]=minerror;
  }}
