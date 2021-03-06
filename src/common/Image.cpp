#include "Image.hpp"
#include <cassert>
#include <iostream>


namespace cameraColorCalibration {
namespace common {

template<typename DataType>
Image<DataType>::Image()
{
}

template<typename DataType>
Image<DataType>::Image(std::size_t width, std::size_t height, std::size_t nbChannels)
{
  createInternalBuffer(width, height, nbChannels);
}

template<typename DataType>
Image<DataType>::Image(OFX::Image *imgData)
{
  setOfxImage(imgData);
}

//  template<typename DataType>
//  Image<DataType>::Image(const Image &other)
//  {
//    if(other._hasOwnership || other._imgPtr)
//    {
//      throw std::logic_error("Don't duplicate the full image buffer.");
//    }
//    _imgPtr = other._imgPtr;
//    _data = other._data;
//    _hasOwnership = other._hasOwnership;
//    _width = other._width;
//    _height = other._height;
//    _nbChannels = other._nbChannels;
//    _size = other._size;
//    _nbPixels = other._nbPixels;
//    _rowBufferSize = other._rowBufferSize;
//    _channelQuantization = other._channelQuantization;
//  }

template<typename DataType>
Image<DataType>::~Image()
{
  if(!_hasOwnership)
  {
    delete _imgPtr;
  }
  clear();
}
  
template<typename DataType>
void Image<DataType>::setOfxImage(OFX::Image *imgData)
{
  std::size_t width = imgData->getRegionOfDefinition().x2 - imgData->getRegionOfDefinition().x1;
  std::size_t height = imgData->getRegionOfDefinition().y2 - imgData->getRegionOfDefinition().y1;
  
  //assert(width > 0);
  //assert(height > 0);
  
  //copy the pointer for delete
  _imgPtr = imgData;
 
  setExternalBuffer((DataType*)imgData->getPixelData(), width, height, imgData->getPixelComponentCount(), imgData->getRowBytes() / sizeof(DataType));
}

template<typename DataType>
void Image<DataType>::createInternalBuffer(std::size_t width, std::size_t height, std::size_t nbChannels)
{
  _hasOwnership = true; //important
  clear();
  _hasOwnership = true;
  _nbChannels = nbChannels;
  _width = width;
  _height = height;
  _size = width * height * nbChannels;
  _data = new DataType[_size];
  _nbPixels = width * height;
  _rowBufferSize = width * nbChannels;
}

template<typename DataType>
void Image<DataType>::setExternalBuffer(DataType *data, std::size_t width, std::size_t height, std::size_t channels, std::size_t rowBufferSize)
{
  _hasOwnership = false; //important
  clear();
  _data = data;
  _hasOwnership = false;
  _width = width;
  _height = height;
  _nbChannels = channels;
  _size = width * height * channels;
  _nbPixels = width * height;
  _rowBufferSize = rowBufferSize;
}

template<typename DataType>
void Image<DataType>::clear()
{
  if(_hasOwnership)
  {
    delete _data;
  }
  _data = nullptr;
  _hasOwnership = 0;
  _width = 0;
  _height = 0;
  _size = 0;
  _nbPixels = 0;
}

template<typename DataType>
void Image<DataType>::setZero()
{
  for(std::size_t y = 0; y < getHeight(); ++y)
  {
    for(std::size_t x = 0; x < getWidth(); ++x)
    {
      DataType *ptr = getPixel(x, y);

      for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
      {
        *ptr = 0;
        
        ++ptr;
      }
    }
  }
}

template<typename DataType>
void Image<DataType>::setRed()
{
  for(std::size_t y = 0; y < getHeight(); ++y)
  {
    for(std::size_t x = 0; x < getWidth(); ++x)
    {
      DataType *ptr = getPixel(x, y);
      *ptr = 1;
      ++ptr;
      for(std::size_t channel = 1; channel < getNbChannels(); ++channel)
      {
        *ptr = 0;
        
        ++ptr;
      }
    }
  }
}

template<typename DataType>
void Image<DataType>::multiply(const Image<DataType> &other)
{
  assert(this->getSize() == other.getSize());

  for(std::size_t y = 0; y < getHeight(); ++y)
  {
    for(std::size_t x = 0; x < getWidth(); ++x)
    {
      DataType *ptr = getPixel(x, y);
      const DataType *otherPtr = other.getPixel(x, y);

      for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
      {
        *ptr *= *otherPtr;
        
        ++ptr;
        ++otherPtr;
      }
    }
  }
}

template<typename DataType>
void Image<DataType>::multiply(float coefficient)
{
  for(std::size_t y = 0; y < getHeight(); ++y)
  {
    for(std::size_t x = 0; x < getWidth(); ++x)
    {
      DataType *ptr = getPixel(x, y);

      for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
      {
        *ptr *= coefficient;
        
        ++ptr;
      }
    }
  }
}

template<typename DataType>
void Image<DataType>::divide(const Image &other)
{
  assert(this->getSize() == other.getSize());

  for(std::size_t y = 0; y < getHeight(); ++y)
  {
    for(std::size_t x = 0; x < getWidth(); ++x)
    {
      DataType *ptr = getPixel(x, y);
      const DataType *otherPtr = other.getPixel(x, y);

      for(std::size_t channel = 0; channel < getNbChannels(); ++channel)
      {
        if(*otherPtr == 0)
        {
          *ptr = 0;
        }
        else
        {
          *ptr = *ptr / *otherPtr;
        }
        
        ++ptr;
        ++otherPtr;
      }
    }
  }
}

template<typename DataType>
void Image<DataType>::copyFrom(const Image &other)
{
  assert(this->getHeight() == other.getHeight());
  assert(this->getWidth() == other.getWidth());
  assert(this->getNbChannels() >= other.getNbChannels());
  
  this->setZero();
  
  for(std::size_t y = 0; y < getHeight(); ++y)
  {
    for(std::size_t x = 0; x < getWidth(); ++x)
    {
      DataType *ptr = getPixel(x, y);
      const DataType *otherPtr = other.getPixel(x, y);

      for(std::size_t channel = 0; channel < other.getNbChannels(); ++channel)
      {
        *ptr = *otherPtr;

        ++ptr;
        ++otherPtr;
      }
    }
  }
}

template<typename DataType>
void Image<DataType>::checkSameDimensions(const std::vector< Image<DataType> > &images)
{
  if(images.empty())
  {
    throw std::logic_error("Image group is empty");
  }
  for(auto const &image : images)
  {
    if((image.getWidth() != images[0].getWidth())
            || (image.getHeight() != images[0].getHeight()))
    {
      throw std::logic_error("images have different sizes");
    }
  }
}

template class Image<float>;

} // namespace common
} // namespace cameraColorCalibration