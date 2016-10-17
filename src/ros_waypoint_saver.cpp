#include <ros/ros.h>

#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>

#include <interactive_markers/interactive_marker_server.h>
#include <interactive_markers/menu_handler.h>
#include <visualization_msgs/InteractiveMarkerInit.h>
#include <visualization_msgs/MarkerArray.h>
#include <tf/transform_broadcaster.h>
#include <tf/tf.h>

#include <math.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>

bool compareInteractiveMarker(visualization_msgs::InteractiveMarker left,
                              visualization_msgs::InteractiveMarker right)
{
  return std::stoi(left.name) < std::stoi(right.name);
}

std::string timeToStr()
{
    std::stringstream msg;
    const boost::posix_time::ptime now=
        boost::posix_time::second_clock::local_time();
    boost::posix_time::time_facet *const f=
        new boost::posix_time::time_facet("%Y-%m-%d-%H-%M-%S");
    msg.imbue(std::locale(msg.getloc(),f));
    msg << now;
    return msg.str();
}

class WaypointSaver
{
public:
  WaypointSaver(const std::string& waypoints_file):
    waypoints_file_(waypoints_file), saved_waypoints_(false)
  {
    ros::NodeHandle n;
    
    waypoints_sub_ = n.subscribe("/cube/update_full", 1, &WaypointSaver::pointsCallback, this);
    reached_markers_sub_ = n.subscribe("/reach_threshold_markers", 1, &WaypointSaver::reachedMarkerCallback, this);
    is_reached_markers_ = false;
  }

  void pointsCallback(visualization_msgs::InteractiveMarkerInit all_markers)
  {
    ROS_INFO("Waiting for reach_markers");
    if (! is_reached_markers_) {
      return;
    }
    ROS_INFO("Received markers : %d", (int)all_markers.markers.size());
    if(all_markers.markers.size() != reached_markers_.markers.size())
    {
      ROS_ERROR("markers size is mismatch!!!");
    }
    std::ofstream savefile(waypoints_file_.c_str(), std::ios::out);
    std::sort(all_markers.markers.begin(), all_markers.markers.end(), compareInteractiveMarker);
    size_t size = all_markers.markers.size();
    for(unsigned int i = 0; i < size; i++){
      //３次元の位置の指定
      int is_searching_area = 0;
      if (all_markers.markers[i].controls[2].markers[0].color.r > 1.0) {
        is_searching_area = 1;
      }
      savefile << all_markers.markers[i].pose.position.x << ","
               << all_markers.markers[i].pose.position.y << ","
               << 0 << ","
               << all_markers.markers[i].pose.orientation.x << ","
               << all_markers.markers[i].pose.orientation.y << ","
               << all_markers.markers[i].pose.orientation.z << ","
               << all_markers.markers[i].pose.orientation.w << ","
               << is_searching_area << ","
               << 2.0*reached_markers_.markers[i].scale.x << std::endl;
    }
    saved_waypoints_ = true;
    ROS_INFO_STREAM("Saved to : " << waypoints_file_);
  }

  void reachedMarkerCallback(visualization_msgs::MarkerArray reached_markers)
  {
    ROS_INFO("Received Reach threshold markers : %d", (int)reached_markers.markers.size());
    reached_markers_ = reached_markers;
    is_reached_markers_ = true;
  }
  
  std::string waypoints_file_;
  ros::Subscriber waypoints_sub_;
  bool saved_waypoints_;
  bool is_reached_markers_;
  ros::Subscriber reached_markers_sub_;
  visualization_msgs::MarkerArray reached_markers_;
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "waypoint_saver");
  std::string waypoints_name = timeToStr() + ".csv";
  
  WaypointSaver saver(waypoints_name);
  while(!saver.saved_waypoints_ && ros::ok())
  {
    ros::spinOnce();
  }
  
  return 0;
}
