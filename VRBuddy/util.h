#pragma once
namespace util
{
	extern int global_seed;
	int RandomInt(int max, int min);
	double RandomDouble(double max, double min);
	void SeedGenerator(int seed);
	int Round(double d);
	int To1DArray(int x, int y, int width);
}; 
