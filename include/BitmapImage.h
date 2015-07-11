/*
 * BitmapImage.h is part of pgvl and is
 * Copyright 2015 Philip G. Lee <rocketman768@gmail.com>
 */

#ifndef BITMAPIMAGE_H
#define BITMAPIMAGE_H

#include <string>
#include <regex>
#include <inttypes.h>
#include <stdlib.h>
#include <ppm.h>
#include "config.h"

/*!
 * \brief An image class for bitmaps
 *
 * For efficiency, each row is aligned to \c CACHE_LINE_SIZE byte boundaries.
 * \tparam T the type of an individual channel
 */
template<class T>
class BitmapImage {
public:

   //! \brief Default constructor
   BitmapImage(
      int rows = 0,
      int cols = 0,
      int channels = 0
   )
   : _data(0),
     _unalignedData(0),
     _channelWidth(sizeof(T)),
     _rows(0),
     _cols(0),
     _channels(0),
     _rowWidth(0)
   {
      resize(rows, cols, channels);
   }
   ~BitmapImage() {
      delete[] _unalignedData;
   }

   //! \brief Copy constructor
   BitmapImage(BitmapImage const& other) :
      _data(0),
      _unalignedData(0),
      _channelWidth(other._channelWidth),
      _rows(0),
      _cols(0),
      _channels(0),
      _rowWidth(0)
   {
      resize(other._rows, other._cols, other._channels);
      for( int i = 0; i < _rows; ++i ) {
         memcpy(_data + i*_rowWidth, other._data + i*other._rowWidth, _channels*_cols);
      }
   }
   /*!
    * \brief Constructor from image filename
    *
    * \param filename a .ppm or a .pgm image
    */
   BitmapImage(std::string const& filename) :
      _data(0),
      _unalignedData(0),
      _channelWidth(sizeof(T)),
      _rows(0),
      _cols(0),
      _channels(0),
      _rowWidth(0)
   {
      int maxval;
      uint8_t* rawData = 0;

      switch( fileType(filename) ) {
      case FILETYPE_PGM:
         rawData = pgmread(filename.c_str(), &_rows, &_cols);
         _channels = 1;
         break;
      case FILETYPE_PPM:
         rawData = ppmread(filename.c_str(), &_rows, &_cols, &maxval);
         _channels = 3;
         break;
      default:
         LOGE("Bad image format");
         return;
      }

      // Make the row width a multiple of CACHE_LINE_SIZE
      _rowWidth = _channels*_channelWidth*_cols;
      if( _rowWidth % CACHE_LINE_SIZE )
         _rowWidth += CACHE_LINE_SIZE - (_rowWidth % CACHE_LINE_SIZE);

      // Make each row line up with CACHE_LINE_SIZE
      _unalignedData = new uint8_t[_rowWidth*_rows + CACHE_LINE_SIZE];
      _data = _unalignedData;
      if( reinterpret_cast<size_t>(_data) % CACHE_LINE_SIZE )
         _data += CACHE_LINE_SIZE - (reinterpret_cast<size_t>(_data) % CACHE_LINE_SIZE);

      // Copy the data to aligned memory
      for( int i = 0; i < _rows; ++i ) {
         memcpy(_data + i*_rowWidth, rawData + i*_channels*_cols, _channels*_cols);
      }

      delete[] rawData;
   }

   //! \brief Assignment operator
   BitmapImage<T>& operator=(BitmapImage<T> const& rhs)
   {
      // Don't self-assign
      if( this == &rhs )
         return *this;

      resize(rhs._rows, rhs._cols, rhs._channels);
      for( int i = 0; i < _rows; ++i ) {
         memcpy(_data + i*_rowWidth, rhs._data + i*rhs._rowWidth, _channels*_cols);
      }

      return *this;
   }

   //! \brief Number of rows in the image
   int rows() const { return _rows; }
   //! \brief Number of columns in the image
   int cols() const { return _cols; }
   //! \brief Number of channels in the image
   int channels() const { return _channels; }
   //! \brief Number of bytes in a row of pixels
   int rowWidth() const { return _rowWidth; }

   /*!
    * \brief Resize the image
    *
    * Destructively resize the image.
    * \param nRows number of rows
    * \param nCols number of columns
    * \param nChans number of channels
    */
   void resize(int nRows, int nCols, int nChans) {
      delete[] _unalignedData;
      _unalignedData = 0;

      _rows = nRows;
      _cols = nCols;
      _channels = nChans;

      // Make the row width a multiple of CACHE_LINE_SIZE
      _rowWidth = _channels*_channelWidth*_cols;
      if( _rowWidth % CACHE_LINE_SIZE )
         _rowWidth += CACHE_LINE_SIZE - (_rowWidth % CACHE_LINE_SIZE);

      // Make each row line up with CACHE_LINE_SIZE
      _unalignedData = new uint8_t[_rowWidth*_rows + CACHE_LINE_SIZE];
      _data = _unalignedData;
      if( reinterpret_cast<size_t>(_data) % CACHE_LINE_SIZE )
         _data += CACHE_LINE_SIZE - (reinterpret_cast<size_t>(_data) % CACHE_LINE_SIZE);
   }

   /*!
    * \brief Save the file
    *
    * If the number of channels is 1, it appends a .pgm extension and writes.
    * If the number of channels is 3, it appends a .ppm extension and writes.
    *
    * \param basename the base filename without extension
    */
   void save(std::string const& basename) const {
      if( _channelWidth != 1 ) {
         LOGE("Channel width must be 8 bits");
         return;
      }

      uint8_t* rawData = new uint8_t[_rows*_cols*_channels];

      for( int i = 0; i < _rows; ++i ) {
         memcpy(rawData + i*_channels*_cols, _data + i*_rowWidth, _channels*_cols);
      }

      std::string filename(basename);

      switch( _channels ) {
      case 1:
         filename += ".pgm";
         pgmwrite(filename.c_str(), _cols, _rows, rawData, "", 1);
         break;
      case 3:
         // TODO: implement ppmwrite()
         //filename += ".ppm";
         //ppmwrite(filename.c_str(), _cols, _rows, rawData, "", 1);
         LOGE("PPM writing not yet supported");
         break;
      default:
         LOGE("Bad image format");
         break;
      }

      delete[] rawData;
   }

   //! \brief Pointer to the ith row of pixel data
   T* operator[](size_t i) { return reinterpret_cast<T*>(_data + i*_rowWidth); }
   //! \brief Pointer to the ith row of pixel data (const version)
   T const* operator[](size_t i) const { return reinterpret_cast<T const*>(_data + i*_rowWidth); }

   /*!
    * \brief Extract a patch from the image
    *
    * If the specified region is outside the image boundary, the output is a
    * size 0 image.
    *
    * \param[out] out the output patch
    * \param[in] left the left boundary
    * \param[in] right the right boundary
    * \param[in] top the upper boundary
    * \param[in] bottom the lower boundary
    */
   void patch(BitmapImage<T>& out, int left, int right, int top, int bottom) {
      if( left > right ||
         top > bottom ||
         left < 0 ||
         right >= _cols ||
         top < 0 ||
         bottom >= _rows ) {
         out.resize(0,0,0);
         return;
      }

      out.resize(bottom-top+1, right-left+1, _channels);
      for(int i = top; i <= bottom; ++i) {
         memcpy(out._data + (i-top)*out._rowWidth, _data + i*_rowWidth + left*_channels, (right-left+1)*_channels);
      }
   }

private:

   uint8_t* _data;
   uint8_t* _unalignedData;
   int _channelWidth;
   int _rows;
   int _cols;
   int _channels;
   int _rowWidth;

   enum FileType { FILETYPE_NONE, FILETYPE_PGM, FILETYPE_PPM };
   static FileType fileType(std::string const& filename) {
      std::regex ppm(".*[.]ppm$");
      std::regex pgm(".*[.]pgm$");

      if( std::regex_match(filename, pgm) )
         return FILETYPE_PGM;
      else if( std::regex_match(filename, ppm) )
         return FILETYPE_PPM;
      else
         return FILETYPE_NONE;
   }
};

#endif /*BITMAPIMAGE_H*/
