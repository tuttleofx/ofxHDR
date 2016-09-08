#include "rgbCurve.hpp"
#include <algorithm>
#include <functional>
#include <fstream>
#include <iostream>
#include <sstream>


namespace cameraColorCalibration {
namespace common {
    
rgbCurve::rgbCurve(std::size_t size)
{
  for(auto &curve : _data)
  {
    curve.resize(size);
  }
  setZero();
}

rgbCurve::rgbCurve(const std::string &path)
{
  read(path);
}

void rgbCurve::setLinear() 
{
  const float coefficient = 1.f / static_cast<float>(getSize() - 1);
  for(std::size_t i = 0; i < getSize(); ++i)
  {
    setAllChannels(i, i * coefficient);
  }
}

void rgbCurve::setGamma()
{
  const float coefficient = 1.f / static_cast<float>(getSize() - 1);
  for(std::size_t i = 0; i < getSize(); ++i)
  {
    setAllChannels(i, std::pow(4.0f * i * coefficient, 1.7f) + 1e-4);
  }
}

void rgbCurve::setGaussian(double size)
{
  const float coefficient = 1.f / (static_cast<float>(getSize() - 1) / 4.0f);
  for(std::size_t i = 0; i < getSize(); ++i)
  {
    float factor = i / size * coefficient - 2.0f / size;
    setAllChannels(i, std::exp( -factor * factor ));
  }
}

void rgbCurve::setTriangular()
{
  const float coefficient = 1.f / static_cast<float>(getSize() - 1);
  for(std::size_t i = 0; i < getSize(); ++i)
  {
    float value = i * coefficient * 1.8f + 0.1f;
    if (value >= 1.f)
    {
      value = 2.0f - value;
    }
    setAllChannels(i, value);
  }
}


void rgbCurve::setPlateau()
{
  const float coefficient = 1.f / static_cast<float>(getSize() - 1);
  for(std::size_t i = 0; i < getSize(); ++i)
  {
    setAllChannels(i, 1.0f - std::pow((2.0f * i * coefficient - 1.0f), 12.0f));
  }
}

void rgbCurve::setLog10()
{
  const float coefficient = 1.f / static_cast<float>(getSize() - 1);
  const float logInverseNorm = 1.0f / 0.0625f;
  const float logInverseMaxValue = 1.0f / 1e8;
  for(std::size_t i = 0; i < getSize(); ++i)
  {
    setAllChannels(i, logInverseMaxValue * std::pow(10.0f, (i * coefficient * logInverseNorm) - 8.f));
  }
}

void rgbCurve::inverseAllValues()
{
  for(auto &curve : _data)
  {
    for(auto &value : curve)
    {
      if(value != 0.f)
      {
        value = 1.f / value;
      }
    }
  }
}

void rgbCurve::setAllAbsolute()
{
  for(auto &curve : _data)
  {
    for(auto &value : curve)
    {
      value = std::abs(value);
    }
  }
}

void rgbCurve::normalize()
{
  for(auto &curve : _data)
  {
    std::size_t first = 0;
    std::size_t last = curve.size() - 1;
    
    // find first and last value not null
    for (; (first < curve.size()) && (curve[first] == 0) ; ++first);
    for (; (last > 0) && (curve[last] == 0)  ; --last);

    std::size_t middle = first + ((last - first) / 2);
    float midValue = curve[middle];

    if (midValue == 0.0f)
    {
      // find first non-zero middle response
      for (; (middle < last) && (curve[middle] == 0.0f) ; ++middle);
      midValue = curve[middle];
    }
    
    std::cout << "-> middle [" << middle << "]: " << midValue <<std::endl;
    const float coefficient = 1 / midValue;
    
    for(auto &value : curve)
    {
      value *= coefficient;
    }
  }
}

void rgbCurve::interpolateMissingValues()
{
  for(auto &curve : _data)
  {
    std::size_t previousValidIndex = 0;
    for(std::size_t index = 1; index < curve.size(); ++index)
    {
      if(curve[index] != 0.0f)
      {
        if(previousValidIndex+1 < index)
        {
          const float inter = (curve[index] - curve[previousValidIndex]) / (index - previousValidIndex);
          for(std::size_t j = previousValidIndex+1; j < index; ++j)
          {
            curve[j] = curve[previousValidIndex] + inter * (j-previousValidIndex);
          }
        }
        previousValidIndex = index;
      }
    }
  }
}

float& rgbCurve::operator() (float sample, std::size_t channel)
{
  assert(channel < _data.size());
  return _data[channel][getIndex(sample)];
}

float rgbCurve::operator() (float sample, std::size_t channel) const
{
  assert(channel < _data.size());
  return _data[channel][getIndex(sample)];
}

const rgbCurve rgbCurve::operator+(const rgbCurve &other) const
{
  return sum(other);
}

const rgbCurve rgbCurve::operator-(const rgbCurve &other) const
{
  return subtract(other);
}

void rgbCurve::operator*=(const rgbCurve &other)
{
  multiply(other);
}

const rgbCurve rgbCurve::sum(const rgbCurve &other) const
{
  assert(getSize() == other.getSize());

  rgbCurve sum = rgbCurve(*this);
  for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
  {
    auto &sumCurve = sum.getCurve(channel);
    const auto &otherCurve = other.getCurve(channel);

    //sum member channel by the other channel
    std::transform (sumCurve.begin(), sumCurve.end(), otherCurve.begin(), sumCurve.begin(), std::plus<float>());
  }
  return sum;
}

const rgbCurve rgbCurve::subtract(const rgbCurve &other) const
{
  assert(getSize() == other.getSize());

  rgbCurve sub = rgbCurve(*this);
  for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
  {
    auto &subCurve = sub.getCurve(channel);
    const auto &otherCurve = other.getCurve(channel);

    //subtract member channel by the other channel
    std::transform (subCurve.begin(), subCurve.end(), otherCurve.begin(), subCurve.begin(), std::minus<float>());
  }
  return sub;
}

void rgbCurve::multiply(const rgbCurve &other)
{
  assert(getSize() == other.getSize());  

  for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
  {
    auto &curve = getCurve(channel);
    const auto &otherCurve = other.getCurve(channel);

    //multiply member channel by the other channel
    std::transform (curve.begin(), curve.end(), otherCurve.begin(), curve.begin(), std::multiplies<float>());
  }
}

void rgbCurve::write(const std::string &path, const std::string &name) const
{
  std::ofstream file(path);

  if(!file)
  {
    throw std::logic_error("Can't create curves file");
    return;
  }

  std::string text(name + ",Red,Green,Blue,\n");
  for(std::size_t index = 0; index < getSize(); ++index)
  {
    text += std::to_string(index) + ",";
    for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
    {
      text += std::to_string(_data[channel][index]) + ",";
    }
    text += "\n";
  }
  file << text;
  file.close();
}

void rgbCurve::read(const std::string &path)
{
  std::ifstream file(path);
  std::vector <std::vector <std::string> > fileData;
  
  if(!file)
  {
    throw std::logic_error("Can't open curves file");
    return;
  }
  
  //create fileData
  while (file)
  {
    std::string line;
    if (!getline( file, line )) break;

    std::istringstream sline( line );
    std::vector<std::string> record;
    while (sline)
    {
      std::string value;
      if (!getline( sline, value, ',' )) break;
      record.push_back( value );
    }
    fileData.push_back( record );
  }  
  
  //clear rgbCurve
  for(auto &curve : _data)
  {
    curve.clear();
  }
  
  //fill rgbCurve
  
  try 
  {
    for(std::size_t line = 1; line < fileData.size(); ++line)
    {
      _data[0].push_back(std::stof(fileData[line][1]));
      _data[1].push_back(std::stof(fileData[line][2]));
      _data[2].push_back(std::stof(fileData[line][3]));
    }
  }
  catch(std::exception &e)
  {
    throw std::logic_error("Invalid Curve File");
  }

  file.close();
}

double rgbCurve::sumAll(const rgbCurve &curve)
{
  double sum = 0.0f;
  for(std::size_t channel = 0; channel < curve.getNbChannels(); ++channel)
  {
    auto const &sumCurve = curve.getCurve(channel);

    for(auto value : sumCurve)
    {
      sum += value;
    }
  }
  return sum;
}

} // namespace common
} // namespace cameraColorCalibration