#include "opencv2/core/core.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <time.h>

using namespace cv;
using namespace std;

static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';')
{
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file) {
		string error_message = "No valid input file was given, please check the given filename.";
		CV_Error(CV_StsBadArg, error_message);
	}
	string line, path, classlabel;
	while (getline(file, line)) {
		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, classlabel);
		if (!path.empty() && !classlabel.empty()) {
			images.push_back(imread(path, 0));
			labels.push_back(atoi(classlabel.c_str()));
		}
	}
}

int faceRecognition(string pathToRecognizedImage, int originalID)
{
	string fn_csv = "csv.ext";
	vector<Mat> images;
	vector<int> labels;

	try {
		read_csv(fn_csv, images, labels);
	}
	catch (cv::Exception& e) {
		cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
	}

	if (images.size() <= 1)
	{
		string error_message = "This demo needs at least 2 images to work. Please add more images to your data set!";
		CV_Error(CV_StsError, error_message);
	}
	int height = images[0].rows;

	/*Mat testSample = images[images.size() - 1];
	int testLabel = labels[labels.size() - 1];

	images.pop_back();
	labels.pop_back();*/

	Mat testSample = imread(pathToRecognizedImage, CV_LOAD_IMAGE_GRAYSCALE);
	int testLabel = originalID;

	int selection;

	//cout << "Train the face recognizer (1)\nLoad trained recognizer (2)\nTrain and save the recognizer (3)\n";
	//cin >> selection;

	clock_t start = clock();
	Ptr<FaceRecognizer> model = createLBPHFaceRecognizer();
	model->load("first_database.yml");
	/*
	
	if (selection == 1) model->train(images, labels);
	//model->set("threshold", 300.0);
	else if (selection == 2) 
	else if (selection == 3) { model->train(images, labels); model->save("first_database.yml"); }
	*/

	int predictedLabel = model->predict(testSample);

	clock_t end = clock();

	float seconds = (float)(end - start) / CLOCKS_PER_SEC;

	string result_message = format("Predicted class = %d / Actual class = %d.\nTime: %.4f", predictedLabel, testLabel, seconds);
	cout << result_message << endl;


	//model->set("threshold", 250.0);
	//-1 means face is unknown
	//predictedLabel = model->predict(testSample);
	//cout << "After setting threshold, predicted class = " << predictedLabel << endl;

	/*cout << "Model Information:" << endl;
	string model_info = format("\tLBPH(radius=%i, neighbors=%i, grid_x=%i, grid_y=%i, threshold=%.2f)",
	model->getInt("radius"),
	model->getInt("neighbors"),
	model->getInt("grid_x"),
	model->getInt("grid_y"),
	model->getDouble("threshold"));
	cout << model_info << endl;

	vector<Mat> histograms = model->getMatVector("histograms");

	cout << "Size of the histograms: " << histograms[0].total() << endl;*/

	return predictedLabel;
}