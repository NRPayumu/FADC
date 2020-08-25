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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Date.h"
#include "Struct.h"
#include "CAENComm.h"

#include "TTree.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TApplication.h"

using namespace std;

//////////// Board Settings////////////////
int  handle;
const int Device = 0;
const int Link = 0;
const uint32_t BaseAddress=0x52100000;

int main(int argc,char **argv ) {

  const int nevents = atoi(argv[2]);
  const std::string outfile = argv[1];
/*
  outfile.str("");
  outfile<<DIR.c_str()<<argv[1]<<".root";
  cout<<"File->"<<DIR.c_str()<<argv[1]<<".root"<<endl;
  TFile* File=new TFile(ss.str().c_str(),"recreate");
*/
  const std::string filepath = Form("backup/%s",outfile.c_str());//make binary file
  
  //open binary file
  int fd = 0;
  if ((fd = ::open(filepath.c_str(), O_WRONLY | O_CREAT, 0666)) < 0) {
    perror("open");
    printf("Failed to open file : %s\n", filepath.c_str());
    return -1;
  }

  //////////////// Initialize the Board///////////////////////////
  int ret = 0;
  if( (ret = CAENComm_OpenDevice(CAENComm_OpticalLink, Link, Device, BaseAddress, &handle)) != CAENComm_Success) {
    cout << ret << endl;
    printf("\n\n Error opening the device\n");
    exit(1);
  }

  /////////////////Status register///////////////
  int addr=0x8000;
  uint32_t wdata=0x10;
  wdata = wdata | 0x40;  //Self Trigger Negative
  CAENComm_Write32(handle,0x8000, wdata);
  //cout << "0x8000 = " << ret << endl;

  CAENComm_Read32(handle,0x1028,&wdata);
  cout << "Dynamic Range=" << wdata << endl;
  /////////////////n channelDAC ///////////////
  CAENComm_Read32(handle,0x1088,&wdata);
  if(((wdata & 0x4) >>2)!=0)cout<<"can't write offset"<<endl;
  //  CAENComm_Read32(handle,0x1098,&wdata);
  //  cout << "Offset=" << wdata << endl;
  for(int i=0;i<NCH500M;i++){
    if(i==0) addr=0x1098;
    if(i==1) addr=0x1198;
    if(i==2) addr=0x1298;
    if(i==3) addr=0x1398;
    if(i==4) addr=0x1498;
    if(i==5) addr=0x1598;
    if(i==6) addr=0x1698;
    if(i==7) addr=0x1798;
    wdata=0x2C80;//1ch
    //wdata=0x3C80;
    CAENComm_Write32(handle,addr, wdata);
  }
  /*
  ////////////////n channel Threshold ///////////////
  cout << "SelfTriggerCheck ";
  for(int i=0;i<NCH500M;i++){
  if(i==0) addr=0x1080;
  if(i==1) addr=0x1180;
  if(i==2) addr=0x1280;
  if(i==3) addr=0x1380;
  if(i==4) addr=0x1480;
  if(i==5) addr=0x1580;
  if(i==6) addr=0x1680;
  if(i==7) addr=0x1780;
  wdata=0x3200;//1ch
  //wdata=0x1C80;
  wdata=0x05;
  wdata = wdata | (0x01 < 24);
  CAENComm_Write32(handle,addr, wdata);

  CAENComm_Read32(handle,addr,&wdata);
  double STC = wdata >24;
  cout << STC;
}
cout << endl;
wdata = wdata & 0x3fff;
cout << "Threshold=" << wdata << endl;
*/
CAENComm_Read32(handle,0x1098,&wdata);
cout << "Offset=" << wdata << endl;

/////////////////Channnel Enable Mask///////////////
addr=0x8120;
if(NCH500M==1)wdata=0x1;
if(NCH500M==2)wdata=0x3;
if(NCH500M==3)wdata=0x7;
if(NCH500M==4)wdata=0xf;
if(NCH500M==5)wdata=0x1f;
if(NCH500M==6)wdata=0x3f;
if(NCH500M==7)wdata=0x7f;
if(NCH500M==8)wdata=0xff;
CAENComm_Write32(handle,addr, wdata);

/////////////////wdata Organization ///////////////
////////Nr.ofblocks->1024/////////////////
addr=0x800C;
if(MaxBuffer==4)wdata=0x02;
if(MaxBuffer==8)wdata=0x03;
if(MaxBuffer==16)wdata=0x04;
if(MaxBuffer==32)wdata=0x05;
if(MaxBuffer==64)wdata=0x06;
if(MaxBuffer==128)wdata=0x07;
if(MaxBuffer==256)wdata=0x08;
if(MaxBuffer==512)wdata=0x09;
if(MaxBuffer==1024)wdata=0x0A;
CAENComm_Write32(handle,addr,wdata);

////////////////wdata Free///////////////
addr=0xEF28;
wdata=0x0;//1024
CAENComm_Write32(handle,addr,wdata);

///////////////Custom size///////////////////
addr=0x8020;
//wdata=0x19;
wdata=0x32;
//wdata=0x3A;   //NSample500M=600   in Struct.h
CAENComm_Write32(handle,0x8020, wdata);

//make tree
stringstream ss;
string DIR="data/";
int TTrigID = 0;
double TLivetime;
short TFADC[NCH500M][NSample500M];
short Tns500M[NSample500M];
float TPD[NCH500M], TMB[NCH500M], TPH[NCH500M],TRPHQ[NCH500M];
float TQ[NCH500M], TQtail[NCH500M];
float TQave,TQtailave, TRQ;
// float TPSD[NCH500M], TRPHQ[NCH500M];
//float TCHI[8], TTOF[8-1];

ss.str("");
ss<<DIR.c_str()<<argv[1]<<".root";
cout<<"File->"<<DIR.c_str()<<argv[1]<<".root"<<endl;
TFile* File=new TFile(ss.str().c_str(),"recreate");
//TFile* File=new TFile("test_ls.root","recreate");

double StartTime,EndTime;
double TotalRunTime,TotalLiveTime;

TTree* tree0=new TTree("RunInfo","RunInfo");
tree0->Branch("StartTime",&StartTime,"StartTime/D");
tree0->Branch("EndTime",&EndTime,"EndTime/D");
tree0->Branch("TotalRunTime",&TotalRunTime,"TotalRunTime/D");
tree0->Branch("TotalLiveTime",&TotalLiveTime,"TotalLiveTime/D");

TTree* tree=new TTree("tree","tree");
tree->SetMaxTreeSize(1e9);

tree->Branch("TrigID",&TTrigID,"TTrigID/I");
tree->Branch("LiveTime",&TLivetime,"TLivetime/D");
ss.str("");
ss<<"TFADC["<<NCH500M<<"]["<<NSample500M<<"]/S";
tree->Branch("FADC",TFADC,ss.str().c_str());
ss.str("");
ss<<"Tns["<<NSample500M<<"]/S";
tree->Branch("ns",Tns500M,ss.str().c_str());

ss.str("");
ss<<"TPD["<<NCH500M<<"]/F";
tree->Branch("PD",TPD,ss.str().c_str());

ss.str("");
ss<<"TMB["<<NCH500M<<"]/F";
tree->Branch("MB",TMB,ss.str().c_str());
ss.str("");
ss<<"TPH["<<NCH500M<<"]/F";
tree->Branch("PH",TPH,ss.str().c_str());
ss.str("");
ss<<"TQ["<<NCH500M<<"]/F";
tree->Branch("Q",TQ,ss.str().c_str());
ss.str("");
ss<<"TRPHQ["<<NCH500M<<"]/F";
tree->Branch("RPHQ",TRPHQ,ss.str().c_str());
ss.str("");
ss<<"TQtail["<<NCH500M<<"]/F";
tree->Branch("Qtail",TQtail,ss.str().c_str());
ss.str("");
tree->Branch("Qave",&TQave,"Qave/F");
tree->Branch("Qtailave",&TQtailave,"TQtailave/F");
tree->Branch("RQ",&TRQ,"TRQ/F");

for(int i=0;i<NSample500M;i++)Tns500M[i]=(double)i*2.;

/////////////Get DAQ Start Time///////////////////////
Date start_time;
/*
StartTime[0] = start_time.get();
StartTime[1] = start_time.getYear();
StartTime[2] = start_time.getMonth();
StartTime[3] = start_time.getDay();
StartTime[4] = start_time.getHour();
StartTime[5] = start_time.getMinitue();
StartTime[6] = start_time.getSecond();
*/
///////VME Control/////////////
addr=0xEF00;
wdata=0x100;
CAENComm_Write32(handle,addr,wdata);
cout<<"VME Control="<<wdata<<endl<<endl;

///////Aquisition Start/////////////
addr=0x8100;
wdata=0x1C;
CAENComm_Write32(handle,0x8100,wdata);
int nrecords = 0;
double pretime = 0;
double livetime_total=0;
uint32_t* event = new uint32_t[1024*1024*10]; //40MB
//uint32_t* event = new uint32_t[NEvent][4+NCH500M*NArray500M];
do{
  ///////wdata number//////////
  addr=0x812C;
  do {
    CAENComm_Read32(handle,addr,&wdata);
    //cout << wdata << endl;
  } while(wdata < (u_int32_t)(NEvent-1));

  int count = 0;
  int32_t size = NEvent * EventSize;

  CAENComm_MBLTRead(handle, 0x0, (uint32_t*)event, size, &count);
  cout << size << " " << count <<endl;
  uint32_t* buf = event;
  for(int iev = 0; iev < (NEvent-1);iev++){
    uint32_t evtsize = buf[0] & 0x0FFFFFFF;
    uint32_t chmask = (buf[1] & 0xFF) | ((buf[2] & 0xFF000000) >> 16);
    //uint32_t evtcount = (buf[2] & 0x00FFFFFF);
    uint32_t timetag = buf[3];

    //Caluculate LiveTime
    //
    double timetag_d = ((double)timetag) * 8e-9;
    double livetime = timetag_d - pretime;
    if(livetime<0)livetime+=4294967295* 8e-9 *0.5;
    pretime = timetag_d;
    livetime_total+=livetime;

    // debugging now...
    uint32_t bodysize = evtsize - 4;
    int nch = 0;
    for (int ich = 0; ich < 16; ich++) {
      if ((chmask>>ich & 0x1)) nch++;
    }
    const int ndata = bodysize / nch;

    //Caluculate PD MB PH Q QNaI Qtail
    //
    for (int ich = 0; ich < nch; ich++) {
      TPD[ich]=0;
      TMB[ich]=0;
      //TPH[ich]=15000;
      TPH[ich]=0;
      TQ[ich]=0;
      TQave=0;
      TQtail[ich]=0;
      TQtailave=0;
      TRQ=0;
      //TCHI[0]=0;
      for (int idata = 0; idata < ndata; idata++) {
        TFADC[ich][idata*2]  =(double)(buf[4+idata+(ich*ndata)]&0x3FFF);
        TFADC[ich][idata*2+1]=(double)(buf[4+idata+(ich*ndata)]>>16&0x3FFF);
        if(idata*2<NPD)TPD[ich]+=TFADC[ich][idata*2]+TFADC[ich][idata*2+1];
        if(idata*2==NPD)TPD[ich]/=(float)NPD;

       #if 1
        if(idata*2 > NPD && TPH[ich]<TPD[ich] -(float)TFADC[ich][idata*2]){
          TPH[ich]=TPD[ich] - (float)TFADC[ich][idata*2];
          TMB[ich]=(float)idata*2;
        }
        if(idata*2 > NPD && TPH[ich]<TPD[ich] -(float)TFADC[ich][idata*2+1]){
          TPH[ich]=TPD[ich] - (float)TFADC[ich][idata*2+1];
          TMB[ich]=(float)idata*2+1;
        }
       // cout << TTrigID << " " << TPH << endl;
       #endif
       #if 0
        if(TPH[ich]>(float)TFADC[ich][idata*2]){
          TPH[ich]=(float)TFADC[ich][idata*2];
          TMB[ich]=(float)idata*2;
        }
        if(TPH[ich]>(float)TFADC[ich][idata*2+1]){
          TPH[ich]=(float)TFADC[ich][idata*2+1];
          TMB[ich]=(float)idata*2+1;
        }
      // cout << TTrigID << " " << TPH << endl;
       #endif
      }
      for(int idata=0;idata<ndata;idata++){
        if(idata*2>TMB[ich]-PREBIN && idata*2 < TMB[ich]+POSTBIN){
          TQ[ich]+=TPD[ich]-(float)TFADC[ich][idata*2];
          TQ[ich]+=TPD[ich]-(float)TFADC[ich][idata*2+1];
        }
        if(idata*2>TMB[ich]+18 && idata*2 < TMB[ich]+POSTBIN){
          TQtail[ich]+=TPD[ich]-(float)TFADC[ich][idata*2];
          TQtail[ich]+=TPD[ich]-(float)TFADC[ich][idata*2+1];
        }
      }
	TQave = sqrt(TQ[0]*TQ[2]); //ch0,ch2PMT`s signal of GdLS
	TQtailave = sqrt(TQtail[0]*TQtail[2]); //ch0,ch2PMT`s signal of GdLS
        TRQ = TQ[0]/TQ[2];
      // TPSD[ich]=TQtail[ich]/TQ[ich];
      TRPHQ[ich]=TPH[ich]/TQ[ich];
    }

    TLivetime=livetime;
    tree->Fill();
    TTrigID++;

//binary file write
    write(fd, buf, evtsize * sizeof(uint32_t));
    nrecords++;
    buf += evtsize;
    count -= evtsize;
  }
  if (count != 0) {
    printf("Data still remains unrecorded!\n: count=%d\n", count);
  }
  cout << nrecords << " " << livetime_total << "sec"<<endl;

  if(pretime > MAXTIME){
    //////Aquisition Stop/////////////
    addr=0x8100;
    wdata = 0x8;
    CAENComm_Write32(handle,0x8100,wdata);

    ///////Aquisition Start/////////////
    addr=0x8100;
    //wdata=0x3C;
    wdata=0x1C;
    CAENComm_Write32(handle,0x8100,wdata);
  }

  //}while(nrecords < nevents);
}while(livetime_total < (double)nevents);
//////Aquisition Stop/////////////
addr = 0x8100;
wdata = 0x8;
CAENComm_Write32(handle,0x8100,wdata);
close(fd);//binary file close

/////////////Get DAQ End Time///////////////////////
Date end_time;
/*
EndTime[0] = end_time.get();
EndTime[1] = end_time.getYear();
EndTime[2] = end_time.getMonth();
EndTime[3] = end_time.getDay();
EndTime[4] = end_time.getHour();
EndTime[5] = end_time.getMinitue();
EndTime[6] = end_time.getSecond();
*/

cout<<endl;
cout<<"StartTime = "<< start_time.toString() <<endl;
cout<<"EndTime   = "<< end_time.toString() <<endl;
double runtime = end_time.get() - start_time.get();

StartTime = start_time.get();
EndTime = end_time.get();
TotalRunTime = runtime;
TotalLiveTime = livetime_total;

tree0->Fill();

File=tree->GetCurrentFile();
File->Write();
File->Close();

cout<< endl;
cout<<"File Name        = "<< filepath << endl;
cout<<"Number of Events = "<< nrecords << endl;
cout<<"TotalRunTime     = "<< runtime <<" sec"<<endl;
cout<<"livetime_total    = "<< livetime_total<<" sec"<<endl;
cout<<"Rate             = "<<(double) nrecords / livetime_total<<" Hz"<<endl;

return 0;
}
