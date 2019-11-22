//
// Simple example to demonstrate how to use the MAVSDK.
//
// Author: Julian Oes <julian@oes.ch>

#include <chrono>
#include <cstdint>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/offboard/offboard.h>
#include <fstream>
#include <sstream>
#include <thread>
#include "system.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include "share_memory.h"
#include "rapidjson/document.h"

using namespace std::placeholders; // for `_1`
using namespace mavsdk;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::seconds;

#define ERROR_CONSOLE_TEXT "\033[31m" // Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" // Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m" // Restore normal console colour

float gposition_ned_down_m;
float gvelocity_ned_down_m_s;
float geuler_angle_degree;
float velocity_uvw[3];
//Telemetry::Quaternion gQuaternion;
float eRb[3][3];
int64_t init_time; 
//height control PID parameters
float Kp_height = 1.0f;
float Kp_vz = 0.2;
float Ki_vz = 0.01f;
float Kp_yaw = 1.0f;
float Kp_servo_yaw = 0.0f;
float Kp_servo_height = 0.0f;

float z_d = -1.5f;
int land_cnt = 0;

void usage(std::string bin_name)
{
    std::cout << NORMAL_CONSOLE_TEXT << "Usage : " << bin_name << " <connection_url>" << std::endl
              << "Connection URL format should be :" << std::endl
              << " For TCP : tcp://[server_host][:server_port]" << std::endl
              << " For UDP : udp://[bind_host][:bind_port]" << std::endl
              << " For Serial : serial:///path/to/serial/dev[:baudrate]" << std::endl
              << "For example, to connect to the simulator use URL: udp://:14540" << std::endl;
}

void component_discovered(ComponentType component_type)
{
    std::cout << NORMAL_CONSOLE_TEXT << "Discovered a component with type "
              << unsigned(component_type) << std::endl;
}


// Handles Offboard's result
inline void offboard_error_exit(Offboard::Result result, const std::string& message)
{
    if (result != Offboard::Result::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << Offboard::result_str(result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}
// Logs during Offboard control
inline void offboard_log(const std::string& offb_mode, const std::string msg)
{
    std::cout << "[" << offb_mode << "] " << msg << std::endl;
}

inline int64_t getCurrentTime()
{    
    struct timeval tv;    
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;    
}

inline float Saturation_yaw(float y)
{
    if (y > 30.0f)
        return 30.0f;
    else if (y < -30.0f)
        return -30.0f;
    else
        return y;
}

inline float Saturation_height(float h)
{
    if (h > 1.0f)
        return 1.0f;
    else if (h < -1.0f)
        return -1.0f;
    else
        return h;
}

inline void vel_ctrl(std::shared_ptr<mavsdk::Offboard> offboard, float vu, float vv, float delta_height, float yaw_rate)
{
    z_d += delta_height;
    static float vz_inter = 0.0f;
    float position_down = gposition_ned_down_m;
    float v_d = Kp_height*(z_d - position_down);
    float velocity_down = gvelocity_ned_down_m_s;
    float e_vz = -(v_d - velocity_down);

    vz_inter = vz_inter + e_vz*0.02f;
    if (vz_inter > 0.2f)  vz_inter = 0.2f;
    if (vz_inter < -0.2f)  vz_inter = -0.2f;
    float thrust = (Kp_vz*e_vz + Ki_vz*vz_inter)+ 0.5f;
    if (thrust > 1.0f) thrust = 1.0f;
    if (thrust < 0.0f) thrust = 0.0f;

    offboard->set_velocity_body({vu, vv, v_d, yaw_rate});
    std::cout << "Current_Time: " << getCurrentTime() - init_time << ", ";
    std::cout << "set_vu: " << vu << ", set_vv: " << vv << ", set_yawrate: " << yaw_rate << ", set_vd: " << v_d << std::endl;
}

void print_pos_vel(Telemetry::PositionVelocityNED position_velocity_ned)
{
    gposition_ned_down_m = position_velocity_ned.position.down_m;
    // std::cout << "Current_Time: " << getCurrentTime() - init_time << ", ";
    // std::cout << "pos_n: " << position_velocity_ned.position.north_m 
    //           << ", pos_e: " << position_velocity_ned.position.east_m
    //           << ", pos_d: " << gposition_ned_down_m << ", ";
    gvelocity_ned_down_m_s = position_velocity_ned.velocity.down_m_s;
    // std::cout << "vel_n: " << position_velocity_ned.velocity.north_m_s 
    //           << ", vel_e: " << position_velocity_ned.velocity.east_m_s
    //           << ", vel_d: " << gvelocity_ned_down_m_s << ", ";
    // velocity_uvw[0] = eRb[0][0]*position_velocity_ned.velocity.north_m_s + eRb[1][0]*position_velocity_ned.velocity.east_m_s + eRb[2][0]*position_velocity_ned.velocity.down_m_s;
    // velocity_uvw[1] = eRb[0][1]*position_velocity_ned.velocity.north_m_s + eRb[1][1]*position_velocity_ned.velocity.east_m_s + eRb[2][1]*position_velocity_ned.velocity.down_m_s;
    // velocity_uvw[2] = eRb[0][2]*position_velocity_ned.velocity.north_m_s + eRb[1][2]*position_velocity_ned.velocity.east_m_s + eRb[2][2]*position_velocity_ned.velocity.down_m_s;
    // std::cout << "vel_u: " <<   velocity_uvw[0] 
    //           << ", vel_v: " << velocity_uvw[1]
    //           << ", vel_w: " << velocity_uvw[2] << std::endl;
}

// void print_euler(Telemetry::EulerAngle euler_angle)
// {
//     geuler_angle_degree = euler_angle.yaw_deg;
//     std::cout << "Current_Time: " << getCurrentTime() - init_time << ", ";
//     std::cout << "roll_angle: " << euler_angle.roll_deg
//               << ", pitch_angle: " << euler_angle.pitch_deg
//               << ", yaw_angle: " << geuler_angle_degree << std::endl;
// }

// void print_ang_rate(Telemetry::AngularVelocityBody angular_velocity_body)
// {
//     std::cout << "Current_Time: " << getCurrentTime() - init_time << ", ";
//     std::cout << "roll_rate: " << angular_velocity_body.roll_rad_s
//               << ", pitch_rate: " << angular_velocity_body.pitch_rad_s
//               << ", yaw_rate: " << angular_velocity_body.yaw_rad_s << std::endl;
// }

// void print_quat(Telemetry::Quaternion quat)
// {
//     eRb[0][0] = quat.w*quat.w + quat.x*quat.x - quat.y*quat.y - quat.z*quat.z;
//     eRb[0][1] = 2*(quat.x*quat.y - quat.w*quat.z);
//     eRb[0][2] = 2*(quat.x*quat.z + quat.w*quat.y);
//     eRb[1][0] = 2*(quat.x*quat.y + quat.w*quat.z);
//     eRb[1][1] = quat.w*quat.w - quat.x*quat.x + quat.y*quat.y - quat.z*quat.z;
//     eRb[1][2] = 2*(quat.y*quat.z - quat.w*quat.x);
//     eRb[2][0] = 2*(quat.x*quat.z - quat.w*quat.y);
//     eRb[2][1] = 2*(quat.y*quat.z + quat.w*quat.x);
//     eRb[2][2] = quat.w*quat.w - quat.x*quat.x - quat.y*quat.y + quat.z*quat.z;
// }

bool offb_ctrl_servo(std::shared_ptr<mavsdk::Offboard> offboard)
{
    double buffer[4] = {0};
    unsigned int length = sizeof(buffer);
    CShareMemory csm("obj", 1024);

    std::string cfg_path = "./cfg/config.json";
    std::ifstream in(cfg_path);
    std::ostringstream tmp;
    tmp << in.rdbuf();
    std::string json = tmp.str();
    std::cout << "cfg:\n" << json << std::endl;
    rapidjson::Document document;
    document.Parse(json.c_str());
    unsigned int width = document["camera"]["frame_width"].GetUint();
    unsigned int height = document["camera"]["frame_height"].GetUint();
    Kp_height = document["control_parameters"]["Kp_height"].GetFloat();
    Kp_vz = document["control_parameters"]["Kp_vz"].GetFloat();
    Ki_vz = document["control_parameters"]["Ki_vz"].GetFloat();
    Kp_yaw = document["control_parameters"]["Kp_yaw"].GetFloat();
    Kp_servo_yaw = document["control_parameters"]["Kp_servo_yaw"].GetFloat();
    Kp_servo_height = document["control_parameters"]["Kp_servo_height"].GetFloat();
    std::cout << "Kp_servo_yaw: " << Kp_servo_yaw << ", Kp_servo_height: " << Kp_servo_height << std::endl;

    offboard->set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});

    while (1)
    {
        // if (land_cnt >= 50) return true;
        csm.GetFromShareMem(buffer, length);
        // if (buffer[0] < 0)
        // {
        //     land_cnt++;
        //     sleep_for(milliseconds(10));
        //     continue;
        // }
        land_cnt = 0;
        double ex = buffer[0] - width / 2;
        double ey = buffer[1] - height / 2;
        std::cout << "Buffer: ";
        for (int i = 0; i < 4; i++)
        {
            std::cout << buffer[i] << ", ";
        }
        std::cout << std::endl;
        sleep_for(milliseconds(20));

        // offboard->set_attitude_rate({0.0f, pitch_rate, k_yaw * (float)ex, k_thrust * (float)ey});
        // set_velocity_body(VelocityBodyYawspeed), {forward_m_s, right_m_s, down_m_s, yawspeed_deg_s}
        // offboard->set_velocity_body({0.0f, 0.0f, Saturation_height(Kp_servo_height*ey), Saturation_yaw(Kp_servo_yaw*ex)});
        // inline void vel_ctrl(std::shared_ptr<mavsdk::Offboard> offboard, float vu, float vv, float delta_height, float yaw_rate)
        vel_ctrl(offboard, 0.5f, 0.0f, Saturation_height(Kp_servo_height*ey), Saturation_yaw(Kp_servo_yaw*ex));
        std::cout << "vel_ctrl: " << 0.5f << ", " << 0.0f << ", " << Saturation_height(Kp_servo_height*ey) << ", " << Saturation_yaw(Kp_servo_yaw*ex) << std::endl;
        sleep_for(milliseconds(20));
    }
    return true;
}

int main(int argc, char** argv)
{
    Mavsdk dc;
    std::string connection_url;
    ConnectionResult connection_result;

    bool discovered_system = false;
    if (argc == 2) {
        connection_url = argv[1];
        connection_result = dc.add_any_connection(connection_url);
    } else {
        usage(argv[0]);
        return 1;
    }

    if (connection_result != ConnectionResult::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT
                  << "Connection failed: " << connection_result_str(connection_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // If there were multiple, we could specify it with: dc.system(uint64_t uuid);
    System& system = dc.system();

    std::cout << "Waiting to discover system..." << std::endl;
    dc.register_on_discover([&discovered_system](uint64_t uuid) {
        std::cout << "Discovered system with UUID: " << uuid << std::endl;
        discovered_system = true;
    });

    // We usually receive heartbeats at 1Hz, therefore we should find a system after around 2 seconds.
    sleep_for(seconds(2));

    if (!discovered_system) {
        std::cout << ERROR_CONSOLE_TEXT << "No system found, exiting." << NORMAL_CONSOLE_TEXT
                  << std::endl;
        return 1;
    }

    // Register a callback so we get told when components (camera, gimbal) etc are found.
    system.register_component_discovered_callback(component_discovered);

    auto telemetry = std::make_shared<Telemetry>(system);
    auto action = std::make_shared<Action>(system);
    auto offboard = std::make_shared<Offboard>(system);

    // We want to listen to the altitude of the drone at 1 Hz.
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);
    if (set_rate_result != Telemetry::Result::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT
                  << "Setting rate failed:" << Telemetry::result_str(set_rate_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    ///update states
    init_time = getCurrentTime();
    telemetry->position_velocity_ned_async(print_pos_vel);
    // telemetry->attitude_euler_angle_async(print_euler);
    // telemetry->attitude_angular_velocity_body_async(print_ang_rate);
    // telemetry->attitude_quaternion_async(print_quat);

    // Check if vehicle is ready to arm
    while (telemetry->health_all_ok_NOGPS() != true) {
        std::cout << "Vehicle is getting ready to arm" << std::endl;
        sleep_for(seconds(1));
    }
    std::cout << telemetry->health_all_ok_NOGPS() << std::endl;

    // Arm vehicle
    std::cout << "Arming..." << std::endl;
    Action::Result arm_result = action->arm();
    if (arm_result != Action::Result::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT << "Arming failed:" << Action::result_str(arm_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        std::cout << "Arm state fail! " <<  std::endl;
        return 1;
    }
    else
    {
		std::cout << "Arm success!" << std::endl;
	}

    
    // takeoff
    arm_result = action->takeoff();	
    std::cout << "start takeoff" << std::endl;
    sleep_for(seconds(5));
    std::cout << "takeoff finish" << std::endl;

    

    // Send it once before starting offboard, otherwise it will be rejected.
    offboard->set_attitude({0, 0, 0, 0.6});
    
    // start offboard mode
    Offboard::Result offboard_result = offboard->start();
    offboard_error_exit(offboard_result, "Offboard start failed");

    // start visual servo
    std::cout << "Start visual servo" << std::endl;
    offb_ctrl_servo(offboard);
    std::cout << "End visual servo" << std::endl;
    


    // Now, stop offboard mode.
    offboard_result = offboard->stop();
    offboard_error_exit(offboard_result, "Offboard stop failed: ");


    std::cout << "Landing..." << std::endl;
    const Action::Result land_result = action->land();
    if (land_result != Action::Result::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT << "Land failed:" << Action::result_str(land_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // Check if vehicle is still in air
    while (telemetry->in_air()) {
        std::cout << "Vehicle is landing..." << std::endl;
        sleep_for(seconds(1));
    }
    std::cout << "Landed!" << std::endl;

    // We are relying on auto-disarming but let's keep watching the telemetry for a bit longer.
    sleep_for(seconds(3));
    std::cout << "Finished..." << std::endl;

    return 0;
}
