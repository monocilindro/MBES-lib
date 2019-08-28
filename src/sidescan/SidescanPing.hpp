/*
* Copyright 2019 © Centre Interdisciplinaire de développement en Cartographie des Océans (CIDCO), Tous droits réservés
*/

/* 
 * File:   SidescanPing.hpp
 * Author: glm
 *
 * Created on August 28, 2019, 5:34 PM
 */

#ifndef SIDESCANPING_HPP
#define SIDESCANPING_HPP

#include <vector>

class SidescanPing {
public:
    SidescanPing();
    SidescanPing(const SidescanPing& orig);
    virtual ~SidescanPing();
    
    void setDistancePerSample(double d){distancePerSample = d;};
    double getDistancePerSample(){ return distancePerSample;};
    
    
    std::vector<double> & getSamples(){ return samples;};
    
    void setSamples(std::vector<double> & s){
        samples = s;
    }
    
    int getChannelNumber(){ return channelNumber;};
    void setChannelNumber(int channel){ channelNumber = channel;};
    
private:
    std::vector<double> samples; //we will boil down all the types to double. This is not a pretty hack, but we need to support every sample type
    double distancePerSample;
    int channelNumber;
};

#endif /* SIDESCANPING_HPP */

