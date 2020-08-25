#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

const int MaxBuffer=1024;
const int NEvent=254;//MaxBuffer;

const int NCH500M=3;
//const int NCH500M=2;
const int NSample500M=500;
const int NArray500M=(int)(NSample500M/2);//Nsample/ch/4;

const int EventSize=16+NArray500M*4*NCH500M;//1016byte
//const int MaxNEvent=2046;//1023000;//1.016GB
const int MaxFileSize=1e9;//1GB

const int NPD=30;
const int PREBIN=10; //1bin = 2ns
const int MIDBIN=10; //1bin = 2ns
const int POSTBIN=60; //1bin = 2ns

const int PHMIN=200.;
const int PHMAX=2000.;

const double MAXTIME=15.0e+06;

const double PlotShowInterval=10.;//sec
