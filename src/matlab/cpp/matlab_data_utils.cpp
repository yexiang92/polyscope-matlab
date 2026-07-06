#include "matlab_data_utils.h"

#include <sstream>

namespace ps_mex {

void throwError(matlab::engine::MATLABEngine* matlabPtr, const std::string& msg) {
  matlab::data::ArrayFactory factory;
  matlabPtr->feval(u"error", 0,
                   std::vector<matlab::data::Array>({factory.createScalar(msg)}));
}

std::string getString(const Array& arr) {
  if (arr.getType() != ArrayType::MATLAB_STRING && arr.getType() != ArrayType::CHAR) {
    throw std::runtime_error("Expected string or char array");
  }
  if (arr.getType() == ArrayType::MATLAB_STRING) {
    matlab::data::TypedArray<matlab::data::MATLABString> strArr(arr);
    if (strArr.getNumberOfElements() != 1) {
      throw std::runtime_error("Expected scalar string");
    }
    return std::string(strArr[0]);
  } else {
    matlab::data::CharArray charArr(arr);
    return charArr.toAscii();
  }
}

std::vector<std::string> getStringVector(const Array& arr) {
  if (arr.getType() != ArrayType::CELL) {
    throw std::runtime_error("Expected cell array of strings");
  }
  matlab::data::CellArray cellArr(arr);
  std::vector<std::string> out;
  out.reserve(cellArr.getNumberOfElements());
  for (const auto& elem : cellArr) {
    out.push_back(getString(elem));
  }
  return out;
}

bool getScalarBool(const Array& arr) {
  if (arr.getType() != ArrayType::LOGICAL) {
    throw std::runtime_error("Expected logical scalar");
  }
  matlab::data::TypedArray<bool> logicalArr(arr);
  if (logicalArr.getNumberOfElements() != 1) {
    throw std::runtime_error("Expected scalar logical");
  }
  return logicalArr[0];
}

double getScalarDouble(const Array& arr) {
  if (arr.getType() != ArrayType::DOUBLE) {
    throw std::runtime_error("Expected double scalar");
  }
  matlab::data::TypedArray<double> darr(arr);
  if (darr.getNumberOfElements() != 1) {
    throw std::runtime_error("Expected scalar double");
  }
  return darr[0];
}

float getScalarFloat(const Array& arr) {
  return static_cast<float>(getScalarDouble(arr));
}

int getScalarInt(const Array& arr) {
  if (arr.getType() == ArrayType::DOUBLE) {
    return static_cast<int>(getScalarDouble(arr));
  } else if (arr.getType() == ArrayType::INT32) {
    matlab::data::TypedArray<int32_t> iarr(arr);
    if (iarr.getNumberOfElements() != 1) {
      throw std::runtime_error("Expected scalar int");
    }
    return iarr[0];
  } else if (arr.getType() == ArrayType::INT64) {
    matlab::data::TypedArray<int64_t> iarr(arr);
    if (iarr.getNumberOfElements() != 1) {
      throw std::runtime_error("Expected scalar int");
    }
    return static_cast<int>(iarr[0]);
  }
  throw std::runtime_error("Expected scalar integer");
}

namespace {

template <typename T>
std::vector<T> copyNumericVector(const Array& arr) {
  ArrayType t = arr.getType();
  if (t == ArrayType::DOUBLE) {
    matlab::data::TypedArray<double> view(arr);
    std::vector<T> out;
    out.reserve(view.getNumberOfElements());
    for (auto& v : view) {
      out.push_back(static_cast<T>(v));
    }
    return out;
  } else if (t == ArrayType::SINGLE) {
    matlab::data::TypedArray<float> view(arr);
    std::vector<T> out;
    out.reserve(view.getNumberOfElements());
    for (auto& v : view) {
      out.push_back(static_cast<T>(v));
    }
    return out;
  } else if (t == ArrayType::INT32) {
    matlab::data::TypedArray<int32_t> view(arr);
    std::vector<T> out;
    out.reserve(view.getNumberOfElements());
    for (auto& v : view) {
      out.push_back(static_cast<T>(v));
    }
    return out;
  } else if (t == ArrayType::UINT32) {
    matlab::data::TypedArray<uint32_t> view(arr);
    std::vector<T> out;
    out.reserve(view.getNumberOfElements());
    for (auto& v : view) {
      out.push_back(static_cast<T>(v));
    }
    return out;
  } else if (t == ArrayType::INT64) {
    matlab::data::TypedArray<int64_t> view(arr);
    std::vector<T> out;
    out.reserve(view.getNumberOfElements());
    for (auto& v : view) {
      out.push_back(static_cast<T>(v));
    }
    return out;
  } else {
    throw std::runtime_error("Unsupported numeric array type");
  }
}

} // namespace

std::vector<float> getVectorFloat(const Array& arr) { return copyNumericVector<float>(arr); }
std::vector<double> getVectorDouble(const Array& arr) { return copyNumericVector<double>(arr); }
std::vector<int> getVectorInt(const Array& arr) { return copyNumericVector<int>(arr); }
std::vector<uint32_t> getVectorUInt32(const Array& arr) { return copyNumericVector<uint32_t>(arr); }

namespace {

template <typename T>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> copyNumericMatrix(const Array& arr, int expectedCols) {
  auto dims = arr.getDimensions();
  if (dims.size() != 2) {
    throw std::runtime_error("Expected 2D numeric array");
  }
  size_t rows = dims[0];
  size_t cols = dims[1];
  if (expectedCols >= 0 && static_cast<int>(cols) != expectedCols) {
    std::ostringstream oss;
    oss << "Expected array with " << expectedCols << " columns, got " << cols;
    throw std::runtime_error(oss.str());
  }

  Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> mat(rows, cols);
  // Eigen default is column-major, same as MATLAB. We can copy element-wise.
  auto data = copyNumericVector<T>(arr);
  if (data.size() != rows * cols) {
    throw std::runtime_error("Internal size mismatch");
  }
  std::copy(data.begin(), data.end(), mat.data());
  return mat;
}

} // namespace

Eigen::MatrixXf getMatrixFloat(const Array& arr, int expectedCols) {
  return copyNumericMatrix<float>(arr, expectedCols);
}
Eigen::MatrixXd getMatrixDouble(const Array& arr, int expectedCols) {
  return copyNumericMatrix<double>(arr, expectedCols);
}
Eigen::MatrixXi getMatrixInt(const Array& arr, int expectedCols) {
  return copyNumericMatrix<int>(arr, expectedCols);
}

Eigen::VectorXf getEigenVectorFloat(const Array& arr) {
  auto v = getVectorFloat(arr);
  Eigen::VectorXf out(v.size());
  std::copy(v.begin(), v.end(), out.data());
  return out;
}

Eigen::VectorXd getEigenVectorDouble(const Array& arr) {
  auto v = getVectorDouble(arr);
  Eigen::VectorXd out(v.size());
  std::copy(v.begin(), v.end(), out.data());
  return out;
}

matlab::data::TypedArray<double> createScalarDouble(matlab::data::ArrayFactory& factory, double val) {
  return factory.createScalar(val);
}

matlab::data::TypedArray<bool> createScalarBool(matlab::data::ArrayFactory& factory, bool val) {
  return factory.createScalar(val);
}

matlab::data::TypedArray<double> createVectorDouble(matlab::data::ArrayFactory& factory,
                                                    const std::vector<double>& vals) {
  auto arr = factory.createArray<double>({vals.size(), 1});
  for (size_t i = 0; i < vals.size(); ++i) {
    arr[i] = vals[i];
  }
  return arr;
}

matlab::data::TypedArray<double> createVec2(matlab::data::ArrayFactory& factory, double x, double y) {
  auto arr = factory.createArray<double>({1, 2});
  arr[0] = x;
  arr[1] = y;
  return arr;
}

matlab::data::TypedArray<double> createVec4(matlab::data::ArrayFactory& factory, double x, double y, double z,
                                            double w) {
  auto arr = factory.createArray<double>({1, 4});
  arr[0] = x;
  arr[1] = y;
  arr[2] = z;
  arr[3] = w;
  return arr;
}

namespace {

template <typename T, int N>
Eigen::Matrix<T, N, 1> getVecN(const Array& arr) {
  auto m = getMatrixDouble(arr, N);
  if (m.rows() != 1 && m.cols() != 1) {
    throw std::runtime_error("Expected a 1xN or Nx1 vector");
  }
  Eigen::Matrix<T, N, 1> out;
  if (m.rows() == 1) {
    for (int i = 0; i < N; ++i) out(i) = static_cast<T>(m(0, i));
  } else {
    for (int i = 0; i < N; ++i) out(i) = static_cast<T>(m(i, 0));
  }
  return out;
}

} // namespace

Eigen::Vector2f getVec2(const Array& arr) { return getVecN<float, 2>(arr); }
Eigen::Vector3f getVec3(const Array& arr) { return getVecN<float, 3>(arr); }
Eigen::Vector4f getVec4(const Array& arr) { return getVecN<float, 4>(arr); }
Eigen::Vector2d getVec2d(const Array& arr) { return getVecN<double, 2>(arr); }
Eigen::Vector4d getVec4d(const Array& arr) { return getVecN<double, 4>(arr); }

matlab::data::TypedArray<double> createMatrixDouble(matlab::data::ArrayFactory& factory,
                                                    const Eigen::MatrixXd& mat) {
  std::vector<double> data;
  data.reserve(static_cast<size_t>(mat.rows() * mat.cols()));
  // MATLAB arrays are column-major; pack data column by column.
  for (int j = 0; j < mat.cols(); ++j) {
    for (int i = 0; i < mat.rows(); ++i) {
      data.push_back(mat(i, j));
    }
  }
  return factory.createArray<double>({static_cast<size_t>(mat.rows()), static_cast<size_t>(mat.cols())},
                                     data.data(), data.data() + data.size());
}

matlab::data::TypedArray<int32_t> createMatrixInt(matlab::data::ArrayFactory& factory,
                                                  const Eigen::MatrixXi& mat) {
  std::vector<int32_t> data;
  data.reserve(static_cast<size_t>(mat.rows() * mat.cols()));
  for (int j = 0; j < mat.cols(); ++j) {
    for (int i = 0; i < mat.rows(); ++i) {
      data.push_back(static_cast<int32_t>(mat(i, j)));
    }
  }
  return factory.createArray<int32_t>({static_cast<size_t>(mat.rows()), static_cast<size_t>(mat.cols())},
                                      data.data(), data.data() + data.size());
}

matlab::data::TypedArray<uint8_t> createUint8Array(matlab::data::ArrayFactory& factory,
                                                   const std::vector<uint8_t>& data,
                                                   const std::vector<size_t>& dims) {
  auto arr = factory.createArray<uint8_t>(dims);
  for (size_t i = 0; i < data.size(); ++i) {
    arr[i] = data[i];
  }
  return arr;
}

// ------------------------------------------------------------------
// OptionsParser
// ------------------------------------------------------------------
OptionsParser::OptionsParser(matlab::mex::ArgumentList& inputs, size_t startIdx,
                             matlab::engine::MATLABEngine* matlabPtr)
    : matlabPtr_(matlabPtr) {
  if ((inputCount(inputs) - startIdx) % 2 != 0) {
    throw std::runtime_error("Options must be specified as name-value pairs");
  }
  for (size_t i = startIdx; i < inputCount(inputs); i += 2) {
    std::string name = ps_mex::getString(getInput(inputs, i));
    opts_[name] = getInput(inputs, i + 1);
  }
}

bool OptionsParser::has(const std::string& name) const { return opts_.count(name) > 0; }

const Array& OptionsParser::get(const std::string& name) const {
  auto it = opts_.find(name);
  if (it == opts_.end()) {
    throw std::runtime_error("Missing option: " + name);
  }
  return it->second;
}

bool OptionsParser::getBool(const std::string& name, bool defaultVal) const {
  if (!has(name)) return defaultVal;
  return getScalarBool(get(name));
}

double OptionsParser::getDouble(const std::string& name, double defaultVal) const {
  if (!has(name)) return defaultVal;
  return getScalarDouble(get(name));
}

float OptionsParser::getFloat(const std::string& name, float defaultVal) const {
  if (!has(name)) return defaultVal;
  return getScalarFloat(get(name));
}

int OptionsParser::getInt(const std::string& name, int defaultVal) const {
  if (!has(name)) return defaultVal;
  return getScalarInt(get(name));
}

std::string OptionsParser::getString(const std::string& name, const std::string& defaultVal) const {
  if (!has(name)) return defaultVal;
  return ps_mex::getString(get(name));
}

Eigen::Vector3f OptionsParser::getVec3(const std::string& name, const Eigen::Vector3f& defaultVal) const {
  if (!has(name)) return defaultVal;
  auto mat = getMatrixFloat(get(name), 3);
  if (mat.rows() != 1) {
    throw std::runtime_error("Expected 1x3 vector for option " + name);
  }
  return mat.row(0);
}

} // namespace ps_mex
