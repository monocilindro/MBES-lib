/*
* Copyright 2019 © Centre Interdisciplinaire de développement en Cartographie des Océans (CIDCO), Tous droits réservés
*/

#ifndef POSITION_HPP
#define POSITION_HPP

#include <iostream>
#include "utils/Constants.hpp"
#include <cmath>
#include <Eigen/Dense>

/*!
* \brief Position class
* \author Guillaume Labbe-Morissette, Jordan McManus, Emile Gagne
* \date September 13, 2018, 3:29 PM
*/
class Position {
public:

  /**
  * Creates a position
  *
  * @param microEpoch time value calculated since January 1970 (micro-second)
  * @param latitude the latitude of the position
  * @param longitude the longitude of the position
  * @param ellipsoidalHeight the ellipsoidal height of the position
  */
  Position(uint64_t microEpoch,double latitude, double longitude, double ellipsoidalHeight) :
  timestamp(microEpoch),
  slat(sin(latitude * D2R)),
  clat(cos(latitude * D2R)),
  slon(sin(longitude * D2R)),
  clon(cos(longitude * D2R)),
  vector(latitude,longitude,ellipsoidalHeight)
  {}

    /**Destroys the position*/
    ~Position() {
    }

    /**Returns the timestamp of the position*/
    uint64_t getTimestamp()	const	{ return timestamp; }

    /**
    * Sets the timestamp of the position
    *
    * @param e the new timestamp value
    */
    void     setTimestamp(uint64_t e)	{ timestamp = e;}

    /**Returns the latitude of the position*/
    double   getLatitude()	const	{ return vector(0); }

    /**
    * Sets the latitude of the position
    *
    * @param l the new latitude
    */
    void     setLatitude(double l)	{ vector(0)=l; slat=sin(vector(0) * D2R); clat=cos(vector(0) * D2R);}

    /**Returns the longitude of the position*/
    double   getLongitude()	const	{ return vector(1); }

    /**
    * Sets the longitude of the position
    *
    * @param l the new longitude
    */
    void     setLongitude(double l)     { vector(1)=l; slon=sin(vector(1) * D2R);clon=cos(vector(1) * D2R);}

    /**Returns the ellipsoidal height of the position*/
    double   getEllipsoidalHeight()   const  	{ return vector(2); }

    /**
    * Sets the ellipsoidal height of the position
    *
    * @param h the new ellipsoidal height
    */
    void     setEllipsoidalHeight(double h) 	{ vector(2)=h;}

    /**Returns the sine value of the latitude*/
    double   getSlat()	const	{ return slat; }

    /**Returns the sine value of the longitude*/
    double   getSlon()	const	{ return slon; }

    /**Returns the cosine value of the latitude*/
    double   getClat()	const	{ return clat; }

    /**Returns the cosine value of the longitude*/
    double   getClon()	const	{ return clon; }

    /**Returns the vectorized form of the position*/
    Eigen::Vector3d & getVector() { return vector;}

    static bool sortByTimestamp(Position & p1,Position & p2){
      return p1.getTimestamp() < p2.getTimestamp();
    }

  private:

    /**Timestamp value of the position (micro-second)*/
    uint64_t timestamp;

    /*Trigonometry is stored to prevent redundant recalculations*/

    /**Sine value of the latitude*/
    double slat;

    /**Cosine value of the latitude*/
    double clat;

    /**Sine value of the longitude*/
    double slon;

    /**Cosine value of the longitude*/
    double clon;

    /**Position vector in WGS84*/
    Eigen::Vector3d vector;

    /**
    * Returns a text value that contains the informations of the position
    *
    * @param os the text value that contains the most informations of the position
    * @param obj the position that we need to get the informations
    */
    friend std::ostream& operator<<(std::ostream& os, const Position& obj) {
      return os << "( " << obj.vector(0) << " , " << obj.vector(1) << " , " << obj.vector(2) << " )";
    }
  };

  #endif /* POSITION_HPP */
