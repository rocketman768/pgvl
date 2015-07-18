/*
 * Image.cpp is part of pgvl and is
 * Copyright 2015 Philip G. Lee <rocketman768@gmail.com>
 */

#include <Image.h>
#include <cmath>
#include <Eigen/Dense>
#include <SDL.h>

SDL_Surface* toSurface(Image<uint8_t>& img) {
   return SDL_CreateRGBSurfaceFrom(
      img[0],
      img.cols(),
      img.rows(),
      8*img.channels(),
      img.rowWidth(),
      0x0000FF,
      0x00FF00,
      0xFF0000,
      0
   );
}

void srgb2rgb(Image<float>& img) {
   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();

   auto convert = [](float& c) -> void {
      if( c <= 0.04045f )
         c /= 12.92f;
      else
         c = powf((c+0.055f)/(1.f+0.055f), 2.4f);
   };

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         for(int k = 0; k < chans; ++k) {
            convert(img[i][j*chans+k]);
         }
      }
   }
}

void rgb2srgb(Image<float>& img) {
   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();

   auto convert = [](float& c) -> void {
      if( c <= 0.0031308f )
         c *= 12.92f;
      else
         c = (1.f+0.055f)*powf(c, 1.f/2.4f) - 0.055f;
   };

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         for(int k = 0; k < chans; ++k) {
            convert(img[i][j*chans+k]);
         }
      }
   }
}

void rgb2xyz(Image<float>& img) {
   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();
   Eigen::Vector3f rgb;
   Eigen::Vector3f xyz;
   Eigen::Matrix3f A;

   A << 0.49f, 0.31f, 0.20f, 0.17697f, 0.81240f, 0.01063f, 0.00f, 0.01f, 0.99f;
   A /= A(1,0);

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         rgb(0) = img[i][j*chans+0];
         rgb(1) = img[i][j*chans+1];
         rgb(2) = img[i][j*chans+2];

         xyz = A*rgb;
         img[i][j*chans+0] = xyz(0);
         img[i][j*chans+1] = xyz(1);
         img[i][j*chans+2] = xyz(2);
      }
   }
}

void xyz2rgb(Image<float>& img) {
   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();
   Eigen::Vector3f rgb;
   Eigen::Vector3f xyz;
   Eigen::Matrix3f A;

   // This is the rgb->xyz matrix. Need its inverse.
   A << 0.49f, 0.31f, 0.20f, 0.17697f, 0.81240f, 0.01063f, 0.00f, 0.01f, 0.99f;
   A /= A(1,0);

   A = A.inverse();

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         xyz(0) = img[i][j*chans+0];
         xyz(1) = img[i][j*chans+1];
         xyz(2) = img[i][j*chans+2];

         rgb = A*xyz;
         img[i][j*chans+0] = rgb(0);
         img[i][j*chans+1] = rgb(1);
         img[i][j*chans+2] = rgb(2);
      }
   }
}

void rgb2hsl(Image<float>& img) {
   float r,g,b;
   float vmax, vmin;
   float h,s,l;

   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         r = img[i][j*chans+0];
         g = img[i][j*chans+1];
         b = img[i][j*chans+2];

         vmax = std::max(r,std::max(g,b));
         vmin = std::min(r,std::min(g,b));
         l = (vmax+vmin)/2.f;

         if( l < 0.5 )
            s = (vmax-vmin)/(vmax+vmin);
         else
            s = (vmax-vmin)/(2.f-(vmax+vmin));

         if( vmax == r ) {
            h = 60.f*(g-b)/s;
         } else if( vmax == g ) {
            h = 120.f+60.f*(b-r)/s;
         } else {
            h = 240.f+60.f*(r-g)/s;
         }

         if( h < 0.f )
            h += 360.f;

         img[i][j*chans+0] = h;
         img[i][j*chans+1] = s;
         img[i][j*chans+2] = l;
      }
   }
}

void hsl2rgb(Image<float>& img) {
   float r,g,b;
   float h,s,l;
   float c, x, m;

   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         h = img[i][j*chans+0];
         s = img[i][j*chans+1];
         l = img[i][j*chans+2];

         c = (1.f - std::abs(2*l-1))*s;
         x = (1.f - std::abs(fmod(h/60.f,2.f) - 1.f))*c;
         m = l - c/2.f;

         r = g = b = m;

         if( h < 60.f ) {
            r += c;
            g += x;
         } else if( h < 120.f ) {
            r += x;
            g += c;
         } else if( h < 180.f ) {
            g += c;
            b += x;
         } else if( h < 240.f ) {
            g += x;
            b += c;
         } else if( h < 300.f ) {
            r += x;
            b += c;
         } else {
            r += c;
            b += x;
         }

         img[i][j*chans+0] = r;
         img[i][j*chans+1] = g;
         img[i][j*chans+2] = b;
      }
   }
}

void rgb2hsv(Image<float>& img) {
   float r,g,b;
   float h,s,v;

   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         r = img[i][j*chans+0];
         g = img[i][j*chans+1];
         b = img[i][j*chans+2];

         v = std::max(r,std::max(g,b));
         if( v < 1e-5 )
            s = 0;
         else
            s = (v - std::min(r,std::min(g,b))) / v;

         if( v == r ) {
            h = 60.f*(g-b)/(v - std::min(r,std::min(g,b)));
         } else if( v == g ) {
            h = 120.f+60.f*(b-r)/(v - std::min(r,std::min(g,b)));
         } else {
            h = 240.f+60.f*(r-g)/(v - std::min(r,std::min(g,b)));
         }

         if( h < 0.f )
            h += 360.f;

         img[i][j*chans+0] = h;
         img[i][j*chans+1] = s;
         img[i][j*chans+2] = v;
      }
   }
}

void hsv2rgb(Image<float>& img) {
   float r,g,b;
   float h,s,v;
   float c, x, m;

   int const rows = img.rows();
   int const cols = img.cols();
   int const chans = img.channels();

   for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
         h = img[i][j*chans+0];
         s = img[i][j*chans+1];
         v = img[i][j*chans+2];

         c = v*s;
         x = (1.f - std::abs(fmod(h/60.f,2.f) - 1.f))*c;
         m = v-c;

         r = g = b = m;

         if( h < 60.f ) {
            r += c;
            g += x;
         } else if( h < 120.f ) {
            r += x;
            g += c;
         } else if( h < 180.f ) {
            g += c;
            b += x;
         } else if( h < 240.f ) {
            g += x;
            b += c;
         } else if( h < 300.f ) {
            r += x;
            b += c;
         } else {
            r += c;
            b += x;
         }

         img[i][j*chans+0] = r;
         img[i][j*chans+1] = g;
         img[i][j*chans+2] = b;
      }
   }
}
