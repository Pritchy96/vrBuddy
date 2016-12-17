// ----------------------------------------------------------
// Includes 
// ----------------------------------------------------------
#include "fractal_creator.h"
#include "util.h"
#include <vector>
#include <cstdlib>
#include <math.h>
using namespace std;


// ----------------------------------------------------------
// Global Variables
// ----------------------------------------------------------
double screenSize = 0;   //Width + Height of screen.


vector<vector<float>> fractal_creator::MakeFractal(int width, int height, int Roughness, int maxValue, int minValue)
{
	screenSize = width + height;
	double maxDistance = 0, centerX = width / 2, centerY = height / 2;
	maxDistance = sqrt((pow(centerX, 2)) + (pow(centerY, 2)));

	vector<vector<float>>  map = vector<vector<float>>();
	//Resizing array and setting all values to 0.
	map.resize(width, vector<float>(height, 0));

	//Calculate corner values (c1, c2, c3, c4).
	double c1 = util::RandomDouble(1, 0);
	double c2 = util::RandomDouble(1, 0);
	double c3 = util::RandomDouble(1, 0);
	double c4 = util::RandomDouble(1, 0);

	//Call Divide, begin the iteration.
	Divide(&map, 0, 0, width, height, Roughness, c1, c2, c3, c4, minValue, maxValue);

	return map;
}

/// <summary>
/// </summary>
/// <param name="colMapRgbValues"> Array of BGR Bytes </param>
/// <param name="bitmapWidth"> Width of the map image </param>
/// <param name="x"> X Pos </param>
/// <param name="y"> Y Pos </param>
/// <param name="width"> Width of current rectangle being worked on. </param>
/// <param name="height"> Height of current rectangle being worked on. </param>
/// <param name="c1"></param>
/// <param name="c2"></param>
/// <param name="c3"></param>
/// <param name="c4"></param>
void fractal_creator::Divide(vector<vector<float>> *mapRgbValues, double x, double y, double width, double height, int roughness, double c1, double c2, double c3, double c4, int minValue, int maxValue)
{
	//X and Y are the old c1 coordinates from the last recursive iteration.

	double middle, mid1, mid2, mid3, mid4;

	//calculate width and hight of new rectangle by halving the last.
	double newWidth = width / 2;
	double newHeight = height / 2;

	//If our rectangles are bigger than 1px x 1px.
	if (width > 1 || height > 1)
	{
		//Square Step.
		//Calculate middle Point by averaging corners and then adding a random displacement.
		middle = Round(((c1 + c2 + c3 + c4) / 4 + Displace(newWidth + newHeight, roughness)));

		//Diamond Step.
		//Calculating the edge points in order for the 4 points of each rectangle to all have values.
		//this is just the average of the two points it bisects.
		mid1 = Round((c1 + c2) / 2);
		mid2 = Round((c1 + c3) / 2);
		mid3 = Round((c2 + c4) / 2);
		mid4 = Round((c3 + c4) / 2);

		//Call divide to calculate the middle of the new rectangles.
		Divide(mapRgbValues, x, y, newWidth, newHeight, roughness, c1, mid1, mid2, middle, minValue, maxValue);
		Divide(mapRgbValues, x + newWidth, y, width - newWidth, newHeight, roughness, mid1, c2, middle, mid3, minValue, maxValue);
		Divide(mapRgbValues, x, y + newHeight, newWidth, height - newHeight, roughness, mid2, middle, c3, mid4, minValue, maxValue);
		Divide(mapRgbValues, x + newWidth, y + newHeight, width - newWidth, height - newHeight, roughness, middle, mid3, mid4, c4, minValue, maxValue);
	}
	//If our rectangles are now 1px x 1px, we are ready to calculate final values and draw.
	else
	{
		//Average the points of the pixel sized rectangle down into a single number, which is that pixels final gradientValue.
		double heightValue = (c1 + c2 + c3 + c4) / 4;   //Height value generated from random plasma noise.
														//Setting the final value for this pixel.+
		(*mapRgbValues)[util::Round(x)][util::Round(y)] = ((heightValue * (maxValue - minValue)) - minValue);
	}
}

//Makes sure values stay within limits.
double fractal_creator::Round(double num)
{
	if (num > 1)
		return 1;
	else if (num < 0)
		return 0;
	else
		return num;
}

//Displaces gradientValue a small amount.
double fractal_creator::Displace(double rectSize, int roughness)
{
	double Max = rectSize / screenSize * roughness;
	return (util::RandomDouble(1, 0) - 0.5) * Max;
}
