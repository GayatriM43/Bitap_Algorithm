 #include <stdlib.h>
 #include <string.h>
 #include <limits.h>    //1110 (14)  1100 (12) 11100 (28,1) % 16 =12
 #include <stdio.h>                   //111000 (56,2) % 16 = 8
 #include <math.h>
 #define BITS sizeof(int)*8
 //current params maxedit distance =12 ; max read length : 32 
 //change it later to unsigned long long int to hold 64-TBD
 int perfop(int value , int maxbits)
 {
    //printf("%d %d", value, maxbits); 
    return  (value << 1) & (int)(pow(2,maxbits)-1);
 }
 int getval(char x)
 {
     if(x=='A')
     return 0;
     else if(x=='G')
     return 1;
     else if (x=='C')
     return 2;
     else
     return 3;
 }
 const char *bitap_fuzzy_search(const char *text, const char *pattern, int k)
 {

     int m=strlen(pattern);
     int n = strlen(text);
     int PM[5]={0};   // A-0 , G-1, C-2, T-3
     int R[12]={pow(2,m)-1};
     int oldR[12]={0};

    //generating pattern bitmasks

     for (int i=0;i<m;i++)
     {
         if(pattern[i]=='A')
        { 
            PM[1]+=pow(2,m-i-1);
            PM[2]+=pow(2,m-i-1);
            PM[3]+=pow(2,m-i-1);
        }
        else if (pattern[i]=='G')
        {
            PM[0]+=pow(2,m-i-1);
            PM[2]+=pow(2,m-i-1);
            PM[3]+=pow(2,m-i-1);
        }
        else if (pattern[i]=='C')
      {
            PM[0]+=pow(2,m-1-i);
            PM[1]+=pow(2,m-1-i);
            PM[3]+=pow(2,m-1-i);
        }
        else if (pattern[i]=='T')
        {
            PM[0]+=pow(2,m-1-i);
            PM[1]+=pow(2,m-1-i);
            PM[2]+=pow(2,m-1-i);
        }

        
     }
    //printf("%d\n",PM[3]);
    unsigned int  msb = 1 << (m - 1);

    int startloc=-1;
    int ed=-1;
    int D,S,M,I;
    for (int i=n-1;i!=-1;--i)
    {
        for(int d=0;d<=k;d++)
        { 
            oldR[d]=R[d];
        }
        int curPM = PM[getval(text[i])];
        //printf("%d\n",getval(text[i]));
        //printf("%d\n",R[0]);
        R[0]=perfop(oldR[0],m) | curPM;  //exact matching step
        //printf("R[0] is %d\n",curPM);
        for (int d=1;d<=k;d++)
        {
            I = perfop(R[d-1],m);
            S = perfop(oldR[d-1],m);
            D = oldR[d-1];
            M = perfop(oldR[d],m) | curPM;
            R[d]= D & S & I & M;

        
        }
        //printf("checks");
        for (int d=0;d<=k;d++)
        {
            //printf("%d and %d\n",R[d], (int)pow(2,m-1));
        if ( R[d] > (int)pow(2,m-1) )
        {
            
            startloc = i;
            if(d>ed)
            ed = d;
            //printf("here is %d\n",ed);
        }}
        //printf("%d",R[0]);
    }
    printf("edit distance : %d \n",ed);

 }

 int main()
 {
    //FILE *f= dup(STDOUT_FILENO);
    bitap_fuzzy_search("CGTGA", "CTGA", 7);
    
 }