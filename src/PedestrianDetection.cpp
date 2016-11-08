//============================================================================
// Name        : PedestrianDetection.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

// Setting up: C++11
// http://stackoverflow.com/questions/9131763/eclipse-cdt-c11-c0x-support
// Setting up opencv:
// http://docs.opencv.org/doc/tutorials/introduction/linux_eclipse/linux_eclipse.html
// Matio is required to read and write mat-files.
// http://sourceforge.const double PI  =3.141592653589793238463;net/projects/matio/

#include <iostream>
#include <cv.h>
#include <highgui.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include "PedestrianDetector.hpp"
#include <dirent.h>
#include <string.h>
#include <math.h>
#include <ros/ros.h>
#include <opencv2/opencv.hpp>
#include <sensor_msgs/Image.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/MultiArrayDimension.h>
#include <std_msgs/MultiArrayLayout.h>
#include <image_transport/image_transport.h>
#include <camera_info_manager/camera_info_manager.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <visualization_msgs/Marker.h>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace cv;
cv::Mat showBB(vector<bbType> bbs, Mat image, bool wait) {
	double alpha = 0.3;
	double threshold = 50;
	for (int iBbs = 0; iBbs < bbs.size(); iBbs++) {
		//printf("n%d, x: %f, %f, %f, %f, %f, Dist: %f \n",iBbs,bbs[iBbs].x1,bbs[iBbs].y2, bbs[iBbs].width3, bbs[iBbs].height4, bbs[iBbs].score5, bbs[iBbs].distance );

		Scalar useColor(0, 0, 0);
		if (bbs[iBbs].score5 > 70) {
			alpha = 0.3;
			useColor[1] = 0; // G
			useColor[2] = 255; // R
		} else {
			alpha = 0.1;
			useColor[1] = 255; // G
			useColor[2] = 0; // R
		}
		Mat rectangleImage(image.size[0], image.size[1], CV_8UC3,
				cv::Scalar(0, 0, 0));
		rectangle(rectangleImage,
				Rect(bbs[iBbs].x1, bbs[iBbs].y2, bbs[iBbs].width3,
						bbs[iBbs].height4), useColor, CV_FILLED, 8, 0);

		stringstream strsScore;
		strsScore.precision(3);
		strsScore << bbs[iBbs].score5;
		putText(image, strsScore.str() + "p", Point(bbs[iBbs].x1, bbs[iBbs].y2),
				FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255)); //, int thickness=1, int lineType=8, bool bottomLeftOrigin=false

		stringstream strsDistance;
		strsDistance.precision(3);
		strsDistance << bbs[iBbs].distance;
		putText(image, strsDistance.str() + "m",
				Point(bbs[iBbs].x1, bbs[iBbs].y2 + bbs[iBbs].height4),
				FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255)); //, int thickness=1, int lineType=8, bool bottomLeftOrigin=false

		//putText(image, to_string((int)(round(bbs[iBbs].score5))), Point(bbs[iBbs].x1,bbs[iBbs].y2), FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,255)); //, int thickness=1, int lineType=8, bool bottomLeftOrigin=false
		//putText(image, "d:" + to_string((int)(round(bbs[iBbs].distance))), Point(bbs[iBbs].x1,bbs[iBbs].y2+bbs[iBbs].height4), FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,255)); //, int thickness=1, int lineType=8, bool bottomLeftOrigin=false
		addWeighted(rectangleImage, alpha, image, 1, 0.0, image);
	}

//	if (wait)
//		namedWindow("BoundingsBox", CV_WINDOW_AUTOSIZE);
//	imshow("BoundingsBox", image);
	return image;
	//printf("Code is done!!");

}

class MyNode {
public:
	MyNode() :
			nh("~"), it(nh) {
		ROS_ERROR("NodeBeingCreated");
		nh.param<double>("FOV_verticalDeg",FOV_verticalDeg,47.0);
		nh.param<double>("FOV_horizontal",FOV_horizontal,83.0);
		nh.param<double>("angleTiltDegrees",angleTiltDegrees,7.0);
		nh.param<double>("cameraHeight",cameraHeight,1.9);
		nh.param<double>("imageResize",imageResize,0.5);
		nh.param<std::string>("topic_name",topic_name,"/usb_cam/image_raw");
		nh.param<std::string>("model_dir",model_dir,"model");
		vector<string> strParts;
		boost::split(strParts,topic_name,boost::is_any_of("/"));
		ROS_ERROR("%s\n",model_dir.c_str());
		//cam_pub = it.advertiseCamera("imageWithBBox", 1);
		cam_pub = it.advertise("imageWithBBox", 1);


		//ROS_INFO("Estimating human distance using a camera height of %fm and angle of %fdegrees",cameraHeight,angleTiltDegrees);
		//ROS_ERROR("Estimating human distance using a camera height of %fm and angle of %fdegrees",cameraHeight,angleTiltDegrees);
		//cinfor_ = boost::shared_ptr<camera_info_manager::CameraInfoManager>(new camera_info_manager::CameraInfoManager(nh, "test", ""));

		array_pub = nh.advertise<std_msgs::Float64MultiArray>("DistanceAngle", 1);

		vector<string> outputTopicTmp;
		outputTopicTmp.push_back("BBox");
		outputTopicTmp.push_back(strParts[1]);
		array_pub2 = nh.advertise<std_msgs::Float64MultiArray>(boost::algorithm::join(outputTopicTmp,"/"), 1);
		vis_pub = nh.advertise<visualization_msgs::Marker>( "/PedestrianMarker", 0 );
		//cam_sub = it.subscribeCamera(topic_name.c_str(), 1, &MyNode::onImage, this);
		cam_sub = it.subscribe(topic_name.c_str(), 1, &MyNode::onImage,this);
		
		//cam_sub = it.subscribe(topic_name.c_str(), 1, &MyNode::onImage);
		

		/* Making detector object.
		bool fastDetector = 1;
		if(fastDetector) {
			model_dir = "pedmodels/AcfInriaDetector.mat";
		}
		else {
			model_dir = "pedmodels/LdcfInriaDetector.mat";
		}*/ 
		ROS_ERROR("JustBeforeModelLoad");
		oPedDetector = new PedestrianDetector(model_dir);
		oPedDetector->setCameraSetup(FOV_verticalDeg, FOV_horizontal,
					angleTiltDegrees, cameraHeight);

		ROS_ERROR("NodeHasBeenCreated");


	}
	;

	~MyNode() {

	}
	;

	void onImage(const sensor_msgs::ImageConstPtr& msg) {
		// do all the stuff here
		//ROS_ERROR("GOT Image");
		//convert  image to opencv
		ROS_ERROR("ImageHasBeenReceived"); 
		cv_bridge::CvImagePtr cv_ptr;
		try {
			cv_ptr = cv_bridge::toCvCopy(msg,
					sensor_msgs::image_encodings::BGR8);
		} catch (cv_bridge::Exception& e) {
			ROS_ERROR("cv_bridge exception: %s", e.what());
			return;
		}
		resize(cv_ptr->image, image, Size(), imageResize, imageResize);
		

		bbs = oPedDetector->pedDetector(image);


		cv::Mat img = showBB(bbs, image, 0);


		//sensor_msgs::CameraInfoPtr cc(new sensor_msgs::CameraInfo(cinfor_->getCameraInfo()));
		sensor_msgs::ImagePtr msg_out = cv_bridge::CvImage(std_msgs::Header(),"bgr8", img).toImageMsg();
		msg_out->header.stamp = ros::Time::now();

		std_msgs::Float64MultiArray bbMsg;
		std_msgs::Float64MultiArray bboxMsg;
		bbMsg.data.clear();
		bboxMsg.data.clear();
		for (int iBbs = 0; iBbs < bbs.size(); ++iBbs) {

			bbMsg.data.push_back(bbs[iBbs].distance);
			bbMsg.data.push_back(bbs[iBbs].angle);
			bboxMsg.data.push_back(bbs[iBbs].x1/float(image.cols));
			bboxMsg.data.push_back(bbs[iBbs].y2/float(image.rows));
			bboxMsg.data.push_back(bbs[iBbs].width3/float(image.cols));
			bboxMsg.data.push_back(bbs[iBbs].height4/float(image.rows));
			bboxMsg.data.push_back(fmin(bbs[iBbs].score5/200.0,1));
			bboxMsg.data.push_back(2); // Humans are given the class 2.
			//bbMsg.data.push_back(bbs[iBbs].)
		}

		// Creating visual marker
		visualization_msgs::Marker marker;
		marker.header.frame_id = "/laser";
		marker.header.stamp = ros::Time();
		marker.ns = "my_namespace";
		marker.id = 0;
		marker.type = visualization_msgs::Marker::CYLINDER;
		marker.action = visualization_msgs::Marker::ADD;
		marker.pose.orientation.x = 0.0;
		marker.pose.orientation.y = 0.0;
		marker.pose.orientation.z = 0.0;
		marker.pose.orientation.w = 1.0;
		marker.scale.x = 1;
		marker.scale.y = 1.0;
		marker.scale.z = 2.0;
		marker.color.a = 1.0; // Don't forget to set the alpha!
		marker.color.r = 1.0;
		marker.color.g = 0.0;
		marker.color.b = 0.0;
		marker.pose.position.x = 0;
		marker.pose.position.y = 0;
		marker.pose.position.z = 1;
		
		for (int iBbs = 0; iBbs < bbs.size(); ++iBbs) {
			marker.pose.position.x = bbs[iBbs].distance*cos(bbs[iBbs].angle);
			marker.pose.position.y = bbs[iBbs].distance*sin(bbs[iBbs].angle);
			vis_pub.publish(marker);	
		}
		if(bbs.size() == 0) {
			marker.color.a = 0.0;
			vis_pub.publish(marker);	
		}
		//cam_pub.publish(msg_out, cc);
		cam_pub.publish(msg_out);
		array_pub.publish(bbMsg);
		array_pub2.publish(bboxMsg);

	}
private:
	double FOV_verticalDeg,FOV_horizontal,angleTiltDegrees,cameraHeight;
	double imageResize;
	std::string model_dir;
	std::string topic_name;
	cv::Mat image;
	std::vector<bbType> bbs;
	ros::NodeHandle nh;
	image_transport::ImageTransport it;
	//image_transport::CameraPublisher cam_pub;
	//image_transport::CameraSubscriber cam_sub;
	image_transport::Publisher cam_pub;
	image_transport::Subscriber cam_sub;
	//boost::shared_ptr<camera_info_manager::CameraInfoManager> cinfor_;
	ros::Publisher array_pub;
	ros::Publisher array_pub2;
	ros::Publisher vis_pub;
	PedestrianDetector* oPedDetector;

};

int main(int argc, char** argv) {

	ros::init(argc, argv, "pede");

	MyNode node;

	ros::spin();
//
//	ros::NodeHandle nh("~");
//
//	image_transport::ImageTransport it(nh);
//
//	image_transport::CameraPublisher cam_pub = it.advertiseCamera("processed",
//			1);
//
//	boost::shared_ptr<camera_info_manager::CameraInfoManager> cinfor_(
//			new camera_info_manager::CameraInfoManager(nh, "test", ""));
//
//	ros::Publisher array_pub = nh.advertise<std_msgs::Float64MultiArray>(
//			"array", 10);
//
//	string dirImage = "pedmodels/InriaTest.png";
//	//string dirImage = "pedmodels/KimTest.jpg";
//	//string dirImage = "pedmodels/MarkTest.jpg";
//	vector<bbType> bbs;
//	double FOV_verticalDeg = 47; // Vertical field-of-view of camera.
//	double FOV_horizontal = 83; // Horizontal field-of-view of camera.
//	double angleTiltDegrees = 7; // Downward tilt in degrees.
//	double cameraHeight = 1.9; // Height Position of camera.
//
//	double imageResize = 0.5;
//	//Mat image = imread(dirImage, 1);
//
//	Mat image, inputImage;
//	struct dirent *ent;
//	//string dirImages = "/home/pistol/Desktop/DataFolder/2014-11-03-14-37-11/WebCam";
//	//string dirImages = "/home/pistol/Desktop/DataFolder/2014-10-16-12-18-30/WebCam";
//	string dirImages = "/home/pistol/Desktop/DataFolder/WebCam";
//
//	DIR *dir = opendir(dirImages.data());
//
//	char * pch;
//
//	//image = imread(dirImage, 1);
//
//	//cout << image.cols << "x" << image.rows << endl;
//
//	// Making detector object.
//	bool fastDetector = 1;
//	string dirFolder = "/home/pistol/ice/src/pedestrian_detector/";
//	string dirDetector;
//	if (fastDetector) {
//		dirDetector = dirFolder + "pedmodels/AcfInriaDetector.mat";
//	} else {
//		dirDetector = dirFolder + "pedmodels/LdcfInriaDetector.mat";
//	}
//
//	PedestrianDetector oPedDetector(dirDetector);
//	// Providing camera settings.
//	oPedDetector.setCameraSetup(FOV_verticalDeg, FOV_horizontal,
//			angleTiltDegrees, cameraHeight);
//
//	bool useExample = 0;
//	if (useExample) {
//		inputImage = imread(dirImage, 1);
//		if (!inputImage.data) {
//			printf("No image data \n");
//			return -1;
//		}
//		resize(inputImage, image, Size(), imageResize, imageResize);
//
//		clock_t start, end;
//		start = clock();
//		bbs = oPedDetector.pedDetector(image);
//		end = clock();
//		double time = (double) (end - start) / CLOCKS_PER_SEC * 1000.0;
//		cout << "\n t1:" << time << " ms\n";
//
//		showBB(bbs, image, 0);
//		waitKey(0);
//
//	}
//	struct dirent **namelist;
//	int i;
//	string fileName;
//	//string pch;
//	int n = scandir(dirImages.data(), &namelist, 0, alphasort);
//	if (n < 0)
//		perror("scandir");
//	else {
//		for (i = 0; i < n; i++) {
//			printf("%s\n", namelist[i]->d_name);
//
//			//while ((ent = readdir (dir)) != NULL) {
//			fileName = namelist[i]->d_name;
//			string pch = strrchr(namelist[i]->d_name, '.');
//			free(namelist[i]);
//			if (pch.compare(".jpg") == 0) {
//				printf("%s\n", fileName.data());
//				string dirImage = (dirImages + '/' + fileName);
//				inputImage = imread(dirImage.data(), 1);
//				resize(inputImage, image, Size(), imageResize, imageResize);
//
//				clock_t start, end;
//				start = clock();
//				bbs = oPedDetector.pedDetector(image);
//				end = clock();
//				double time = (double) (end - start) / CLOCKS_PER_SEC * 1000.0;
//				cout << "\n t1:" << time << " ms\n";
//				cv::Mat img = showBB(bbs, image, 0);
//				waitKey(10);
//
//				sensor_msgs::CameraInfoPtr cc(
//						new sensor_msgs::CameraInfo(cinfor_->getCameraInfo()));
//				sensor_msgs::ImagePtr msg = cv_bridge::CvImage(
//						std_msgs::Header(), "bgr8", img).toImageMsg();
//				msg->header.stamp = ros::Time::now();
//
//				std_msgs::Float64MultiArray bbMsg;
//				bbMsg.data.clear();
//				for (int iBbs = 0; iBbs < bbs.size(); ++iBbs) {
//
//					bbMsg.data.push_back(bbs[iBbs].distance);
//					bbMsg.data.push_back(bbs[iBbs].angle);
//					//bbMsg.data.push_back(bbs[iBbs].)
//				}
//
//				cam_pub.publish(msg, cc);
//				array_pub.publish(bbMsg);
//				ros::spinOnce();
//
//			}
//			//closedir (dir);
//		}
//	}
//	free(namelist);

	return 0;
}

