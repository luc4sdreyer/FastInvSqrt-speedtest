#include <stdio.h>
#include <math.h>
#include "time.h"
#include <algorithm>
#include <vector>
#include <xmmintrin.h>
#include <windows.h>

/// Create a Timer, which will immediately begin counting
/// up from 0.0 seconds.
/// You can call reset() to make it start over.
/// Source: http://www.mindcontrol.org/~hplus/misc/simple-timer.html
class Timer {
	public:
		Timer() {
			reset();
		}
		
	/// reset() makes the timer start over counting from 0.0 seconds.
	void reset() {
		unsigned __int64 pf;
		QueryPerformanceFrequency( (LARGE_INTEGER *)&pf );
		freq_ = 1.0 / (double)pf;
		QueryPerformanceCounter( (LARGE_INTEGER *)&baseTime_ );
	}
	
	/// seconds() returns the number of seconds (to very high resolution)
	/// elapsed since the timer was last created or reset().
	double seconds() {
		unsigned __int64 val;
		QueryPerformanceCounter( (LARGE_INTEGER *)&val );
		return (val - baseTime_) * freq_;
	}
	
	/// seconds() returns the number of milliseconds (to very high resolution)
	/// elapsed since the timer was last created or reset().
	double milliseconds() {
		return seconds() * 1000.0;
	}
	
	private:
	double freq_;
	unsigned __int64 baseTime_;
};

/// The brilliant hack to compute the Fast inverse square root
/// https://en.wikipedia.org/wiki/Fast_inverse_square_root
inline float FastInvSqrt(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x; 
	i = 0x5f3759df - (i >> 1);
	x = *(float*)&i;
	x = x*(1.5f-(xhalf*x*x));
	return x;
}

/// SSE operation that also approximates the inverse square root
inline void ssesqrt_recip( float * pout, float * pin )
{
   __m128 in = _mm_load_ss( pin );
   _mm_store_ss( pout, _mm_rsqrt_ss( in ) );
}

#define SIZE 4096
#define ITERATIONS 8192
int main() {
	float times[50];
	int loops = 20;
	
	float floatIn[SIZE]; 
	float floatOut[SIZE]; 
	
	time_t seconds;
	time(&seconds);
	srand((unsigned int) seconds);
	Timer t;
	double a = t.milliseconds();
	
	//rerun the whole loop 20 times and take the shortest time
	for (int j = 0; j < loops; ++j) {
		for (int i = 0; i < SIZE; ++i) {
			floatIn[i] = rand() % 100 + 0.01;
			floatOut[i] = 0;
		}
		double a = t.milliseconds();
		for (int k = 0; k < ITERATIONS; ++k) {
			for (int i = 0; i < SIZE; ++i) {
				floatOut[i] = FastInvSqrt(floatIn[i]);
				//ssesqrt_recip(&(floatOut[i]),&(floatIn[i]));	
				//floatOut[i] = 1/sqrt(floatIn[i]);
			}
		}
		double b = t.milliseconds();
		times[j] = ((float)b - (float)a) / 1000.0F;
	}
	std::vector<float> vTimes2 (times, times+loops); 
	std::sort (vTimes2.begin(), vTimes2.begin()+loops);
	float rmse = 0;
	for (int i = 0; i < SIZE; ++i) {
		float o = 1/sqrt(floatIn[i]);
		rmse += pow(1/sqrt(floatIn[i]) - floatOut[i],2);
	}
	rmse = sqrt(rmse/SIZE);
	printf("%d calculations in %fs, %fns per calculation, RMSE: %f\r\n", (SIZE*ITERATIONS),vTimes2[0],vTimes2[0]*1000000000/(SIZE*ITERATIONS),rmse);
	
	//check if the three functions are within 1% of each other
	for (int i = 0; i < SIZE; ++i) {
		float f1 = FastInvSqrt(floatIn[i]);
		float f2;
		ssesqrt_recip(&f2,&(floatIn[i]));
		float f3 = 1/sqrt(floatIn[i]);
		if ((std::max(f1,f2)/std::min(f1,f2)) > 1.01 || (std::max(f1,f3)/std::min(f1,f3)) > 1.01) {
			printf("%d\t%f\t%f\t%f\t%f\r\n", i, floatIn[i],f1,f2,f3);
			break;
		}
	}
}

