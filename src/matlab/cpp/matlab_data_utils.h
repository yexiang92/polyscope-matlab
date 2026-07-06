#pragma once

#include "mex_io_helpers.h"

#include "Eigen/Dense"

#include <string>
#include <unordered_map>
#include <vector>

namespace ps_mex {

using matlab::data::Array;
using matlab::data::ArrayType;
using matlab::data::TypedArray;

// Throw a MATLAB error from C++
void throwError(matlab::engine::MATLABEngine* matlabPtr, const std::string& msg);

// Convert a MATLAB char/string scalar to std::string
std::string getString(const Array& arr);

// Convert a MATLAB cell array of strings to std::vector<std::string>
std::vector<std::string> getStringVector(const Array& arr);

// Scalar conversions
bool getScalarBool(const Array& arr);
double getScalarDouble(const Array& arr);
float getScalarFloat(const Array& arr);
int getScalarInt(const Array& arr);

// Convert a numeric MATLAB array to std::vector<float>, in column-major order
std::vector<float> getVectorFloat(const Array& arr);
std::vector<double> getVectorDouble(const Array& arr);
std::vector<int> getVectorInt(const Array& arr);
std::vector<uint32_t> getVectorUInt32(const Array& arr);

// Convert a 2D numeric MATLAB array to Eigen::MatrixXf (column-major like MATLAB)
// If expectedCols >= 0, validates the second dimension.
Eigen::MatrixXf getMatrixFloat(const Array& arr, int expectedCols = -1);
Eigen::MatrixXd getMatrixDouble(const Array& arr, int expectedCols = -1);
Eigen::MatrixXi getMatrixInt(const Array& arr, int expectedCols = -1);

// Convert a 1D numeric MATLAB array to Eigen::VectorXf
Eigen::VectorXf getEigenVectorFloat(const Array& arr);
Eigen::VectorXd getEigenVectorDouble(const Array& arr);

// Helpers to create MATLAB output arrays
matlab::data::TypedArray<double> createScalarDouble(matlab::data::ArrayFactory& factory, double val);
matlab::data::TypedArray<bool> createScalarBool(matlab::data::ArrayFactory& factory, bool val);
matlab::data::TypedArray<double> createVectorDouble(matlab::data::ArrayFactory& factory,
                                                    const std::vector<double>& vals);
matlab::data::TypedArray<double> createVec2(matlab::data::ArrayFactory& factory, double x, double y);
matlab::data::TypedArray<double> createVec4(matlab::data::ArrayFactory& factory, double x, double y, double z, double w);

// Read 1x2 / 1x3 / 1x4 numeric arrays as glm-like vectors
Eigen::Vector2f getVec2(const Array& arr);
Eigen::Vector3f getVec3(const Array& arr);
Eigen::Vector4f getVec4(const Array& arr);
Eigen::Vector2d getVec2d(const Array& arr);
Eigen::Vector4d getVec4d(const Array& arr);
matlab::data::TypedArray<double> createMatrixDouble(matlab::data::ArrayFactory& factory,
                                                    const Eigen::MatrixXd& mat);
matlab::data::TypedArray<int32_t> createMatrixInt(matlab::data::ArrayFactory& factory,
                                                  const Eigen::MatrixXi& mat);
matlab::data::TypedArray<uint8_t> createUint8Array(matlab::data::ArrayFactory& factory,
                                                   const std::vector<uint8_t>& data,
                                                   const std::vector<size_t>& dims);

// Parse trailing name-value pairs from an ArgumentList.
// Example: inputs = {cmd, name, points, 'radius', 0.1, 'enabled', true}
// startIdx is the index of the first option name.
class OptionsParser {
public:
  OptionsParser(matlab::mex::ArgumentList& inputs, size_t startIdx,
                matlab::engine::MATLABEngine* matlabPtr);

  bool has(const std::string& name) const;
  const Array& get(const std::string& name) const;

  bool getBool(const std::string& name, bool defaultVal) const;
  double getDouble(const std::string& name, double defaultVal) const;
  float getFloat(const std::string& name, float defaultVal) const;
  int getInt(const std::string& name, int defaultVal) const;
  std::string getString(const std::string& name, const std::string& defaultVal) const;
  Eigen::Vector3f getVec3(const std::string& name, const Eigen::Vector3f& defaultVal) const;

private:
  std::unordered_map<std::string, Array> opts_;
  matlab::engine::MATLABEngine* matlabPtr_;
};

} // namespace ps_mex
