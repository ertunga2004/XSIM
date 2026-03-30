#include <stdio.h>
#include <io.h>
#include <math.h>
#include <stdlib.h>
#include <iomanip>
#include <conio.h>
#include <iostream>


// CONSTANT VAR.

float IntTime = 9,
      SerTime = 8;

long  NWarmUp = 5000;

int I;
FILE *F[10], *FF, *File;


int   DDRule = 1;   // TWK

float k1 = 6.963, k2 = 0.0, k3 = 0.0, k4 = 0.0;

/*
int   DDRule = 2;   // NOP

float k1 = 50.062, k2 = 0.0, k3 = 0.0, k4 = 0.0;


int   DDRule = 3;   // TWK+NOP

float k1 = 21.511, k2 = -143.238, k3 = 0.0, k4 = 0.0;


int   DDRule = 4;   // JIQ

float k1 = 14.294, k2 = -17.614, k3 = 0.0, k4 = 0.0;


int   DDRule = 5;   // WIQ

float k1 = 12.213, k2 = -0.048, k3 = 0.0, k4 = 0.0;


int   DDRule = 6;   // RMR

float k1 = 26.948, k2 = 0.009, k3 = 0.699, k4 = -0.037;
*/


double SEED;
int PRIME,
    PrimeList[10] = { 347, 349, 359, 367, 373,
                      419, 421, 431, 433, 439};

const int N_OP  = 9;
const int N_RO  = 5;
const int N_MAC = 9;
const int N_JOB = 1000;

const int ARRIVAL = 1;
const int DEPART  = 2;

const int IDLE = 0;
const int BUSY = 1;

const double MAXF = 9999999.0;
const int MAXI = 9999999;

float MachineServiceTimes[N_MAC + 1] = {5, 7, 2, 3, 6, 1, 7, 9, 4, 2}; // Makine bazlı servis süreleri

// STRUCT

struct JobType
{ long  No;
  int   Type;
  float r;
  int   NOP,
        cNOP,
        rNOP,
        M[N_OP+1];
  float P[N_OP+1],
        d,
        TPT,
        cTPT,
        rTPT,
        C,
        F,
        L,
        T,
        E,
        ets,
        Pt,
        etf,
        EW[N_OP+1];

  int   Mt,
        Status;

  long  OOR,
        JIQOR;
  float MaxP,
        EWOR,
        TPTOR;

  JobType *Next;
} *Job, *First, *Pred, *Temp;

struct MacType
{ int   No;
  int   Status;
  float FreeTime,
        BusyTime,
        IdleTime,
        TUTime,
        TITime;
  long  JIQ,
        TIQ;
  float WIQ,
        EW;
} Mac[N_MAC+1];

struct RouteType
{ int  No,
       NOP,
       M[N_OP+1];
} Route[N_RO+1];

struct NextDepType
{ long  JNo;
  int   MNo;
  float Time;
} NextDep;

// GENERAL VAR.

float TotalF,
      AvgF,
      TotalT,
      AvgT,
      TotalE,
      AvgE,
      TAD,
      TSE,
      MAD,
      MSE,
      TotalMacTime,
      TotalShopTime;

int   Util;

long  NJobsComing,
      PNJobsInShop,
      NJobsInShop,
      NJobsDesired,
      NJobsComp,
      NJobsTard,
      NJobsEar,
      TNJobsInShop;

float ArrTime,
      NextArrTime;

int   NextEvent;

float t;

long NAppJobs;

int  AppJobs[N_JOB],
     AppJob,
     DelJob;


// UNIFORM & EXP. DIST.

float Uniform()
{
  float Result, Frac;
  Result = SEED * PRIME;
  long x = Result;
  Frac = Result - x;
  SEED = Frac;
  return Frac;
}

float Exp(float x)
{
  return ( -x * log( (1-Uniform()) ) );
}

int RouteNo()
{
  float x;
  int RN;
  x = Uniform();
  if (x<0.2)           RN = 1;
  if (x>=0.2 && x<0.4) RN = 2;
  if (x>=0.4 && x<0.6) RN = 3;
  if (x>=0.6 && x<0.8) RN = 4;
  if (x>=0.8)          RN = 5;
  return RN;
}

// ARRIVAL TIME

void CreateArrTime()
{
  float x;
  x = 1 + Exp(IntTime);
  ArrTime += x;
}

// INITALIZING

void Initialize()
{
  Job   = 0;
  First = 0;
  Pred  = 0;
  Temp  = 0;

  for (int i=1; i<=N_MAC; i++)
  {
    Mac[i].No = i;
    Mac[i].Status = IDLE;
    Mac[i].FreeTime = 0.0;
    Mac[i].BusyTime = 0.0;
    Mac[i].IdleTime = 0.0;
    Mac[i].TUTime = 0.0;
    Mac[i].TITime = 0.0;
    Mac[i].JIQ = 0;
    Mac[i].WIQ = 0.0;
    Mac[i].EW  = 0.0;
    Mac[i].TIQ = 0;
  }

  Route[1].No = 1;
  Route[1].NOP = N_MAC;
  Route[1].M[1] = 3;
  Route[1].M[2] = 2;
  Route[1].M[3] = 1;
  Route[1].M[4] = 5;
  Route[1].M[5] = 4;
  Route[1].M[6] = 9;
  Route[1].M[7] = 6;
  Route[1].M[8] = 8;
  Route[1].M[9] = 7;

  Route[2].No = 2;
  Route[2].NOP = N_MAC;
  Route[2].M[1] = 5;
  Route[2].M[2] = 3;
  Route[2].M[3] = 4;
  Route[2].M[4] = 1;
  Route[2].M[5] = 2;
  Route[2].M[6] = 9;
  Route[2].M[7] = 7;
  Route[2].M[8] = 6;
  Route[2].M[9] = 8;

  Route[3].No = 3;
  Route[3].NOP = N_MAC;
  Route[3].M[1] = 8;
  Route[3].M[2] = 1;
  Route[3].M[3] = 6;
  Route[3].M[4] = 5;
  Route[3].M[5] = 2;
  Route[3].M[6] = 3;
  Route[3].M[7] = 7;
  Route[3].M[8] = 9;
  Route[3].M[9] = 4;

  Route[4].No = 4;
  Route[4].NOP = N_MAC;
  Route[4].M[1] = 3;
  Route[4].M[2] = 5;
  Route[4].M[3] = 4;
  Route[4].M[4] = 1;
  Route[4].M[5] = 7;
  Route[4].M[6] = 2;
  Route[4].M[7] = 9;
  Route[4].M[8] = 8;
  Route[4].M[9] = 6;

  Route[5].No = 5;
  Route[5].NOP = N_MAC;
  Route[5].M[1] = 6;
  Route[5].M[2] = 2;
  Route[5].M[3] = 3;
  Route[5].M[4] = 5;
  Route[5].M[5] = 9;
  Route[5].M[6] = 1;
  Route[5].M[7] = 4;
  Route[5].M[8] = 8;
  Route[5].M[9] = 7;

  TAD = 0.0;
  TSE = 0.0;
  TotalF = 0.0;
  TotalT = 0.0;
  TotalE = 0.0;
  TotalMacTime = 0.0;
  TotalShopTime = 0.0;
  NJobsComing = 0;
  PNJobsInShop = 0;
  NJobsInShop = 0;
  NJobsComp = 0;
  NJobsTard = 0;
  NJobsEar = 0;
  TNJobsInShop = 0;
  NAppJobs = 0;

  NextDep.JNo = 0;
  NextDep.MNo = 0;
  NextDep.Time = MAXF;

  ArrTime = 0.0;
  t = 0.0;
}

// CLEAR JOB LIST

void ClearList()
{
  Job = First;
  while (Job->Next != 0)
  {
    Temp = Job;
    Job = Job->Next;
    delete Temp;
  }
  delete Job;
}



// DUE DATE DETERMINATION

float DueDate()
{
  long  JIQ = 0,
        WIS = 0;
  float WIQ = 0.0,
        EW  = 0.0;

  float DD;

  JobType *J;

  int i;

  switch (DDRule)
  {
    case 7: // for NN
    case 1: // TWK
      DD = Job->r + k1*Job->TPT;
      break;
    case 2: // NOP
      DD = Job->r + k1*Job->NOP;
      break;
    case 3: // TWK+NOP
      DD = Job->r + k1*Job->TPT + k2*Job->NOP;
      break;
    case 4: // JIQ
      for (i=1; i<=N_OP; i++)
        JIQ += Mac[Job->M[i]].JIQ;
      DD = Job->r + k1*Job->TPT + k2*JIQ;
      break;
    case 5: // WIQ
      for (i=1; i<=N_OP; i++)
        WIQ += Mac[Job->M[i]].WIQ;
      DD = Job->r + k1*Job->TPT + k2*WIQ;
      break;
    case 6: // RMR
      for (i=1; i<=N_OP; i++)
        WIQ += Mac[Job->M[i]].WIQ;
      J = First;
      while (J != 0)
      { WIS += J->rNOP;
        J = J->Next;
      }
      for (i=1; i<=N_OP; i++)
      if (Mac[Job->M[i]].JIQ <= 0) EW = 0;
      else EW += Mac[Job->M[i]].EW / Mac[Job->M[i]].JIQ;
      DD = k1*Job->NOP + k2*WIQ + k3*WIS + k4*EW;
      break;
  }
  return DD;
}


// WRITING INFO INTO FILE

void WriteInfoToFile()
{
  float AD = fabs(Job->C-Job->d);
  float SE = pow(AD, 2);

  fprintf(F[I], "%8.0f%15.0f%8.0f%8.0f%15.0f\n",
                AD, SE, Job->T, Job->E, Job->F );

}

void WriteFinalResultsToFile(int simIndex)
{
  if (NJobsComp > NWarmUp) AvgF = TotalF/(NJobsComp-NWarmUp);
  else AvgF = 0;

  if (NJobsComp > NWarmUp) AvgT = TotalT/(NJobsComp-NWarmUp);
  else AvgT = 0;

  if (NJobsComp > NWarmUp) AvgE = TotalE/(NJobsComp-NWarmUp);
  else AvgE = 0;

  if (NJobsComp > NWarmUp) MAD = TAD/(NJobsComp-NWarmUp);
  else MAD = 0;

  if (NJobsComp > NWarmUp) MSE = TSE/(NJobsComp-NWarmUp);
  else MSE = 0;

  // Ana sonuç dosyasına yazma:
  fprintf(File, "%8.0f  %8.0f  %8.0f  %8d  %8.0f  %8d  %8.0f  %8d\n",
    MAD, MSE, AvgT, NJobsTard, AvgE, NJobsEar, AvgF, NJobsInShop);
// Aynı sonuçları geçici dosyaya da yazma:
  fprintf(F[simIndex], "%8.0f  %8.0f  %8.0f  %8d  %8.0f  %8d  %8.0f  %8d\n",
    MAD, MSE, AvgT, NJobsTard, AvgE, NJobsEar, AvgF, NJobsInShop);
}


// DISPLAYING INFO

void DisplayInfo()
{
    // Ekranı temizleme ve imleci başa taşıma (isteğe bağlı)
    std::cout << "\033[H";  // Imleci en üst satıra taşır (opsiyonel)

    // Yüzdeyi hesapla
    int x = (float)NJobsComp / NJobsDesired * 100;

    // Ekrana yüzdeyi yazdır
    std::cout << x << " % complete..." << std::endl;
}


void WhatIsNextEvent()
{
  if (NJobsInShop < 1 || NextArrTime <= NextDep.Time)
  {
    NextEvent = ARRIVAL;
    t = NextArrTime;
  }
  else
  {
    NextEvent = DEPART;
    t = NextDep.Time;
  }
}

void FindNextDep()
{
  NextDep.Time = MAXF;
  NextDep.JNo = 0;
  NextDep.MNo = 0;

  Job = First;
  while (Job!= 0)
  {
    if (Job->etf < NextDep.Time)
    {
      NextDep.Time = Job->etf;
      NextDep.MNo = Job->Mt;
      NextDep.JNo = Job->No;
    }
    Job = Job->Next;
  }
}

void Update(int M, int N)
{
  Job = First;
  while (Job!= 0 )
  {
    if (Job->Mt == M && Job->No != N)
      if (Job->ets < Mac[M].FreeTime)
      {
        Job->EW[M] += (Mac[M].FreeTime - Job->ets);
        Mac[M].EW += (Mac[M].FreeTime - Job->ets);
        Job->ets = Mac[M].FreeTime;
      }
    Job = Job->Next;
  }
}

void Arrive()
{
  if (First == 0)
  {
    First = new JobType;
    Job = First;
    Job->Next = 0;
  }
  else
  {
    Job = First;
    while (Job->Next != 0) Job = Job->Next;
    Job->Next = new JobType;
    Job = Job->Next;
    Job->Next = 0;
  }
  NJobsInShop++;
  NJobsComing++;

  Job->No = NJobsComing;
  Job->Type = RouteNo();
  Job->r = t;
  Job->NOP = Route[Job->Type].NOP;
  Job->cNOP = 0;
  Job->rNOP = Job->NOP;
  Job->TPT = 0.0;
  for (int i=1; i<=Job->NOP; i++)
  {
    Job->M[i] = Route[Job->Type].M[i];
    Job->P[i] = 1 + Exp(MachineServiceTimes[Job->M[i]]); // Her makinenin kendi süresi kullanılıyor
    Job->TPT += Job->P[i];
    Job->EW[i] = 0.0;
  }
  Job->cTPT = 0.0;
  Job->rTPT = Job->TPT;
  Job->TPTOR = Job->TPT;
  Job->d = DueDate();
  Job->Mt = Job->M[1];
  Job->Pt = Job->P[1];

  if (Mac[Job->Mt].Status == IDLE)
  {
    Mac[Job->Mt].TITime += (t - Mac[Job->Mt].IdleTime);
    Mac[Job->Mt].Status = BUSY;
    Mac[Job->Mt].BusyTime = t;
    Job->ets = t;
    Job->etf = Job->ets + Job->Pt;
    Mac[Job->Mt].FreeTime = Job->etf;

    Update(Job->Mt, Job->No);

  }
  else
  {
    Job->ets = Mac[Job->Mt].FreeTime;
    Job->EW[Job->Mt] = (Job->ets - t);
    Mac[Job->Mt].EW += Job->EW[Job->Mt];
    Job->etf = MAXF;
    Mac[Job->Mt].JIQ++;
    Mac[Job->Mt].TIQ++;
    Mac[Job->Mt].WIQ += Job->Pt;
  }

  CreateArrTime();
  NextArrTime = ArrTime;
  FindNextDep();
}

enum SchedulingRule {
    SPT = 1,
    EDD,
    LIFO,
    LPT,
    STPT,
    LDR,
    LDT,
    SDT,
    SDR,
    PTWINQ,
    NINQ,
    LTWR,
    MTWR,
    SMR,
    SIO,
    LMR,
    LMT,
    LTPT,
    LRO,
    SMT
};

SchedulingRule GetUserSchedulingRule() {
    int choice;
    std::cout << "\nSiralama Kurali Secin:\n";
    std::cout << "1. SPT (Shortest Processing Time)\n";
    std::cout << "2. EDD (Earliest Due Date)\n";
    std::cout << "3. LIFO (Last In First Out)\n";
    std::cout << "4. LPT (Longest Processing Time)\n";
    std::cout << "5. STPT (Shortest Total Processing Time)\n";
    std::cout << "6. LDR (Largest Dynamic Ratio)\n";
    std::cout << "7. LDT (Largest Dynamic Total)\n";
    std::cout << "8. SDT (Smallest Dynamic Total)\n";
    std::cout << "9. SDR (Smallest Dynamic Ratio)\n";
    std::cout << "10. PT+WINQ (Process Time + Waiting in Queue)\n";
    std::cout << "11. NINQ (Next In Queue)\n";
    std::cout << "12. LTWR (Least Time to Wait in Queue)\n";
    std::cout << "13. MTWR (Most Time to Wait in Queue)\n";
    std::cout << "14. SMR (Smallest Mean Ratio)\n";
    std::cout << "15. SIO (Shortest Imminent Operation)\n";
    std::cout << "16. LMR (Largest (operation time * total operation time))\n";
    std::cout << "17. LMT (Largest (operation time * total processing time))\n";
    std::cout << "18. LTPT (Longest Total Processing Time)\n";
    std::cout << "19. LRO (Largest Unassigned Operation Count)\n";
    std::cout << "20. SMT (Smallest (operation time * total operation time))\n";
    std::cout << "Seciminizi yapin (1-20): ";
    std::cin >> choice;

    return static_cast<SchedulingRule>(choice);
}

// SPT (SHORTEST PRODUCE TIME) FUNCTION
JobType* FindJobBySPT(int machine, JobType* firstJob, float currentTime) 
{
    float minTPT = MAXF;
    JobType* selectedJob = nullptr;

    JobType* job = firstJob;
    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->TPT < minTPT) 
            {
                minTPT = job->TPT;
                selectedJob = job;
            }
        }
        job = job->Next;
    }

    return selectedJob;
}

// EDD (Earliest Due Date) FUNCTION
JobType* FindJobByEDD(int machine, JobType* firstJob, float currentTime) 
{
    float minDueDate = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;
    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->d < minDueDate) 
            {
                minDueDate = job->d;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// LIFO (Last In First Out) FUNCTION
JobType* FindJobByLIFO(int machine, JobType* firstJob, float currentTime) 
{
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;
    long maxJobNo = 0;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->No > maxJobNo) 
            {
                maxJobNo = job->No;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// LPT (Longest Processing Time) FUNCTION
JobType* FindJobByLPT(int machine, JobType* firstJob, float currentTime) 
{
    float maxProcTime = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->TPT > maxProcTime) 
            {
                maxProcTime = job->TPT;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// STPT (Shortest Total Processing Time) FUNCTION
JobType* FindJobBySTTP(int machine, JobType* firstJob, float currentTime) 
{
    float minTotalProcTime = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float totalTime = job->TPT;
            if (totalTime < minTotalProcTime) 
            {
                minTotalProcTime = totalTime;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// LDR (Largest Dynamic Ratio) FUNCTION
JobType* FindJobByLDR(int machine, JobType* firstJob, float currentTime) 
{
    float maxRatio = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float ratio = job->TPT / job->rTPT;
            if (ratio > maxRatio) 
            {
                maxRatio = ratio;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// LDT (Largest Dynamic Total) FUNCTION
JobType* FindJobByLDT(int machine, JobType* firstJob, float currentTime) 
{
    float maxRatio = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float ratio = job->TPT / job->TPTOR;
            if (ratio > maxRatio) 
            {
                maxRatio = ratio;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// SDT (Smallest Dynamic Total) FUNCTION
JobType* FindJobBySDT(int machine, JobType* firstJob, float currentTime) 
{
    float minRatio = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float ratio = job->TPT / job->TPTOR;
            if (ratio < minRatio) 
            {
                minRatio = ratio;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// SDR (Smallest Dynamic Ratio) FUNCTION
JobType* FindJobBySDR(int machine, JobType* firstJob, float currentTime) 
{
    float minRatio = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float ratio = job->TPT / job->rTPT;
            if (ratio < minRatio) 
            {
                minRatio = ratio;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// PT+WINQ (Process Time + Waiting in Queue) FUNCTION
JobType* FindJobByPTWINQ(int machine, JobType* firstJob, float currentTime)
{
    float minSum = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float sum = job->Pt + Mac[machine].WIQ;
            if (sum < minSum) 
            {
                minSum = sum;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// NINQ (Next In Queue) FUNCTION
JobType* FindJobByNINQ(int machine, JobType* firstJob, float currentTime)
{
    int minQueueLength = INT_MAX;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            int nextMachine = job->M[job->cNOP + 1];
            int queueLength = Mac[nextMachine].JIQ;
            if (queueLength < minQueueLength) 
            {
                minQueueLength = queueLength;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}


// LTWR (Least Time to Wait in Queue) FUNCTION
JobType* FindJobByLTWR(int machine, JobType* firstJob, float currentTime)
{
    float minWork = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->rTPT < minWork) 
            {
                minWork = job->rTPT;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// MTWR (Most Time to Wait in Queue) FUNCTION
JobType* FindJobByMTWR(int machine, JobType* firstJob, float currentTime)
{
    float maxWork = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->rTPT > maxWork) 
            {
                maxWork = job->rTPT;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// SMR (Smallest Mean Ratio) FUNCTION
JobType* FindJobBySMR(int machine, JobType* firstJob, float currentTime)
{
    float minProduct = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float product = job->Pt * job->rTPT;
            if (product < minProduct) 
            {
                minProduct = product;
                selectedJob = job;
            }
        }
        job = job->Next;
    }
    return selectedJob;
}

// SIO (Shortest Imminent Operation) FUNCTION
JobType* FindJobBySIO(int machine, JobType* firstJob, float currentTime) 
{
    float minTPT = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->TPT < minTPT) 
            {
                minTPT = job->TPT;
                selectedJob = job;
            }
        }
        job = job->Next;
    }

    return selectedJob;
}

// LMR (Largest (operation time * total operation time)) FUNCTION
JobType* FindJobByLMR(int machine, JobType* firstJob, float currentTime) 
{
    float maxValue = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float value = job->TPT * job->TPTOR;  // totalTime yerine TPTOR kullanıldı
            if (value > maxValue) 
            {
                maxValue = value;
                selectedJob = job;
            }
        }
        job = job->Next;
    }

    return selectedJob;
}

// LMT (Largest (operation time * total processing time)) FUNCTION
JobType* FindJobByLMT(int machine, JobType* firstJob, float currentTime) 
{
    float maxValue = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float value = job->TPT * job->TPTOR;  // totalTime yerine TPTOR kullanıldı
            if (value > maxValue) 
            {
                maxValue = value;
                selectedJob = job;
            }
        }
        job = job->Next;
    }

    return selectedJob;
}

// LTPT (Longest Total Processing Time) FUNCTION
JobType* FindJobByLTPT(int machine, JobType* firstJob, float currentTime) 
{
    float maxTotalTime = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            if (job->TPTOR > maxTotalTime)  // totalTime yerine TPTOR kullanıldı
            {
                maxTotalTime = job->TPTOR;
                selectedJob = job;
            }
        }
        job = job->Next;
    }

    return selectedJob;
}

// LRO (Largest Unassigned Operation Count) FUNCTION
JobType* FindJobByLRO(int machine, JobType* firstJob, float currentTime) 
{
    int maxUnassigned = -1;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            int unassignedCount = 0;
            for (int i = 0; i <= job->NOP; i++) 
            {
                if (job->M[i] == 0) // Örneğin, işin bu aşama atanmamışsa
                {
                    unassignedCount++;
                }
            }

            if (unassignedCount > maxUnassigned) 
            {
                maxUnassigned = unassignedCount;
                selectedJob = job;
            }
        }
        job = job->Next;
    }

    return selectedJob;
}

// SMT (Smallest (operation time * total operation time)) FUNCTION
JobType* FindJobBySMT(int machine, JobType* firstJob, float currentTime) 
{
    float minValue = MAXF;
    JobType* selectedJob = nullptr;
    JobType* job = firstJob;

    while (job != nullptr) 
    {
        if (job->Mt == machine && job->ets <= currentTime) 
        {
            float value = job->TPT * job->TPTOR;  // totalTime yerine TPTOR kullanıldı
            if (value < minValue) 
            {
                minValue = value;
                selectedJob = job;
            }
        }
        job = job->Next;
    }

    return selectedJob;
}



// DEPARTING
void Depart(long J, int M, SchedulingRule rule)
{
  // Find departing job
  Job = First;
  while (Job->No != J || Job->Mt != M)
    Job = Job->Next;

  Mac[M].Status = IDLE;
  Mac[M].IdleTime = t;
  Mac[M].TUTime += ( t - Mac[M].BusyTime);
  Mac[M].FreeTime = t;
  Job->cNOP++;
  Job->rNOP--;
  Job->rTPT = Job->TPT - Job->P[Job->cNOP];
  Job->cTPT = Job->TPT - Job->rTPT;
  // Job not completed
  if (Job->cNOP < Job->NOP)
  {
    Job->Mt = Job->M[Job->cNOP + 1];
    Job->Pt = Job->P[Job->cNOP + 1];
    if (Mac[Job->Mt].Status == IDLE)
    {
      Mac[Job->Mt].TITime += (t - Mac[Job->Mt].IdleTime);
      Mac[Job->Mt].Status = BUSY;
      Mac[Job->Mt].BusyTime = t;
      Job->ets = t;
      Job->etf = Job->ets + Job->Pt;
      Mac[Job->Mt].FreeTime = Job->etf;

      Update(Job->Mt, Job->No);

    }
    else
    {
      Job->ets = Mac[Job->Mt].FreeTime;
      Job->EW[Job->Mt] = Job->ets - t;
      Mac[Job->Mt].EW += Job->EW[Job->Mt];
      Job->etf = MAXF;
      Mac[Job->Mt].JIQ++;
      Mac[Job->Mt].TIQ++;
      Mac[Job->Mt].WIQ += Job->Pt;
    }
  }
  // Job completed
  else
  {
    NJobsComp++;
    DelJob = Job->No;
    if (NJobsComp > NWarmUp)
    {
      Job->C = t;
      Job->F = Job->C - Job->r;
      TotalF += Job->F;
      Job->L = Job->C - Job->d;
      TAD += fabs(Job->L);
      TSE += pow( fabs(Job->L),2 );
      if (Job->L > 0)
      {
        Job->T = Job->L;
        Job->E = 0;
        TotalT += Job->T;
        NJobsTard++;
      }
      else
      {
        Job->T = 0;
        if (Job->L == 0) Job->E = 0;
        else Job->E = - Job->L;
        TotalE += Job->E;
        NJobsEar++;
       }
      WriteInfoToFile();
    }
    DisplayInfo();


    // Deleting job
    Job = First;
    Pred = 0;
    while (Job->No != 0 && Job->No != DelJob)
    {
      Pred = Job;
      Job = Job->Next;
    }
    if (Pred == 0)
    {
      Temp = First;
      First = Temp->Next;
    }
    else
    {
      Temp = Pred->Next;
      Pred->Next = Temp->Next;
    }
    delete Temp;
    NJobsInShop--;
  }

  // 20 FUNCTIONS FOR JOB SELECTION
  JobType* selectedJob = nullptr;
    switch (rule) 
    {
        case SPT:
            selectedJob = FindJobBySPT(M, First, t);
            break;
        case EDD:
            selectedJob = FindJobByEDD(M, First, t);
            break;
        case LIFO:
            selectedJob = FindJobByLIFO(M, First, t);
            break;
        case LPT:
            selectedJob = FindJobByLPT(M, First, t);
            break;
        case STPT:
            selectedJob = FindJobBySTTP(M, First, t);
            break;
        case LDR:
            selectedJob = FindJobByLDR(M, First, t);
            break;
        case LDT:
            selectedJob = FindJobByLDT(M, First, t);
            break;
        case SDT:
            selectedJob = FindJobBySDT(M, First, t);
            break;
        case SDR:
            selectedJob = FindJobBySDR(M, First, t);
            break;
        case PTWINQ:
            selectedJob = FindJobByPTWINQ(M, First, t);
            break;
        case NINQ:
            selectedJob = FindJobByNINQ(M, First, t);
            break;
        case LTWR:
            selectedJob = FindJobByLTWR(M, First, t);
            break;
        case MTWR:
            selectedJob = FindJobByMTWR(M, First, t);
            break;
        case SMR:
            selectedJob = FindJobBySMR(M, First, t);
            break;
        case SIO:
            selectedJob = FindJobBySIO(M, First, t);
            break;
        case LMR:
            selectedJob = FindJobByLMR(M, First, t);
            break;
        case LMT:
            selectedJob = FindJobByLMT(M, First, t);
            break;
        case LTPT:
            selectedJob = FindJobByLTPT(M, First, t);
            break;
        case LRO:
            selectedJob = FindJobByLRO(M, First, t);
            break;
        case SMT:
            selectedJob = FindJobBySMT(M, First, t);
            break;
    }

  if (selectedJob != nullptr)
  {
    Mac[selectedJob->Mt].JIQ--;
    Mac[selectedJob->Mt].WIQ -= selectedJob->Pt;
    Mac[selectedJob->Mt].EW -= selectedJob->EW[selectedJob->Mt];
    Mac[selectedJob->Mt].TITime += (t - Mac[selectedJob->Mt].IdleTime);
    Mac[selectedJob->Mt].Status = BUSY;
    Mac[selectedJob->Mt].BusyTime = t;
    selectedJob->ets = t;
    selectedJob->etf = selectedJob->ets + selectedJob->Pt;
    Mac[selectedJob->Mt].FreeTime = selectedJob->etf;

    Update(selectedJob->Mt, selectedJob->No);

  }
  FindNextDep();
}



// MAIN PROGRAM

#include <stdio.h>
#include <io.h>
#include <math.h>
#include <stdlib.h>
#include <iomanip>
#include <conio.h>
#include <iostream>
#include <string.h>

// PSO kütüphanesi ve Eigen kütüphanesi vs. dahil edilmeleri (kodunuzun diğer kısımları)
// ... (kodunuzun diğer bölümleri burada yer alıyor)

// Global değişkenler, yapılar ve fonksiyon prototipleri burada tanımlı...
// (Initialize, Arrive, Depart, WriteFinalResultsToFile, ClearList, vs.)

int main()
{
    // Kullanıcıdan sıralama kuralını alıyoruz
    SchedulingRule selectedRule = GetUserSchedulingRule();

    char FN[10][8], FileName[8], x[8];

    system("cls");
    printf("Output file name: ");
    scanf("%s", FileName);

    char FName[12];
    strcpy(FName, FileName);
    strcat(FName, ".dat");
    File = fopen(FName, "w");
    // İlk satıra başlık yazıyoruz (metrik isimlerini kendi çıktınıza göre ayarlayın)
    fprintf(File, " Sim#       MAD       MSE      AvgT        nT      AvgE        nE      AvgF       JIS\n");

    NJobsDesired = 15000;

    // 10 benzetim çalışması yapılıyor
    for (int N = 0; N < 10; N++)
    {
        PRIME = PrimeList[N];
        SEED = 0.645329;

        printf("\nSim. run #%d\n\n", (N + 1));
        fprintf(File, "%5d", (N + 1));

        //-------------------------------------------------
        Initialize();

        I = N;
        strcpy(x, FileName);

        char s[8];
        itoa(I, s, 10);
        strcat(x, s);
        strcpy(FN[I], x);
        F[I] = fopen(FN[I], "w");

        CreateArrTime();
        NextArrTime = ArrTime;
        while (NJobsComp < NJobsDesired)
        {
            WhatIsNextEvent();
            switch (NextEvent)
            {
                case ARRIVAL:
                    Arrive();
                    break;
                case DEPART:
                    Depart(NextDep.JNo, NextDep.MNo, selectedRule);
                    break;
            }
        }
        WriteFinalResultsToFile(N);
        ClearList();
        //-------------------------------------------------
        fclose(F[I]);
    }
    fclose(File);

    // Geçici dosyalardan okuma yapabilmek için dosya isimlerini yeniden oluşturuyoruz
    for (I = 0; I < 10; I++)
    {
        strcpy(x, FileName);
        char s[8];
        itoa(I, s, 10);
        strcat(x, s);
        strcpy(FN[I], x);
    }

    // Simülasyonlar arası ortalama ve standart sapmayı hesaplayacağımız dosyayı açıyoruz
    FF = fopen(FileName, "w");

    // Örneğin; 5 metrik olduğunu varsayıyoruz (kendi çıktı dosyanıza göre "numMetrics" ayarlanmalı)
    int numMetrics = 8;
    // Tüm 10 geçici dosyayı okuma modunda açıyoruz
    for (I = 0; I < 10; I++)
        F[I] = fopen(FN[I], "r");

    // Dosyalardan okunan her satır için (her benzetimde aynı sıra ile metrikler yazılmış olmalı)
    while (!feof(F[0]))
    {
        for (int j = 0; j < numMetrics; j++)
        {
            float sum = 0.0, sumSq = 0.0, value;
            // 10 benzetimden okunan j'inci metrik değeri üzerinden hesaplama yapıyoruz
            for (int k = 0; k < 10; k++)
            {
                fscanf(F[k], "%f", &value);
                sum += value;
                sumSq += value * value;
            }
            float mean = sum / 10.0;
            float variance = (sumSq / 10.0) - (mean * mean);
            float stdev = sqrt(variance);
            // Hem ortalamayı hem de standart sapmayı dosyaya yazdırıyoruz
            fprintf(FF, "Mean: %10.2f  SD: %10.2f  ", mean, stdev);
        }
        fprintf(FF, "\n");
        // Her dosyadan bir satır atlamak için (satır sonu okuma)
        for (int k = 0; k < 10; k++)
            fscanf(F[k], "\n");
    }

    // Açılan tüm dosyaları kapatıyoruz
    for (I = 0; I < 10; I++)
        fclose(F[I]);
    fclose(FF);
    // Geçici dosyaları siliyoruz
    for (I = 0; I < 10; I++)
        remove(FN[I]);

    printf("\n\nPress any key...");
    getch();
    return 0;
}

