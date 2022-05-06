#include "fd_hardware/fd_effort_hi.hpp"

#include "dhdc.h"
#include "drdc.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <iostream>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

namespace fd_hardware
{
  // ------------------------------------------------------------------------------------------
hardware_interface::return_type FDEffortHardwareInterface::configure(
  const hardware_interface::HardwareInfo & info)
{
  if (configure_default(info) != hardware_interface::return_type::OK)
  {
    return hardware_interface::return_type::ERROR;
  }

  hw_states_position_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());
  hw_states_velocity_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());
  hw_states_effort_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());
  hw_commands_effort_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());


  for (const hardware_interface::ComponentInfo & joint : info_.joints)
  {
    // PHI has currently exactly 3 states and 1 command interface on each joint
    if (joint.command_interfaces.size() != 1)
    {
      RCLCPP_FATAL(
        rclcpp::get_logger("FDEffortHardwareInterface"),
        "Joint '%s' has %d command interfaces found. 1 expected.", joint.name.c_str(),
        joint.command_interfaces.size());
      return hardware_interface::return_type::ERROR;
    }

    if (joint.command_interfaces[0].name != hardware_interface::HW_IF_EFFORT)
    {
      RCLCPP_FATAL(
        rclcpp::get_logger("FDEffortHardwareInterface"),
        "Joint '%s' have %s command interfaces found. '%s' expected.", joint.name.c_str(),
        joint.command_interfaces[0].name.c_str(), hardware_interface::HW_IF_EFFORT);
      return hardware_interface::return_type::ERROR;
    }

    if (joint.state_interfaces.size() != 3)
    {
      RCLCPP_FATAL(
        rclcpp::get_logger("FDEffortHardwareInterface"),
        "Joint '%s' has %d state interface. 3 expected.", joint.name.c_str(),
        joint.state_interfaces.size());
      return hardware_interface::return_type::ERROR;
    }

    if (joint.state_interfaces[0].name != hardware_interface::HW_IF_POSITION)
    {
      RCLCPP_FATAL(
        rclcpp::get_logger("FDEffortHardwareInterface"),
        "Joint '%s' have %s state interface. '%s' expected.", joint.name.c_str(),
        joint.state_interfaces[0].name.c_str(), hardware_interface::HW_IF_POSITION);
      return hardware_interface::return_type::ERROR;
    }
    if (joint.state_interfaces[1].name != hardware_interface::HW_IF_VELOCITY)
    {
      RCLCPP_FATAL(
        rclcpp::get_logger("FDEffortHardwareInterface"),
        "Joint '%s' have %s state interface. '%s' expected.", joint.name.c_str(),
        joint.state_interfaces[0].name.c_str(), hardware_interface::HW_IF_VELOCITY);
      return hardware_interface::return_type::ERROR;
    }
    if (joint.state_interfaces[2].name != hardware_interface::HW_IF_EFFORT)
    {
      RCLCPP_FATAL(
        rclcpp::get_logger("FDEffortHardwareInterface"),
        "Joint '%s' have %s state interface. '%s' expected.", joint.name.c_str(),
        joint.state_interfaces[0].name.c_str(), hardware_interface::HW_IF_EFFORT);
      return hardware_interface::return_type::ERROR;
    }
  }

  status_ = hardware_interface::status::CONFIGURED;
  return hardware_interface::return_type::OK;
}
  // ------------------------------------------------------------------------------------------
std::vector<hardware_interface::StateInterface>
FDEffortHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  for (uint i = 0; i < info_.joints.size(); i++)
  {
    state_interfaces.emplace_back(hardware_interface::StateInterface(
      info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_states_position_[i]));
      
  }
  for (uint i = 0; i < info_.joints.size(); i++)
  {
    state_interfaces.emplace_back(hardware_interface::StateInterface(
      info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &hw_states_velocity_[i]));
      
  }
  for (uint i = 0; i < info_.joints.size(); i++)
  {
    state_interfaces.emplace_back(hardware_interface::StateInterface(
      info_.joints[i].name, hardware_interface::HW_IF_EFFORT, &hw_states_effort_[i]));
      
  }

  return state_interfaces;
}
  // ------------------------------------------------------------------------------------------
std::vector<hardware_interface::CommandInterface>
FDEffortHardwareInterface::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
  for (uint i = 0; i < info_.joints.size(); i++)
  {
    command_interfaces.emplace_back(hardware_interface::CommandInterface(
      info_.joints[i].name, hardware_interface::HW_IF_EFFORT, &hw_commands_effort_[i]));
  }

  return command_interfaces;
}
  // ------------------------------------------------------------------------------------------
hardware_interface::return_type FDEffortHardwareInterface::start()
{
  RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"), "Starting ...please wait...");

  if(connectToDevice()){
    status_ = hardware_interface::status::STARTED;
    RCLCPP_INFO(
      rclcpp::get_logger("FDEffortHardwareInterface"), "System Successfully started!");
    return hardware_interface::return_type::OK;
  }
  else{
    RCLCPP_ERROR(
      rclcpp::get_logger("FDEffortHardwareInterface"), "System Not started!");
    return hardware_interface::return_type::ERROR;
  }
}
  // ------------------------------------------------------------------------------------------
hardware_interface::return_type FDEffortHardwareInterface::stop()
{
  RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"), "Stopping ...please wait...");

  if(disconnectFromDevice()){
    status_ = hardware_interface::status::STOPPED;
    RCLCPP_INFO(
      rclcpp::get_logger("FDEffortHardwareInterface"), "System successfully stopped!");
    return hardware_interface::return_type::OK;
  }
  else{
    RCLCPP_ERROR(
      rclcpp::get_logger("FDEffortHardwareInterface"), "System Not stopped!");
    return hardware_interface::return_type::ERROR;
  }
}
  // ------------------------------------------------------------------------------------------
hardware_interface::return_type FDEffortHardwareInterface::read()
{  
  // "Success" flag
  int flag = 0;

  // Get position & orientation
  flag += dhdGetPosition(&hw_states_position_[0], &hw_states_position_[1], &hw_states_position_[2], interface_ID_);
  if(dhdHasWrist(interface_ID_) && hw_states_position_.size()>3)
  {
    flag += dhdGetOrientationRad(&hw_states_position_[3],&hw_states_position_[4],&hw_states_position_[5],interface_ID_);
  }
  if(dhdHasGripper(interface_ID_) && hw_states_position_.size()>6)
  {
    flag += dhdGetGripperAngleRad(&hw_states_position_[6],interface_ID_);
  }
  
  // Get velocity
  flag += dhdGetLinearVelocity(&hw_states_velocity_[0], &hw_states_velocity_[1], &hw_states_velocity_[2], interface_ID_);
  if(dhdHasWrist(interface_ID_) && hw_states_velocity_.size()>3)
  {
    flag += dhdGetAngularVelocityRad(&hw_states_velocity_[3], &hw_states_velocity_[4], &hw_states_velocity_[5], interface_ID_);
  }
  if(dhdHasGripper(interface_ID_) && hw_states_velocity_.size()>6)
  {
    flag += dhdGetGripperAngularVelocityRad(&hw_states_velocity_[6],interface_ID_);
  }
  // Get forces  
  double torque[3];
  double gripper_force;  
  flag += dhdGetForceAndTorqueAndGripperForce(&hw_states_effort_[0], &hw_states_effort_[1], &hw_states_effort_[2], &torque[0], &torque[1], &torque[2], &gripper_force, interface_ID_);
  if(dhdHasWrist(interface_ID_) && hw_states_effort_.size()>3)
  {
    hw_states_effort_[3] = torque[0];
    hw_states_effort_[4] = torque[1];
    hw_states_effort_[5] = torque[2];
  }
  if(dhdHasGripper(interface_ID_) && hw_states_effort_.size()>6)
  {
    hw_states_effort_[6] = gripper_force;
  }

  if (flag >= 0) 
    return hardware_interface::return_type::OK;
  else{
    RCLCPP_ERROR(rclcpp::get_logger("FDEffortHardwareInterface"), "Updating from system failed!");
    return hardware_interface::return_type::ERROR;
  }
}
  // ------------------------------------------------------------------------------------------
hardware_interface::return_type FDEffortHardwareInterface::write()
{
  //TODO(mcbed): write to FD system
  bool isNan = false;
  for(auto & command: hw_commands_effort_)
    if(command != command)
      isNan = true;

  if(!isNan){
    if(dhdHasGripper(interface_ID_) && hw_states_effort_.size()>6)
      dhdSetForceAndTorqueAndGripperForce ( hw_commands_effort_[0], hw_commands_effort_[1], hw_commands_effort_[2], 
                                            hw_commands_effort_[3], hw_commands_effort_[4], hw_commands_effort_[5],
                                            hw_commands_effort_[6], interface_ID_);
    else if(dhdHasWrist(interface_ID_) && hw_states_effort_.size()>3)
      dhdSetForceAndTorqueAndGripperForce ( hw_commands_effort_[0], hw_commands_effort_[1], hw_commands_effort_[2],
                                            hw_commands_effort_[3], hw_commands_effort_[4], hw_commands_effort_[5], 
                                            0, interface_ID_);
    else
      dhdSetForceAndTorqueAndGripperForce (hw_commands_effort_[0], hw_commands_effort_[1], hw_commands_effort_[2], 
                                            0, 0, 0, 0, interface_ID_);                                                                                      
  }
  else
    dhdSetForceAndTorqueAndGripperForce (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, interface_ID_);
  

  return hardware_interface::return_type::OK;
}
  // ------------------------------------------------------------------------------------------
bool FDEffortHardwareInterface::connectToDevice(){
  int major, minor, release, revision;
  dhdGetSDKVersion (&major, &minor, &release, &revision);

  // Open connection
  if (dhdOpen () >= 0) {
      RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : %s device detected",dhdGetSystemName()); 

      // Check if the device has 3 dof or more
      if(dhdHasWrist (interface_ID_))
        RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Rotation enabled "); 
      else
        RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Rotation disabled "); 

      // Retrieve the mass of the device
      double effector_mass = 0.0;
      if (dhdGetEffectorMass(&effector_mass,interface_ID_) == DHD_NO_ERROR) {     
          RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Effector Mass = %sg",std::to_string(effector_mass*1000.0)); 
      }
      else {
          RCLCPP_WARN(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Impossible to retrieve effector mass !"); 
      }

      // Set force limit & enable force
      double forceMax = 12; // N
      if (dhdSetMaxForce(forceMax,interface_ID_) < DHD_NO_ERROR) {
          disconnectFromDevice();
      }
      // apply zero force
      dhdSetBrakes( DHD_OFF,interface_ID_);

      if (dhdEnableForce (DHD_ON,interface_ID_) < DHD_NO_ERROR) {
          disconnectFromDevice();
      }
      // Gravity compensation
      if (dhdSetGravityCompensation (DHD_ON,interface_ID_) < DHD_NO_ERROR) {
          RCLCPP_WARN(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Could not enable the gravity compensation !"); 
          disconnectFromDevice();
      }
      else{
          RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Gravity compensation enabled"); 
      }
      RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Device connected !");

      
      if (dhdSetForceAndTorqueAndGripperForce (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,interface_ID_) < DHD_NO_ERROR) {
          disconnectFromDevice();
      }       

      // Sleep 100 ms
      dhdSleep (0.1);
      isConnected_ = true;

      return isConnected_;
  }
  else {
      RCLCPP_ERROR(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Could not connect to device !"); 
      isConnected_ = false;
      return isConnected_;
  }
}
  // ------------------------------------------------------------------------------------------
bool FDEffortHardwareInterface::disconnectFromDevice(){
  RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : stopping the device."); 
  // Stop the device: disables the force on the haptic device and puts it into BRAKE mode.
  int hasStopped = -1;
  while (hasStopped < 0) {
      hasStopped = dhdStop(interface_ID_);
      // Sleep 100 ms
      dhdSleep (0.1);
  }

  // close device connection
  int connectionIsClosed = dhdClose (interface_ID_);
  if (connectionIsClosed >= 0) {
     RCLCPP_INFO(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd :  Disconnected ! "); 
      interface_ID_ = false;
      return true;
  }
  else {
      RCLCPP_ERROR(rclcpp::get_logger("FDEffortHardwareInterface"),"dhd : Failled to disconnect !");     
      return false;
  }
}

}  // namespace fd_hardware
  // ------------------------------------------------------------------------------------------
#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  fd_hardware::FDEffortHardwareInterface, hardware_interface::SystemInterface)   