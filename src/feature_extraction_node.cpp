#include "feature_extraction/feature_extraction_node.h"

FeatureExtractionNode::FeatureExtractionNode()
{

  ros::NodeHandle nh("~");

  ///////////////////////////////////
  /* Feature Extraction Parameters */
  ///////////////////////////////////
  nh.param("usc_max_radius", searchRadius, 1.0);
  nh.param("usc_min_radius", minRadius, 0.05);
  // nh.param("usc_number_radius_bins", numRadiusBins, 10);
  nh.param("usc_point_density_radius", pointDensityRadius, 0.5);
  nh.param("usc_local_radius", localRadius, 0.5);


  nh.param("z_min", zMin, 2.0);
  nh.param("z_max", zMax, 2.0);
  
  std::cout << "Cloud Filter Parameters:" << std::endl;
  std::cout << "    z_min: " << zMin << std::endl;
  std::cout << "    z_max: " << zMax << std::endl;

  ////////////////////////////////
  /* ROS Publishers/Subscribers */
  ////////////////////////////////
  pc_sub = nh.subscribe ("/velodyne_points", 0, &FeatureExtractionNode::cloudCallback, this);
  imu_sub = nh.subscribe ("/xsens/data", 0, &FeatureExtractionNode::imuCallback, this);

  kp_pub = nh.advertise<PointCloud> ("keypoints", 0);
  filt_pub = nh.advertise<PointCloud> ("cloud_filt", 0);

}

FeatureExtractionNode::~FeatureExtractionNode()
{
  std::cout << "FeatureExtractionNode::~FeatureExtractionNode" << std::endl;
}

void FeatureExtractionNode::imuCallback (const sensor_msgs::ImuConstPtr& msg)
{
  tf::Quaternion quat;
  tf::quaternionMsgToTF(msg->orientation, quat);

  // the tf::Quaternion has a method to acess roll pitch and yaw
  double yaw,tmproll;
  tf::Matrix3x3(quat).getRPY(tmproll, pitch, yaw);
  roll = tmproll - M_PI;

  // std::cout << "roll = " << roll*180/3.14 << "pitch = " << pitch*180/3.14 << std::endl;

}

void FeatureExtractionNode::cloudCallback (const sensor_msgs::PointCloud2ConstPtr& msg)
{
  // definitions
  pcl::PCLPointCloud2 cloud2;
  PointCloud::Ptr cloud_in(new PointCloud);
  PointCloud::Ptr keypoints(new PointCloud());
  //pcl::PointCloud<pcl::SHOT352>::Ptr descriptors(new pcl::PointCloud<pcl::SHOT352>());

  // sensor_msgs::PointCloud2 to pcl::PCLPointCloud2
  pcl_conversions::toPCL(*msg,cloud2);

  // pcl::PCLPointCloud2 to PointCloud
  pcl::fromPCLPointCloud2(cloud2,*cloud_in);

  
  PointCloud::Ptr cloud(new PointCloud);
  rotateCloud(*cloud_in,cloud);

  filterCloud(cloud);

  cloud->header.frame_id = msg->header.frame_id;

  
  filt_pub.publish (cloud);

  
  // Object for storing the USC descriptors for each point.
  // pcl::UniqueShapeContext thissdfasd;
  //pcl::PointCloud<pcl::UniqueShapeContext1960>::Ptr descriptors(new pcl::PointCloud<pcl::UniqueShapeContext1960>());
  /*
  // USC estimation object.
  pcl::UniqueShapeContext<pcl::PointXYZ, pcl::UniqueShapeContext1960, pcl::ReferenceFrame> usc;
  usc.setInputCloud(cloud);
  // Search radius, to look for neighbors. It will also be the radius of the support sphere.
  usc.setRadiusSearch(searchRadius);
  // The minimal radius value for the search sphere, to avoid being too sensitive
  // in bins close to the center of the sphere.
  usc.setMinimalRadius(minRadius);
  // Radius used to compute the local point density for the neighbors
  // (the density is the number of points within that radius).
  usc.setPointDensityRadius(pointDensityRadius);
  // Set the radius to compute the Local Reference Frame.
  usc.setLocalRadius(localRadius);
  usc.compute(*descriptors);

  // kp_pub.publish (keypoints);
  */

}

void FeatureExtractionNode::rotateCloud (const PointCloud &cloud, PointCloud::Ptr transformed_cloud)
{

  Eigen::Affine3f transform = Eigen::Affine3f::Identity();

  // Define a translation of 0.0 meters on the x, y, and z axis.
  transform.translation() << 0.0, 0.0, 0.0;

  // The rotation matrix theta radians arround Z axis
  transform.rotate (  Eigen::AngleAxisf (pitch, Eigen::Vector3f::UnitY())*
                      Eigen::AngleAxisf (roll, Eigen::Vector3f::UnitX())   );

  // Executing the transformation
  pcl::transformPointCloud (cloud, *transformed_cloud, transform);
}

void FeatureExtractionNode::filterCloud (PointCloud::Ptr cloud){

// Filter object.
pcl::PassThrough<Point> filter;
filter.setInputCloud(cloud);
filter.setFilterFieldName("z");
filter.setFilterLimits(zMin, zMax);
filter.filter(*cloud);

filter.setFilterLimitsNegative(true);
filter.setFilterFieldName("x");
filter.setFilterLimits(-131.0, 1.0);
filter.filter(*cloud);

filter.setFilterFieldName("y");
filter.setFilterLimits(-1.0, 1.0);
filter.filter(*cloud);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
  // Initialize ROS
  ros::init (argc, argv, "feature_extraction_node");
  FeatureExtractionNode node;

  // Spin
  ros::spin ();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
