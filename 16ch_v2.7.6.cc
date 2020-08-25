#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "Struct.h"
#include "CAENComm.h"

using namespace std;

//////////// Board Settings////////////////  
int  Device=0;
int  Link=0;
int  BHandle;

string DIR="data/";
const int NMOD=1;

uint32_t FADC_Address=0x52100000;

float RefV[PREBIN+POSTBIN];
float RefE[PREBIN+POSTBIN];

int main(int argc,char **argv ) {

  struct tm *time_st;
  struct timeval tmpstime,tmpetime;
  time_t starttimer;

  struct Struct500M *buf500M;

  int NowTime;
  int TimeVal[4];	

  int gscal=0;
  int TrigID;
  double StartTime[6],EndTime[6];
  double IntervalTime=0;
  double PreTime[2]={0,0};
  double CurrTime[2]={0,0};
  double TotalRunTime=0;
  double TotalLiveTime=0;
  double LiveTime;
  double pretime=0;

  short FADC[NCH500M][NSample500M];
  float PD[NCH500M];

  int nevent = atoi(argv[2]);

  //////////////// Initialize the Board///////////////////////////
  int ret = 0;
  if( (ret = CAENComm_OpenDevice(CAENComm_OpticalLink, Link, 0, FADC_Address, &BHandle)) != CAENComm_Success) {
    cout << ret << endl;
    printf("\n\n Error opening the device\n");
    exit(1);
  }

  /////////////////Status register/////////////// 
  int Addrs=0x8000;
  uint32_t Buffer=0x10;
  CAENComm_Write32(BHandle,0x8000, Buffer);
  cout << "0x800 = " << ret << endl;

  /////////////////n channelDAC ///////////////
  for(int i=0;i<NCH500M;i++){
    if(i==0)Addrs=0x1098;
    if(i==1)Addrs=0x1198;
    if(i==2)Addrs=0x1298;
    if(i==3)Addrs=0x1398;
    if(i==4)Addrs=0x1498;
    if(i==5)Addrs=0x1598;
    if(i==6)Addrs=0x1698;
    if(i==7)Addrs=0x1798;
    Buffer=0x2C80;//1ch
    CAENComm_Write32(BHandle,Addrs, Buffer);
  }
  
  /////////////////Channnel Enable Mask///////////////
  Addrs=0x8120;
  if(NCH500M==1)Buffer=0x1;
  if(NCH500M==2)Buffer=0x3;
  if(NCH500M==3)Buffer=0x7;
  if(NCH500M==4)Buffer=0xf;
  if(NCH500M==5)Buffer=0x1f;
  if(NCH500M==6)Buffer=0x3f;
  if(NCH500M==7)Buffer=0x7f;
  if(NCH500M==8)Buffer=0xff;
  CAENComm_Write32(BHandle,Addrs, Buffer);
  
  /////////////////Buffer Organization ///////////////
  ////////Nr.ofblocks->1024/////////////////
  Addrs=0x800C;
  if(MaxBuffer==4)Buffer=0x02;
  if(MaxBuffer==8)Buffer=0x03;
  if(MaxBuffer==16)Buffer=0x04;
  if(MaxBuffer==32)Buffer=0x05;
  if(MaxBuffer==64)Buffer=0x06;
  if(MaxBuffer==128)Buffer=0x07;
  if(MaxBuffer==256)Buffer=0x08;
  if(MaxBuffer==512)Buffer=0x09;
  if(MaxBuffer==1024)Buffer=0x0A;
  CAENComm_Write32(BHandle,Addrs,Buffer);
  
  ////////////////Buffer Free///////////////
  Addrs=0xEF28;
  Buffer=0x0;//1024
  CAENComm_Write32(BHandle,Addrs,Buffer);
  
  ///////////////Custom size///////////////////
  Addrs=0x8020;
  //Buffer=0x19;
  Buffer=0x32;
  CAENComm_Write32(BHandle,0x8020, Buffer);
  
  /////////////Get DAQ Start Time///////////////////////
  time(&starttimer);
  gettimeofday(&tmpstime,NULL);
  time_st=localtime(&tmpstime.tv_sec);
  
  StartTime[0]=time_st->tm_year+1900;
  StartTime[1]=time_st->tm_mon+1;
  StartTime[2]=time_st->tm_mday;
  StartTime[3]=time_st->tm_hour;
  StartTime[4]=time_st->tm_min;
  StartTime[5]=time_st->tm_sec+tmpstime.tv_usec*1.0e-06;
  
  ////////////Data taking///////////////////
  TrigID=0;   
  
  ///////VME Control/////////////
  Addrs=0xEF00;
  Buffer=0x100;
  CAENComm_Write32(BHandle,Addrs,Buffer);
  cout<<"VME Control="<<Buffer<<endl;
  
  ///////Aquisition Start/////////////
  Addrs=0x8100;
  Buffer=0x1C;
  CAENComm_Write32(BHandle,0x8100,Buffer);
  do{ 
    ///////Buffer number//////////
    Addrs=0x812C;
    do{
      CAENComm_Read32(BHandle,Addrs,&Buffer);
     }while(Buffer<(u_int32_t)(NEvent-1));
    
    double tmplt=0;
    
    int count;
    int32_t Size=NEvent*EventSize;
    
    buf500M=(Struct500M *)malloc(sizeof(Struct500M));   
    CAENComm_MBLTRead(BHandle, 0x0, (uint32_t*)buf500M, Size,&count);
    cout<<Size<<" "<<count<<endl;
    
    for(int BufferNo=0;BufferNo<(NEvent-1);BufferNo++){
      NowTime=buf500M->fevent[BufferNo][3];
      for(int p=0;p<4;p++){
	TimeVal[p]=(0xff & NowTime);
	NowTime=NowTime>>8;
      }
      
      LiveTime=(double)((TimeVal[0]+TimeVal[1]*pow(16,2.0)
			 +TimeVal[2]*pow(16,4.0)+TimeVal[3]*pow(16,6.0))*8./1000.)-pretime;
      
      if(LiveTime<0)LiveTime=(double)((TimeVal[0]+TimeVal[1]*pow(16,2.0)
					+TimeVal[2]*pow(16,4.0)+TimeVal[3]*pow(16,6.0))*8./1000.);
      
      //if(LiveTime<0)LiveTime+=4294967295;
      //if(LiveTime<0)LiveTime+=(8.*(2147483647-1)/1000.);
       
      
      pretime=(double)((TimeVal[0]+TimeVal[1]*pow(16,2.0)
			+TimeVal[2]*pow(16,4.0)+TimeVal[3]*pow(16,6.0))*8./1000.);
      
      //500M 1//
      for(int ch=0;ch<NCH500M;ch++){
	PD[ch]=0;
	for(int bin=0;bin<NArray500M;bin++){
	  for(int p=0;p<2;p++){
	    //FADC[ch][bin*2+p]=(0x3fff & buf500M->fadc[BufferNo][ch][bin]);
	    //buf500M->fadc[BufferNo][ch][bin]=buf500M->fadc[BufferNo][ch][bin]>>16;
	    FADC[ch][bin*2+p]=(0x3fff & buf500M->fevent[BufferNo][ch*NArray500M+bin+4]);
	    buf500M->fevent[BufferNo][ch*NArray500M+bin+4]=buf500M->fevent[BufferNo][ch*NArray500M+bin+4]>>16;
	    if((bin*2+p)<NPD){
	      PD[ch]+=FADC[ch][bin*2+p];
	      //htmp->SetBinContent(bin*2+p+1,0);
	    }
	    if((bin*2+p)==NPD)PD[ch]/=(float)NPD;
	    //if((bin*2+p)>=NPD)htmp->SetBinContent(bin*2+p+1,PD[ch]-FADC[ch][bin*2+p]);
	  }
	}
      }
      TotalLiveTime+=LiveTime;
      tmplt+=LiveTime;
      
      TrigID++;
      
    }
    cout<<TrigID<<" "<<TotalLiveTime*1.0e-06<<"sec"<<endl;
    
    free(buf500M);

    if(pretime>MAXTIME){
      //////Aquisition Stop/////////////
      Addrs=0x8100;
      Buffer = 0x8;  
      CAENComm_Write32(BHandle,0x8100,Buffer);

      ///////Aquisition Start/////////////
      Addrs=0x8100;
      //Buffer=0x3C;
      Buffer=0x1C;
      CAENComm_Write32(BHandle,0x8100,Buffer);
    }

    //gRate->SetPoint(gscal, TotalLiveTime*1.0e-06, NEvent/(tmplt*1.0e-06));    
    gscal++;

    gettimeofday(&tmpstime,NULL);
    time_st=localtime(&tmpstime.tv_sec);
    
    CurrTime[0]=time_st->tm_min;
    CurrTime[1]=time_st->tm_sec+tmpstime.tv_usec*1.0e-06;

    IntervalTime+=((CurrTime[0]-PreTime[0])*60.);
    IntervalTime+=((CurrTime[1]-PreTime[1]));

    cout<<CurrTime[0]<<" "<<CurrTime[1]<<" ";
    cout<<PreTime[0]<<" "<<PreTime[1]<<" ";
    cout<<IntervalTime<<endl;

    PreTime[0]=CurrTime[0];
    PreTime[1]=CurrTime[1];

    //if(IntervalTime>PlotShowInterval)IntervalTime=0;

    if(IntervalTime>PlotShowInterval){
      IntervalTime=0;
    }
  
   } while(TrigID<nevent);

   //////Aquisition Stop/////////////
   Addrs =0x8100;
   Buffer = 0x8;  
   CAENComm_Write32(BHandle,0x8100,Buffer);
 

  /////////////Get DAQ End Time///////////////////////
   TotalLiveTime*=1.0e-06;

  gettimeofday(&tmpetime,NULL);
  time_st=localtime(&tmpetime.tv_sec);

  EndTime[0]=time_st->tm_year+1900;
  EndTime[1]=time_st->tm_mon+1;
  EndTime[2]=time_st->tm_mday;
  EndTime[3]=time_st->tm_hour;
  EndTime[4]=time_st->tm_min;
  EndTime[5]=time_st->tm_sec+tmpetime.tv_usec*1.0e-06;

  cout<<endl;
  cout<<"StartTime = "<<StartTime[0]<<"."<<StartTime[1]<<"."<<StartTime[2]<<", ";
  cout<<StartTime[3]<<":"<<StartTime[4]<<":"<<StartTime[5]<<endl;
  cout<<"EndTime   = "<<EndTime[0]<<"."<<EndTime[1]<<"."<<EndTime[2]<<", ";
  cout<<EndTime[3]<<":"<<EndTime[4]<<":"<<EndTime[5]<<endl;

  TotalRunTime+=((EndTime[0]-StartTime[0])*365.*86400.);
  //if((EndTime[1]-StartTime[1])<0)TotalRunTime-=(365.*86400.);
  TotalRunTime+=((EndTime[1]-StartTime[1])*30.*86400.);
  //if((EndTime[2]-StartTime[2])<0)TotalRunTime-=(30.*86400.); 
  TotalRunTime+=((EndTime[2]-StartTime[2])*86400.);
  //if((EndTime[3]-StartTime[3])<0)TotalRunTime-=86400.;
  TotalRunTime+=((EndTime[3]-StartTime[3])*3600.);
  //if((EndTime[4]-StartTime[4])<0)TotalRunTime-=3600.;
  TotalRunTime+=((EndTime[4]-StartTime[4])*60.);
  //if((EndTime[5]-StartTime[5])<0)TotalRunTime-=60.;
  TotalRunTime+=((EndTime[5]-StartTime[5]));

  cout<<endl;
  cout<<"File Name        = "<<argv[1]<<endl;
  cout<<"Number of Events = "<<TrigID<<endl;
  cout<<"TotalRunTime     = "<<TotalRunTime<<" sec"<<endl;
  cout<<"TotalLiveTime    = "<<TotalLiveTime<<" sec"<<endl;
  cout<<"Rate             = "<<(double)TrigID/TotalLiveTime<<" Hz"<<endl;

  return 0;
}
