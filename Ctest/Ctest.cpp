// Ctest.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include"iostream"
#include"opencv.hpp"
#include"highgui.hpp"
#include"xfeatures2d.hpp"

#define Lcx 652.74           
#define Lcy 459.102    //左相机的光心位置

#define Rcx 610.779
#define Rcy 445.699   //右相机的光心位置

#define fx1 3359.59
#define fy1 3360.56   //左相机的焦距

#define fx2 3369.11
#define fy2 3369.67   //右相机的焦距

#define La1 -0.0584155
#define La2 0.532731
#define La3 0.000793009	
#define La4 -0.00217376   //左相机的四个畸变参数

#define Ra1 -0.0212227
#define Ra2 0.5244
#define Ra3  0.000552162
#define Ra4 -0.00357077  //右相机的四个畸变参数

#define width 1296
#define height 966

using namespace cv;
using namespace std;

void UnifiedCoord(int p, int num, Mat points3D, ofstream &debug) {   //统一坐标系，p=摄站标号，num=点数，points3D当前摄站坐标系下的位置坐标

	int k;
	float trans[4][4] = { { 1,0,0,0 },{ 0,1,0,0 },{ 0,0,1,0 },{ 0,0,0,1 } };
	Mat Trans_Pos(4, 4, CV_32F, trans);   //当前坐标系对应到摄站1坐标系的变换矩阵

	if (p == 1) k = 1;
	else {
		if (p % 2 == 0) {
			k = p;
		}
		else k = p - 1;
	}

	for (int m = k; m >1; ) {   //计算变换矩阵Trans_Pos
		char calibfile[40];
		sprintf_s(calibfile, "%s%d%s", "datas\\", m, "\\Calib3D_Result1.txt");//标定参数文档
		ifstream calib;
		calib.open(calibfile, ios::in);

		Mat T_t(3, 1, CV_32F);
		Mat T_r(3, 1, CV_32F);

		Vec6f cf;
		for (int i = 0; i < 2; i++) {
			calib >> cf[0];
			calib >> cf[1];
			calib >> cf[2];
			T_t = (Mat_<float>(3, 1) << cf[0], cf[1], cf[2]);   //平移向量

			calib >> cf[3];
			calib >> cf[4];
			calib >> cf[5];
			T_r = (Mat_<float>(3, 1) << cf[3], cf[4], cf[5]);   //旋转向量
		}
		float by = T_r.at<float>(0, 0);
		float w = T_r.at<float>(1, 0);
		float k = T_r.at<float>(2, 0);
		//debug << "p-m:" << p << "-" << m << endl << " T_t:" << endl << T_t << endl << " T_r:" << endl << by<<w<<k << endl;

		//旋转向量转换为旋转矩阵
		float a1 = cos(by)*cos(k) - sin(by)*sin(w)*sin(k);
		float a2 = -cos(by)*sin(k) - sin(by)*sin(w)*cos(k);
		float a3 = -sin(by)*cos(w);
		float b1 = cos(w)*sin(k);
		float b2 = cos(w)*cos(k);
		float b3 = -sin(w);
		float c1 = sin(by)*cos(k) + cos(by)*sin(w)*sin(k);
		float c2 = -sin(by)*sin(k) + cos(by)*sin(w)*cos(k);
		float c3 = cos(by)*cos(w);

		Mat T_r1(3, 3, CV_32F);  //3X3旋转矩阵

		T_r1.at<float>(0, 0) = a1;
		T_r1.at<float>(0, 1) = a2;
		T_r1.at<float>(0, 2) = a3;
		T_r1.at<float>(1, 0) = b1;
		T_r1.at<float>(1, 1) = b2;
		T_r1.at<float>(1, 2) = b3;
		T_r1.at<float>(2, 0) = c1;
		T_r1.at<float>(2, 1) = c2;
		T_r1.at<float>(2, 2) = c3;

		//Mat T_r1(3, 3, CV_32F);
		//Rodrigues(T_r, T_r1); //旋转向量转换为3X3旋转矩阵
		//debug << "T_r1:" << T_r1 << endl;

		Mat Trans1(3, 4, CV_32F);
		hconcat(T_r1, T_t, Trans1);   //3X4 [R T]矩阵

		Mat mm(1, 4, CV_32F);
		mm = (Mat_<float>(1, 4) << 0, 0, 0, 1);
		Mat Trans(4, 4, CV_32F);
		vconcat(Trans1, mm, Trans); // [R T] 最下行补[0 0 0 1]变为4X4 矩阵 
		//debug << " Trans: " << endl << Trans << endl;

		Trans_Pos = Trans_Pos * Trans; //逐一相乘
		//debug << " Trans_Pos: " << endl << Trans_Pos << endl << endl;

		m = m - 2;
	}

	float W_X, W_Y, W_Z;
	Mat world_p(4, num, CV_32F);  //统一到世界坐标系的点坐标

	world_p = Trans_Pos * points3D;

	char pathname[40];
	sprintf_s(pathname, "%s%d%s", "datas\\", p, "\\NewResult3D_Points.txt");    //点云命名 
	ofstream cloud;
	cloud.open(pathname);

	for (int n = 0; n < num; n++) {    //写入点云文档
		W_X = world_p.at<float>(0, n);
		W_Y = world_p.at<float>(1, n);
		W_Z = world_p.at<float>(2, n);
		cloud << W_X <<" "<< W_Y<<" "<< W_Z << endl;
	}
}

int main()
{

	//设置的一些内参、外参、畸变参数
	double m1[3][3] = { { fx1,0,Lcx },{ 0,fy1,Lcy },{ 0,0,1 } };
	Mat dist1 = (Mat_<double>(1, 5) << La1, La2, La3, La4, 0); //左相机畸变参数(k1,k2,p1,p2,k3)
	Mat dist2 = (Mat_<double>(1, 5) << Ra1, Ra2, Ra3, Ra4, 0); //右相机畸变参数
	Mat m1_matrix(Size(3, 3), CV_64F, m1);  //左相机的内参矩阵

	double m2[3][3] = { { fx2,0,Rcx },{ 0,fy2,Rcy },{ 0,0,1 } };
	Mat m2_matrix(Size(3, 3), CV_64F, m2);  //右相机的内参矩阵
	double r2[3][1] = { { -0.0180031 },{ 0.0973247 },{ -0.04762 } };
	Mat r2_src2(Size(1, 3), CV_64F, r2);
	Mat r2_matrix(Size(3, 3), CV_64F); //右相机旋转矩阵
	Rodrigues(r2_src2, r2_matrix);   //旋转向量转化为旋转矩阵
	Mat t2_matrix = (Mat_<double>(3, 1) << -92.9037, 3.04819, 6.23956);   //右相机平移向量
	ofstream debug;
	debug.open("debug.txt");

	for (int p = 1; p <= 13; p++) {
		
		//ofstream debug;
		//debug.open("debug.txt");
		char filename1[40];
		sprintf_s(filename1, "%s%d%s", "datas\\", p, "\\MatchResult2D_LeftPoints.TXT");  //左相机匹配点路径
		
		char filename2[40];
		sprintf_s(filename2, "%s%d%s", "datas\\", p, "\\MatchResult2D_RightPoints.TXT");//右相机匹配点路径

		ifstream file1, file2;
		file1.open(filename1,ios::in);
		file2.open(filename2,ios::in);

		int num = -1;
		char str[200];
		while (!file1.eof()) {
			file1.getline(str, sizeof(str));   //根据匹配点文档求匹配总点数
			num++;
		}
		file1.clear();
		file1.seekg(0, ios::beg);//把文件的读指针从文件开头向后移0个字节

		Mat _src1(num, 1, CV_32FC2);
		Mat _src2(num, 1, CV_32FC2);

		Vec2f dd1;
		for (int i = 0; i < num; i++) {               //读入数据
			file1 >> dd1[0];
			file1 >> dd1[1];
			_src1.at<Vec2f>(i, 0) = dd1;

		}
		for (int i = 0; i < num; i++) {
			file2 >> dd1[0];
			file2 >> dd1[1];
			_src2.at<Vec2f>(i, 0) = dd1;

		}
		Mat _dst1;
		Mat _dst2;

		//图像畸变矫正
		undistortPoints(_src1, _dst1, m1_matrix, dist1); //校正后的坐标需要乘以焦距+中心坐标

		undistortPoints(_src2, _dst2, m2_matrix, dist2);
		//debug << "_src1:" << _src1 << endl << "_dst1:" << _dst1 << endl;

		Mat Rl, Rr; //左右相机旋转矩阵
		Mat Pl, Pr; //校正后的投影矩阵
		Mat Q;      //视差计算矩阵
	    
	    //立体校正，校正后的立体相机光轴平行，且行逐行对齐
		stereoRectify(m1_matrix, dist1, m2_matrix, dist2, cvSize(width, height), r2_matrix, t2_matrix, Rl, Rr, Pl, Pr, Q);// , 0, -1, cvSize(width, height));

		//分别投影原立体相机图像坐标到校正后立体相机图像坐标
		Mat iRl(3, 3, CV_64FC1);
		Mat iRr(3, 3, CV_64FC1);
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				iRl.at<double>(i, j) = Pl.at<double>(i, j);//取Pl的-2列所构成的*3矩阵与Rl相乘获得从原左相机平面图像到矫正后左相机平面图像的转换矩阵
				iRr.at<double>(i, j) = Pr.at<double>(i, j);
			}
		}

		iRl = iRl * Rl;
		iRr = iRr * Rr;

		double ir[9];
		double irr[9];
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				ir[i * 3 + j] = iRl.at<double>(i, j);
			}
		}
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				irr[i * 3 + j] = iRr.at<double>(i, j);
			}
		}

		//根据Pl,PR,Q矩阵，计算像平面矫正后的特征点对坐标(x,y)和左右视差d
		Mat Prec3D(4, 1, CV_64FC1);
		Mat Pworld3D(4, 1, CV_64FC1);

		double fxl, fyl, cxl, cyl;
		double fxr, fyr, cxr, cyr;
		//投影相机中心
		//左相机
		fxl = m1_matrix.at<double>(0, 0);
		fyl = m1_matrix.at<double>(1, 1);
		cxl = m1_matrix.at<double>(0, 2);
		cyl = m1_matrix.at<double>(1, 2);
		//右相机
		fxr = m2_matrix.at<double>(0, 0);
		fyr = m2_matrix.at<double>(1, 1);
		cxr = m2_matrix.at<double>(0, 2);
		cyr = m2_matrix.at<double>(1, 2);

		Vec2f dd;
		Vec2f df;
		Mat point(num, 1, CV_32FC2);   
		Mat point2(num, 1, CV_32FC2);

		for (int i = 0; i < num; i++) {

			df = _dst1.at<Vec2f>(i, 0);
			df[0] = (float)(df[0] * fxl + cxl);
			df[1] = (float)(df[1] * fyl + cyl);
			point.at<Vec2f>(i, 0) = df;

			df = _dst2.at<Vec2f>(i, 0);
			df[0] = (float)(df[0] * fxr + cxr);
			df[1] = (float)(df[1] * fyr + cyr);
			point2.at<Vec2f>(i, 0) = df;      //point后面运算并没有用到
		}

		double x1_, y1_, _x, _y, _w, w, x1, y1;
		double x2_, y2_, x2, y2, z1, w1, wx, wy, wz;

		//用于暂存3d坐标
		double**tmpPoints = new double*[num];
		for (int i = 0; i < num; i++) {
			tmpPoints[i] = new double[3];
		}

		for (int i = 0; i < num; i++) {
			dd = _dst1.at<Vec2f>(i, 0);
			x1_ = dd[0];
			y1_ = dd[1];

			//计算投影坐标
			_x = x1_ * ir[0] + y1_ * ir[1] + ir[2];
			_y = x1_ * ir[3] + y1_ * ir[4] + ir[5];
			_w = x1_ * ir[6] + y1_ * ir[7] + ir[8];
			w = 1. / _w;

			_x = _x * w;
			_y = _y * w;
			x1 = _x;
			y1 = _y;

			dd = _dst2.at<Vec2f>(i, 0);
			x2_ = dd[0];
			y2_ = dd[1];

			_x = x2_ * irr[0] + y2_ * irr[1] + irr[2];
			_y = x2_ * irr[3] + y2_ * irr[4] + irr[5];
			_w = x2_ * irr[6] + y2_ * irr[7] + irr[8];
			w = 1. / _w;
			_x = _x * w;
			_y = _y * w;
			x2 = _x;
			y2 = _y;

			//(x1,y1,d,1)构成视差矢量
			Prec3D.at<double>(0, 0) = x1;
			Prec3D.at<double>(1, 0) = y1;
			Prec3D.at<double>(2, 0) = x1 - x2;
			double PTx = x1 - x2;
			Prec3D.at<double>(3, 0) = 1;

			Pworld3D = Q * Prec3D;
			x1 = Pworld3D.at<double>(0, 0);
			y1 = Pworld3D.at<double>(1, 0);
			z1 = Pworld3D.at<double>(2, 0);
			w1 = Pworld3D.at<double>(3, 0);
			wx = x1 / w1;
			wy = -y1 / w1;
			wz = -z1 / w1;

			tmpPoints[i][0] = wx;
			tmpPoints[i][1] = wy;
			tmpPoints[i][2] = wz;   //numX3矩阵
		}

		//Mat points3D(num, 1, CV_32FC3);
		//Vec3f p3D;

		Mat points3D(4,num, CV_32F);
		//char pathname[40];
		//sprintf_s(pathname, "%s%d%s", "datas\\", p, "\\NewResult3D_Points.txt");    //点云命名 
		//ofstream cloud;
		//cloud.open(pathname);

		for (int i = 0; i < num; i++) {
			//p3D[0] = (float)tmpPoints[i][0];
			//p3D[1] = (float)tmpPoints[i][1];
			//p3D[2] = (float)tmpPoints[i][2];
			//points3D.at<Vec3f>(i, 0) = p3D;
			//cloud << tmpPoints[i][0] << " " << tmpPoints[i][1] << " " << tmpPoints[i][2] << endl;
			points3D.at<float>(0, i) = (float)tmpPoints[i][0];
			points3D.at<float>(1, i) = (float)tmpPoints[i][1];
			points3D.at<float>(2, i) = (float)tmpPoints[i][2];
			points3D.at<float>(3, i) = 1;
		}

		/*******************测试用例*********************/
		//ifstream test;
		//test.open("Result3D_Points_Calib.TXT",ios::in);
		//int num1 = 128;
		//int ppp = 2;
		//Mat testpoint(4, num1, CV_32F);
		//Vec3f vv;
		//for (int i = 0; i < num1; i++) {
		//	test >> vv[0];
		//	test >> vv[1];
		//	test >> vv[2];
		//	testpoint.at<float>(0, i) = vv[0];
		//	testpoint.at<float>(1, i) = vv[1];
		//	testpoint.at<float>(2, i) = vv[2];
		//	testpoint.at<float>(3, i) = 1;
		//}
		//debug << testpoint << endl;
		//UnifiedCoord(ppp, num1, testpoint, debug);

		UnifiedCoord(p, num, points3D,debug);   //全局配准函数

		for (int i = 0; i<num; i++)
		{
			delete[] tmpPoints[i];
		}
		delete[] tmpPoints;

		file1, file2.close();
	}
	return 0;
}
