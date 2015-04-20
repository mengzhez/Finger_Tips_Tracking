#include <cv.h>
#include <highgui.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// Class
class movable_image
{
public:
	// Position & Size
	int x;
	int y;
	int width;
	int height;

	// image content
	Mat image;

	movable_image()
	{
	}

	~movable_image()
	{
	}

	// Highlight the image with yellow rectangle
	void highlight(Mat &input)
	{
		Point pt1;
		pt1.x = x;
		pt1.y = y;

		Point pt2;
		pt2.x = x + width;
		pt2.y = y + height;

		rectangle(input, pt1, pt2, CV_RGB(255, 255, 0), 3, 8, 0);
	}
};

// Global variables
bool move_switch = false;
bool dist_switch = true;
int dist_x = 0;
int dist_y = 0;
const int image_num = 4;

// Movable image used for demo
movable_image images[image_num];

// Functions
void init();
void find_skin(Mat &input, Mat &output);
void find_contour(Mat &input, vector<Point> &contour);
void find_finger_tips(Mat &input, Mat &output, vector<Point> contour, Point center);
void display_pointer(Mat &input, int x, int y);
void move_pic(Mat &input, Point finger_tip_max);


void init()
{
	// Read images and set the initial position
	images[0].x = 50;
	images[0].y = 50;
	images[0].image = imread("images/isu_1.png");

	images[1].x = 400;
	images[1].y = 100;
	images[1].image = imread("images/isu_2.png");

	images[2].x = 100;
	images[2].y = 200;
	images[2].image = imread("images/isu_3.png");

	images[3].x = 350;
	images[3].y = 230;
	images[3].image = imread("images/isu_4.png");

	for (int i = 0; i < image_num; i++)
	{
		images[i].width = images[i].image.cols;
		images[i].height = images[i].image.rows;
	}
}

void find_skin(Mat &input, Mat &output)
{
	// Apply Gaussian filter
	GaussianBlur(input, input, Size(3, 3), 0);

	Mat YCbCr;
	vector<Mat> planes;

	// Convert to YCrCb space
	cvtColor(input, YCbCr, CV_RGB2YCrCb);

	// Split
	split(YCbCr, planes);

	// Inialize iterator
	MatIterator_<uchar> it_Cb = planes[1].begin<uchar>();
	MatIterator_<uchar>	it_Cb_end = planes[1].end<uchar>();
	MatIterator_<uchar> it_Cr = planes[2].begin<uchar>();
	MatIterator_<uchar> it_skin = output.begin<uchar>();

	// Find the color of skin
	for (; it_Cb != it_Cb_end; ++it_Cr, ++it_Cb, ++it_skin)
	{
		if (138 <= *it_Cr &&  *it_Cr <= 170 && 100 <= *it_Cb &&  *it_Cb <= 127)
			*it_skin = 255;
		else
			*it_skin = 0;
	}

	//morphologyEx(output, output, cv::MORPH_OPEN , Mat());

	// Dilate and erode
	dilate(output, output, Mat(5, 5, CV_8UC1), Point(-1, -1));
	erode(output, output, Mat(5, 5, CV_8UC1), Point(-1, -1));

	GaussianBlur(output, output, Size(3, 3), 0);
}

void find_contour(Mat &input, vector<Point> &contour)
{
	// Find contour of hand
	int index = 0;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(input, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	if (contours.size())
	{
		// Find the biggest contour
		double area;
		double area_max = 0;
		for (int i = 0; i < contours.size(); i++)
		{
			// Compute the area in the contour
			area = contourArea(Mat(contours[i]));
			if (area > area_max)
			{
				area_max = area;
				index = i;
			}
		}

		contour = contours[index];

		// Find the approximating polygon
		if (contour.size())
		{
			approxPolyDP(contour, contour, 10, true);
		}
	}
}

// Find finger tips
void find_finger_tips(Mat &input, Mat &output, vector<Point> contour, Point center)
{
	vector<Point> finger_tips;
	Point finger_tip_max;
	int dist_prev2 = 0;
	int dist_prev1 = 0;
	int dist_current = 0;
	int dist_max = 0;

	for (int i = 0; i < contour.size(); i++)
	{
		Point point_current = contour[i];

		dist_current = double((point_current.x - center.x) * (point_current.x - center.x) + (point_current.y - center.y) * (point_current.y - center.y));

		if (dist_current<dist_prev1 && dist_prev1>dist_prev2 && dist_prev1<(250 * 250) && dist_prev1>(150 * 150))
		{
			Point finger_tip = contour[i - 1];

			// Finger tips should not on the boundary of the image
			if (finger_tip.x > 30 && finger_tip.x < (input.cols - 30) && finger_tip.y > 30 && finger_tip.y < (input.rows - 30))
			{
				finger_tips.push_back(contour[i - 1]);

				// Find the finger which has max distance
				if (dist_current > dist_max)
				{
					finger_tip_max = finger_tip;
					dist_max = dist_current;
				}

				// Ensure 5 fingers totally
				if (finger_tips.size() == 5)
				{
					break;
				}
			}
		}

		dist_prev2 = dist_prev1;
		dist_prev1 = dist_current;
	}

	for (int i = 0; i < finger_tips.size(); i++)
	{
		circle(output, finger_tips[i], 6, Scalar(0, 255, 0), CV_FILLED);
	}

	// Display the pointer which denotes the finger tip with longest distance
	//display_pointer(output, finger_tip_max.x, finger_tip_max.y);
	//move_pic(output, finger_tip_max);
}

void display_pointer(Mat &input, int x, int y)
{
	Mat pointer = imread("images/pointer.png");

	if (x + pointer.cols > input.cols)
	{
		x = input.cols - pointer.cols;
	}

	if (y + pointer.rows > input.rows)
	{
		y = input.rows - pointer.rows;
	}

	Mat imageROI = input(Rect(x, y, pointer.cols, pointer.rows));
	pointer.copyTo(imageROI, pointer);
}

void move_pic(Mat &input, Point finger_tip_max)
{
	for (int i = 0; i<image_num; i++)
	{
		int x = 0;
		int y = 0;

		if (finger_tip_max.x>images[i].x && finger_tip_max.x<(images[i].x + images[i].width) && finger_tip_max.y>images[i].y && finger_tip_max.y < (images[i].y + images[i].height))
		{
			if (move_switch)
			{
				if (dist_switch)
				{
					dist_x = finger_tip_max.x - images[i].x;
					dist_y = finger_tip_max.y - images[i].y;
					dist_switch = false;
				}

				images[i].x = finger_tip_max.x - dist_x;
				images[i].y = finger_tip_max.y - dist_y;

				dist_x = finger_tip_max.x - images[i].x;
				dist_y = finger_tip_max.y - images[i].y;
			}

			images[i].highlight(input);
		}

		if (images[i].x + images[i].width > input.cols)
		{
			images[i].x = input.cols - images[i].width;
		}

		if (images[i].y + images[i].height > input.rows)
		{
			images[i].y = input.rows - images[i].height;
		}

		Mat imageROI = input(Rect(images[i].x, images[i].y, images[i].width, images[i].height));
		Mat mask(images[i].width, images[i].height, images[i].image.type());
		images[i].image.copyTo(imageROI, mask);
	}
}

int main(int argc, char* argv[])
{
	VideoCapture capture;
	Mat frame;
	Mat skin;
	Mat frame_final;
	Mat test;
	init();

	capture.open(0);
	if (!capture.isOpened())
	{
		cout << "Camera is not opened!\n" << endl;
		return -1;
	}

	while (1)
	{
		capture >> frame;
		flip(frame, frame, 1);
		frame_final = frame;

		if (frame.empty())
			break;

		// Find the hand according to skin area
		skin.create(frame.rows, frame.cols, CV_8UC1);
		find_skin(frame, skin);
		test = skin;

		// Find the center of hand
		Moments moment = moments(skin, true);
		Point center(moment.m10 / moment.m00, moment.m01 / moment.m00);
		circle(frame, center, 8, Scalar(0, 0, 255), CV_FILLED);

		// Find hand contour
		vector<Point> contour;
		find_contour(skin, contour);

		if (contour.size())
		{
			for (int i = 1; i < contour.size(); i++)
			{
				line(frame_final, contour[i], contour[i - 1], (0, 0, 255), 3, 8, 0);
			}

			// Find the finger tips
			find_finger_tips(frame, frame_final, contour, center);
		}

		imshow("show_img1", frame_final);
		imshow("show_img2", test);

		char k = cvWaitKey(20);
		if (k == 27)
		{
			// ESC quit
			break;
		}
		else if (k == 'q')
		{
			if (move_switch)
			{
				// Close move switch
				move_switch = false;
				dist_switch = true;
			}
			else
			{
				// Open move switch
				move_switch = true;
				dist_switch = true;
			}
		}

	}

	return 0;
}
