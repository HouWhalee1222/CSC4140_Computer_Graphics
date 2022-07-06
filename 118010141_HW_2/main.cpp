#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <cmath>
// add some other header files you need

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;
    std::cout << "view" << std::endl << view << std::endl;  // check data

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle, Eigen::Vector3f T, Eigen::Vector3f S, Eigen::Vector3f P0, Eigen::Vector3f P1)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    //Step 1: Build the Translation Matrix M_trans:
    Eigen::Matrix4f M_trans;
    M_trans << 1, 0, 0, -T[0],
               0, 1, 0, -T[1],
               0, 0, 1, -T[2],
               0, 0, 0, 1;
    //Step 2: Build the Scale Matrix S_trans:
    Eigen::Matrix4f S_trans;
    S_trans << S[0], 0, 0, 0,
               0, S[1], 0, 0,
               0, 0, S[2], 0,
               0, 0, 0, 1;
    //Step 3: Implement Rodrigues' Rotation Formular, rotation by angle theta around axix u, then get the model matrix
	// The axis u is determined by two points, u = P1-P0: Eigen::Vector3f P0 ,Eigen::Vector3f P1  
    Eigen::Vector3f u = P1 - P0;
    u.normalize();
    // Create the model matrix for rotating the triangle around a given axis. // Hint: normalize axis first
    Eigen::Matrix4f R_trans;
    double cosA = cos(rotation_angle * MY_PI / 180);
    double sinA = sin(rotation_angle* MY_PI / 180);
    R_trans << cosA + u[0]*u[0]*(1-cosA), u[0]*u[1]*(1-cosA)-u[2]*sinA, u[0]*u[2]*(1-cosA)+u[1]*sinA, 0,
               u[1]*u[0]*(1-cosA)+u[2]*sinA, cosA+u[1]*u[1]*(1-cosA), u[1]*u[2]*(1-cosA)-u[0]*sinA, 0,
               u[2]*u[0]*(1-cosA)-u[1]*sinA, u[2]*u[1]*(1-cosA)+u[0]*sinA, cosA+u[2]*u[2]*(1-cosA), 0,
               0, 0, 0, 1;
    std::cout << "Rotation Matrix:\n" << R_trans << std::endl;

	//Step 4: Use Eigen's "AngleAxisf" to verify your Rotation
	Eigen::AngleAxisf rotation_vector(rotation_angle * MY_PI / 180, Vector3f(u[0], u[1], u[2]));  
	Eigen::Matrix3f rotation_matrix;
	rotation_matrix = rotation_vector.toRotationMatrix();
    std::cout << "Verified Rotation Matrix:\n" << rotation_matrix << std::endl;

    model = S_trans * R_trans * M_trans * model;
    std::cout << "Model Matrix:\n" << model << std::endl;

	return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
    // Implement this function

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.
    float fov = eye_fov * MY_PI / 180;
    float t = zNear * tan(fov / 2);  // Top, y, t = -b
    float r = t * aspect_ratio;  // right, x, r = -l

    // frustum -> cubic
    Eigen::Matrix4f F2C;
    F2C << zNear, 0, 0, 0,
           0, zNear, 0, 0,
           0, 0, zNear+zFar, -zNear*zFar, 
           0, 0, 1, 0;

    // orthographic projection
    Eigen::Matrix4f ProjT;
    ProjT << 1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 1, -(zNear+zFar)/2,
             0, 0, 0, 1;
    
    Eigen::Matrix4f ProjS;
    ProjS << 1/r, 0, 0, 0,
             0, 1/t, 0, 0,
             0, 0, 2/(zFar-zNear), 0,
             0, 0, 0, 1;

    // squash all transformations
    projection = ProjS * ProjT * F2C * projection;
    std::cout << "projection" << std::endl << projection << std::endl; //check

    // Verify
    Eigen::Matrix4f verify;
    verify << zNear/r, 0, 0, 0,
              0, zNear/t, 0, 0,
              0, 0, (zFar+zNear)/(zFar-zNear), -2*zFar*zNear/(zFar-zNear),
              0, 0, 1, 0;
    std::cout << "verify" << std::endl << verify << std::endl; //check   
    return projection;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "result.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
    }

    rst::rasterizer r(1024, 1024);  // Image size

/* ========================================================================= */
    // Parameter Setting
    // define your eye position "eye_pos" to a proper position
    Eigen::Vector3f eye_pos = {0, 0, 10};  // to be modified

    // define a triangle named by "pos" and "ind"
    std::vector<Eigen::Vector3f> pos = {{5, 0, -5}, {0, 5, -5}, {-5, 0, -5}};
    std::vector<Eigen::Vector3i> ind = {{0, 1, 2}};

    // added parameters for get_projection_matrix(float eye_fov, float aspect_ratio,float zNear, float zFar)

    float eye_fov = 45.0;
    float aspect_ratio = 1.0;
    float zNear = 0.1;
    float zFar = 10;

    Eigen::Vector3f T = {0, 0, 0};
    Eigen::Vector3f S = {1, 1, 1};
    Eigen::Vector3f P0 = {0, 0, 0};
    Eigen::Vector3f P1 = {0, 0, 1};

    // Eigen::Vector3f eye_pos = {3, 1.5, 5};  // to be modified

    // // define a triangle named by "pos" and "ind"
    // std::vector<Eigen::Vector3f> pos = {{0, 0, 0}, {0, 3, 0}, {4, 3, 0}};
    // std::vector<Eigen::Vector3i> ind = {{0, 1, 2}};

    // // added parameters for get_projection_matrix(float eye_fov, float aspect_ratio,float zNear, float zFar)

    // float eye_fov = 60.0;
    // float aspect_ratio = 2.5;
    // float zNear = 0.5;
    // float zFar = 20;

    // Eigen::Vector3f T = {0.5, 0.5, 1};
    // Eigen::Vector3f S = {1.5, 1, 0.5};
    // Eigen::Vector3f P0 = {1, 2, 0};
    // Eigen::Vector3f P1 = {0, 0, 1};

/* ========================================================================= */

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle, T, S, P0, P1));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(eye_fov, aspect_ratio, zNear, zFar));

        
        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(1024, 1024, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle, T, S, P0, P1));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(eye_fov, aspect_ratio, zNear, zFar));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(1024, 1024, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(0);

        std::cout << "frame count: " << frame_count++ << '\n';
        std::cout << "angle: " << angle << std::endl;

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
