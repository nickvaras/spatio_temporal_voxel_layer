/*********************************************************************
 *
 * Software License Agreement
 *
 *  Copyright (c) 2018, Simbe Robotics, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Steve Macenski (steven.macenski@simberobotics.com)
 * Purpose: Implement OpenVDB's voxel library with ray tracing for our 
 *          internal voxel grid layer.
 *********************************************************************/

#ifndef VOLUME_GRID_H_
#define VOLUME_GRID_H_

// PCL
#include <pcl_ros/transforms.h>
#include <pcl/PCLPointCloud2.h>
// ROS
#include <ros/ros.h>
// STL
#include <math.h>
#include <unordered_map>
#include <ctime>
#include <iostream>
// msgs
#include <sensor_msgs/PointCloud2.h>
#include <visualization_msgs/Marker.h>
// TBB
#include <tbb/parallel_do.h>
// OpenVDB
#include <openvdb/openvdb.h>
#include <openvdb/tools/GridTransformer.h>
#include <openvdb/tools/RayIntersector.h>
// measurement struct
#include <spatio_temporal_voxel_layer/measurement_buffer.hpp>
#include <spatio_temporal_voxel_layer/frustum.hpp>
// Mutex
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace volume_grid
{

struct occupany_cell
{
  occupany_cell(const double& _x, const double& _y) :
    x(_x), y(_y)
  {
  }

  bool operator==(const occupany_cell& other) const
  {
    return x==other.x && y==other.y;
  }

  double x, y;
};

class LevelSet
{
public:
  typedef openvdb::math::Ray<openvdb::Real> GridRay;
  typedef openvdb::math::Ray<openvdb::Real>::Vec3T Vec3Type;

  LevelSet(const float& voxel_size, const int& background_value, const int& decay_model, \
                            const double& voxel_decay);
  ~LevelSet(void);

  void ParallelizeMark(const std::vector<observation::MeasurementReading>& marking_observations);
  void operator()(const observation::MeasurementReading& obs) const;
  void ParallelizeClearFrustums(const std::vector<observation::MeasurementReading>& clearing_observations);

  void GetOccupancyPointCloud(pcl::PointCloud<pcl::PointXYZ>::Ptr& pc);
  void GetFlattenedCostmap(std::unordered_map<occupany_cell, uint>& cell_map);

  bool ResetLevelSet(void);

protected:
  void InitializeGrid(void);
  bool MarkLevelSetPoint(const openvdb::Coord& pt, const double& value) const;
  bool ClearLevelSetPoint(const openvdb::Coord& pt) const;
  bool IsGridEmpty(void) const;
  double GetDecayTime(void);
  double GetAcceleratedDecayTime(const double& acceleration_factor);

  openvdb::Vec3d WorldToIndex(const openvdb::Vec3d& coord) const;
  openvdb::Vec3d IndexToWorld(const openvdb::Coord& coord) const;

  mutable openvdb::DoubleGrid::Ptr _grid;
  int                             _background_value, _decay_model;
  double                          _voxel_size, _voxel_decay;
  bool                            _pub_voxels;
  pcl::PointCloud<pcl::PointXYZ>::Ptr _pc;
  boost::mutex                    _grid_lock;
};

} // end volume_grid namespace

// hash function for unordered_map of occupancy_cells
namespace std {
template <>
struct hash<volume_grid::occupany_cell>
{
  std::size_t operator()(const volume_grid::occupany_cell& k) const
  {
    return ((std::hash<double>()(k.x) ^ (std::hash<double>()(k.y) << 1)) >> 1);
  }
};

} // end std namespace

#endif