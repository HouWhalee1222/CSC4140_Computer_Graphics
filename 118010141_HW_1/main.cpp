#include <cmath>
#include <iostream>
#include "Eigen/Core"
#include "Eigen/Dense"
#include "Eigen/SVD"
#include "Eigen/Geometry"

#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/cvdef.h"

using namespace Eigen;
using namespace cv;
using namespace std;

#define pi 3.1415926

void question1() {
    Vector4f v(1, 1.5, 2, 3);
    Vector4f w(0, 1, 2, 4);

    cout << "2.1.1 - Define vector:" << endl << endl;
    cout << "v = \n" << v << endl << endl;
    cout << "w = \n" << w << endl << endl;

    cout << "2.1.2 - vector add:" << endl << endl;
    cout << "v + w = " << endl;
    cout << (v.hnormalized() + w.hnormalized()) << endl << endl;

    cout << "2.1.3 - vector inner product:" << endl << endl;
    cout << "v * w = " << endl;
    cout << v.hnormalized().dot(w.hnormalized()) << endl << endl;

    cout << "2.1.4 - vector cross product:" << endl << endl;
    cout << "v x w = " << endl;
    cout << v.hnormalized().cross(w.hnormalized()) << endl << endl;
}

void question2() {
    Vector4d v(1, 1.5, 2, 3);

    Matrix4d i;
    i << 1,2,3,4,
         5,6,7,8,
         9,10,11,12,
         13,14,15,16;

    Matrix4d j;
    j << 4,3,2,1,
         8,7,6,5,
         12,11,10,9,
         16,15,14,13;

    cout << "2.2.1 - Define matrix:" << endl << endl;
    cout << "i = \n" << i << endl << endl;
    cout << "j = \n" << j << endl << endl;

    cout << "2.2.2 - matrix add:" << endl << endl;
    cout << "i + j = " << endl;
    cout << i + j << endl << endl;

    cout << "2.2.3 - matrix multiply:" << endl << endl;
    cout << "i * j = " << endl;
    cout << i * j << endl << endl;

    cout << "2.2.4 - matrix multiply vector" << endl << endl;
    cout << "i * v = " << endl;
    cout << i * v << endl << endl;
}

void question3() {
    Mat image = imread("lenna.png", IMREAD_GRAYSCALE);

    normalize(image, image, 0, 1, NORM_MINMAX, CV_32FC1);
    // image.convertTo(image, CV_32FC1, 1.0/255.0);
    imshow("Gray Image", image);
    waitKey();

    Mat u, S, v;
    cout << "SVD Computing ..." << endl;
    SVD::compute(image, S, u, v);

    cout << u.size << " " << S.size << " " << v.size << endl; 

    Mat s = Mat::diag(S);

    Mat imageNew(512, 512, CV_32FC1);

    imageNew = u * s(Range::all(),Range(0,1)) * v(Range(0,1),Range::all());
    imshow("First singular value", imageNew);
    waitKey();

    imageNew = u * s(Range::all(),Range(0,10)) * v(Range(0,10),Range::all());
    imshow("First ten singular value", imageNew);
    waitKey();

    imageNew = u * s(Range::all(),Range(0,50)) * v(Range(0,50),Range::all());
    imshow("First fifty singular value", imageNew);
    waitKey();

}

void question4() {
    Vector3d p(1, 2, 3);
    Vector3d q(4, 5, 6);

    Matrix3d x;
    x << 1,0,0,
         0,cos(45*pi/180),-sin(45*pi/180),
         0,sin(45*pi/180),cos(45*pi/180);

    Matrix3d y;
    y << cos(30*pi/180),0,sin(30*pi/180),
         0,1,0,
         -sin(30*pi/180),0,cos(30*pi/180);

    Matrix3d z;
    z << cos(60*pi/180),-sin(60*pi/180),0,
         sin(60*pi/180),cos(60*pi/180),0,
         0,0,1;

    cout << "2.4 - Basic transformation operations" << endl << endl;
    cout << "The new point is at:" << endl;
    cout << (x * y * z * (p-q)) + q << endl;

}

int main() {
    question1();
    question2();
    question3();
    question4();
    return 0;
}
