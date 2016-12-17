#pragma once
#include <vector>
using namespace std;

class fractal_creator
{
public:
	static double fractal_creator::Displace(double rectSize, int roughness);
	static double fractal_creator::Round(double num);
	static void fractal_creator::Divide(std::vector<std::vector<float>> *mapRgbValues, double x, double y, double width, double height, int roughness, double c1, double c2, double c3, double c4, int minValue, int maxValue);
	static vector<vector<float>> fractal_creator::MakeFractal(int width, int height, int Roughness = 13, int maxValue = 255, int minValue = 0);

};
