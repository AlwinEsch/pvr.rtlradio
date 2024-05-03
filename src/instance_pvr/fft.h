// fft.h: interface for the CFft class.
//
//  This is a somewhat modified version of Takuya OOURA's
//     original radix 4 FFT package.
// A C++ wrapper around his code plus some specialized methods
// for displaying power vs frequency
//
//Copyright(C) 1996-1998 Takuya OOURA
//    (email: ooura@mmm.t.u-tokyo.ac.jp).
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef FFT_H
#define FFT_H

//#include "datatypes.h"

#include <mutex>

#define MAX_FFT_SIZE 65536
#define MIN_FFT_SIZE 512

typedef float tSReal;

typedef struct _sCplx
{
  tSReal re;
  tSReal im;
} tSComplex;

#define TYPEREAL tSReal
#define TYPECPX tSComplex

#define MSIN(x) sinf(x)
#define MCOS(x) cosf(x)
#define MPOW(x, y) powf(x, y)
#define MEXP(x) expf(x)
#define MFABS(x) fabsf(x)
#define MLOG(x) logf(x)
#define MLOG10(x) log10f(x)
#define MSQRT(x) sqrtf(x)
#define MATAN(x) atanf(x)
#define MFMOD(x, y) fmodf(x, y)
#define MATAN2(x, y) atan2f(x, y)

#define K_2PI (2.0 * 3.14159265358979323846)
#define K_PI (3.14159265358979323846)
#define K_PI4 (K_PI / 4.0)
#define K_PI2 (K_PI / 2.0)
#define K_3PI4 (3.0 * K_PI4)

class CFft
{
public:
  CFft();
  virtual ~CFft();
  void SetFFTParams(int32_t size, bool invert, TYPEREAL dBCompensation, TYPEREAL SampleFreq);
  //Methods to obtain spectrum formated power vs frequency
  void SetFFTAve(int32_t ave);
  void ResetFFT();
  bool GetScreenIntegerFFTData(int32_t MaxHeight,
                               int32_t MaxWidth,
                               TYPEREAL MaxdB,
                               TYPEREAL MindB,
                               int32_t StartFreq,
                               int32_t StopFreq,
                               int32_t* OutBuf);
  int32_t PutInDisplayFFT(int32_t n, TYPECPX* InBuf);

  //Methods for doing Fast convolutions using forward and reverse FFT
  void FwdFFT(TYPECPX* pInOutBuf);
  void RevFFT(TYPECPX* pInOutBuf);

private:
  void FreeMemory();
  void makewt(int32_t nw, int32_t* ip, TYPEREAL* w);
  void makect(int32_t nc, int32_t* ip, TYPEREAL* c);
  void bitrv2(int32_t n, int32_t* ip, TYPEREAL* a);
  void cftfsub(int32_t n, TYPEREAL* a, TYPEREAL* w);
  void rftfsub(int32_t n, TYPEREAL* a, int32_t nc, TYPEREAL* c);
  void CpxFFT(int32_t n, TYPEREAL* a, TYPEREAL* w);
  void cft1st(int32_t n, TYPEREAL* a, TYPEREAL* w);
  void cftmdl(int32_t n, int32_t l, TYPEREAL* a, TYPEREAL* w);
  void bitrv2conj(int n, int* ip, TYPEREAL* a);
  void cftbsub(int n, TYPEREAL* a, TYPEREAL* w);

  bool m_Overload;
  bool m_Invert;
  int32_t m_AveCount;
  int32_t m_TotalCount;
  int32_t m_FFTSize;
  int32_t m_LastFFTSize;
  int32_t m_AveSize;
  int32_t m_StartFreq;
  int32_t m_StopFreq;
  int32_t m_BinMin;
  int32_t m_BinMax;
  int32_t m_PlotWidth;

  TYPEREAL m_K_C;
  TYPEREAL m_K_B;
  TYPEREAL m_dBCompensation;
  TYPEREAL m_SampleFreq;
  int32_t* m_pWorkArea;
  int32_t* m_pTranslateTbl;
  TYPEREAL* m_pSinCosTbl;
  TYPEREAL* m_pWindowTbl;
  TYPEREAL* m_pFFTPwrAveBuf;
  TYPEREAL* m_pFFTAveBuf;
  TYPEREAL* m_pFFTSumBuf;
  TYPEREAL* m_pFFTInBuf;
#ifdef FMDSP_THREAD_SAFE
  mutable std::mutex m_Mutex; //for keeping threads from stomping on each other
#endif
};

#endif // FFT_H
