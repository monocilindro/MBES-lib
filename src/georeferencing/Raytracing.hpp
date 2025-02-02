/*
* Copyright 2019 © Centre Interdisciplinaire de développement en Cartographie des Océans (CIDCO), Tous droits réservés
*/

#ifndef RAYTRACING_HPP
#define RAYTRACING_HPP

#include <vector>
#include "../svp/SoundVelocityProfile.hpp"
#include "../Ping.hpp"
#include "../math/CoordinateTransform.hpp"


/*!
 * \brief Raytracing class
 * \author Guillaume Labbe-Morissette, Rabine Keyetieu
 */
class Raytracing{
public:
    
    // gradient inferior to this epsilon is considered 0
    // TODO: find a physically significant value for epsilon
    static constexpr double gradientEpsilon = 0.000001; 
    
    static void constantCelerityRayTracing(double z0, double z1, double c, double snellConstant, double & deltaZ, double & deltaR, double & deltaTravelTime) {
        double cosBn   = snellConstant*c;
        double sinBn   = sqrt(1 - pow(cosBn, 2));
        
        deltaZ = z1 - z0;
        deltaTravelTime = deltaZ/(c*sinBn);
        deltaR = cosBn*deltaTravelTime*c;
    }
    
    static void lastLayerPropagation(double lastLayerTraveTime, double c_lastLayer, double snellConstant, double & deltaZ, double & deltaR) {
        double cosBn   = snellConstant*c_lastLayer;
        double sinBn   = sqrt(1 - pow(cosBn, 2));
        deltaR = c_lastLayer*lastLayerTraveTime*cosBn;
        deltaZ = c_lastLayer*lastLayerTraveTime*sinBn;
    }
    
    static void constantGradientRayTracing(double c0, double c1, double gradient, double snellConstant, double & deltaZ, double & deltaR, double & deltaTravelTime) {
        double cosBnm1 = snellConstant*c0;
        double cosBn   = snellConstant*c1;
        double sinBnm1 = sqrt(1 - pow(cosBnm1, 2));
        double sinBn   = sqrt(1 - pow(cosBn, 2));
        
        double radiusOfCurvature = 1.0/(snellConstant*gradient);
        
        deltaTravelTime = abs((1./abs(gradient))*log((c1/c0)*((1.0 + sinBnm1)/(1.0 + sinBn))));
        deltaZ = radiusOfCurvature*(cosBn - cosBnm1);
        deltaR = radiusOfCurvature*(sinBnm1 - sinBn);
    }
    
    static double soundSpeedGradient(double z0, double c0, double z1, double c1) {
        if(z1 == z0) {
            //this happens when svp contains multiple entries at same depth
            std::stringstream ss;
            ss << "Can't calculate gradient for svp samples at same depth: z0=" << z0 << " z1=" << z1;
            throw std::invalid_argument(ss.str());
        }
        
        return (c1 - c0) / (z1 - z0);
    }
    
    static void launchVectorParameters(Ping & ping, Eigen::Matrix3d & boresightMatrix,Eigen::Matrix3d & imu2nav, double & sinAz, double & cosAz, double & beta0) {
        /*
	 * Compute launch vector
         */
	Eigen::Vector3d launchVectorSonar; //in sonar frame
	CoordinateTransform::sonar2cartesian(launchVectorSonar,ping.getAlongTrackAngle(),ping.getAcrossTrackAngle(), 1.0 );
        
	launchVectorSonar.normalize();
        
	//convert to navigation frame where the raytracing occurs
	Eigen::Vector3d launchVectorNav = imu2nav * (boresightMatrix * launchVectorSonar);

        double vNorm = sqrt(pow(launchVectorNav(0), 2)  + pow(launchVectorNav(1), 2));
        
        //NED convention
	sinAz = (vNorm >0)?launchVectorNav(0)/ vNorm : 0;
	cosAz = (vNorm >0)?launchVectorNav(1)/ vNorm : 0;
	beta0 = asin(launchVectorNav(2));
    }
    
    /**
     * Makes a raytracing
     *
     * @param raytracedPing the raytraced ping for the raytracing
     * @param ping the Ping for the raytracing
     * @param svp the SoundVelocityProfile for the raytracing
     */
    static void rayTrace(Eigen::Vector3d & raytracedPing,Ping & ping,SoundVelocityProfile & svp, Eigen::Matrix3d & boresightMatrix,Eigen::Matrix3d & imu2nav){
        
	double sinAz;
	double cosAz;
	double beta0;
        launchVectorParameters(ping, boresightMatrix, imu2nav, sinAz, cosAz, beta0);
        
        double currentLayerRaytraceTime = 0;
        double currentLayerDeltaZ = 0;
        double currentLayerDeltaR = 0;
        
        double cumulativeRaytraceTime = 0;
        double cumulativeRayX = 0;
        double cumulativeRayZ = 0;
        
        double oneWayTravelTime = (ping.getTwoWayTravelTime()/(double)2);
        
        Eigen::VectorXd depths = svp.getDepths();
        Eigen::VectorXd speeds = svp.getSpeeds();
        Eigen::VectorXd gradient = svp.getSoundSpeedGradient();

        
        //Snell's law's coefficient, using sound speed at transducer
        double snellConstant = cos(beta0)/ping.getSurfaceSoundSpeed();
        unsigned int svpCutoffIndex = svp.getLayerIndexForDepth(ping.getTransducerDepth());
        
        if(svpCutoffIndex < svp.getSize()) {
            // transducer depth is within svp bounds
            //Ray tracing in first layer using sound speed at transducer
            double gradientTransducerSvp = soundSpeedGradient(ping.getTransducerDepth(), ping.getSurfaceSoundSpeed(), depths(svpCutoffIndex), speeds(svpCutoffIndex));
        
            if(abs(gradientTransducerSvp) < gradientEpsilon) {
                constantCelerityRayTracing(
                    ping.getTransducerDepth(),
                    depths(svpCutoffIndex),
                    ping.getSurfaceSoundSpeed(),
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            } else {
                constantGradientRayTracing(
                    ping.getSurfaceSoundSpeed(),
                    speeds(svpCutoffIndex),
                    gradientTransducerSvp,
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            }

            //To ensure to work with the currentLayerIndex-1 cumulated travel time
            if (cumulativeRaytraceTime + currentLayerRaytraceTime <= oneWayTravelTime)
            {
                cumulativeRayX += currentLayerDeltaR;
                cumulativeRayZ += currentLayerDeltaZ;
                cumulativeRaytraceTime += currentLayerRaytraceTime;
            }
        } else {
            // transducer is below deepest SVP sample
            // do nothing, last layer ray tracing after loop
        }
        
        unsigned int currentLayerIndex = svpCutoffIndex;
        while( (cumulativeRaytraceTime + currentLayerRaytraceTime)<=oneWayTravelTime //ray tracing time must be smaller than oneWayTravelTime
                && (currentLayerIndex<svp.getSize()-1) // stop before last layer
        ) {
            if (abs(gradient(currentLayerIndex)) < gradientEpsilon)
            {
                constantCelerityRayTracing(
                    depths(currentLayerIndex),
                    depths(currentLayerIndex+1),
                    speeds(currentLayerIndex),
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            }
            else {
                constantGradientRayTracing(
                    speeds(currentLayerIndex),
                    speeds(currentLayerIndex+1),
                    gradient(currentLayerIndex),
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            }

            //To ensure to work with the currentLayerIndex-1 cumulated travel time
            if (cumulativeRaytraceTime + currentLayerRaytraceTime <=  oneWayTravelTime)
            {
                currentLayerIndex++;
                cumulativeRayX += currentLayerDeltaR;
                cumulativeRayZ += currentLayerDeltaZ;
                cumulativeRaytraceTime += currentLayerRaytraceTime;
            } else {
                break; // this layer's travel time causes overshoot for onewaytraveltime
            }
        }
       
        // Last Layer Propagation
        double c_lastLayer;
        if(svpCutoffIndex < svp.getSize()) {
            c_lastLayer = speeds(currentLayerIndex);
        } else {
            // The transducer is deeper than last SVP sample
            c_lastLayer = ping.getSurfaceSoundSpeed();
        }
        
        double lastLayerTraveTime = oneWayTravelTime - cumulativeRaytraceTime;
        double dxf;
        double dzf;
        
        lastLayerPropagation(lastLayerTraveTime, c_lastLayer, snellConstant, dzf, dxf);

        // Output variable computation
        double Xf = cumulativeRayX + dxf;
        double Zf = cumulativeRayZ + dzf;

        // re-orient ray in navigation frame
        raytracedPing(0) = Xf*sinAz;
        raytracedPing(1) = Xf*cosAz;
        raytracedPing(2) = Zf;
    }
    
    /**
     * Raytracing in a plane x, z
     *
     * @param raytracedPing the raytraced ping in 2D
     * @param layerRays the rays for each layer
     * @param layerTravelTimes the travel times for each layer
     * @param ping the Ping for the raytracing
     * @param svp the SoundVelocityProfile for the raytracing
     */
    static void planarRayTrace(Eigen::Vector2d & raytracedPing, std::vector<Eigen::Vector2d> & layerRays, std::vector<double> & layerTravelTimes, Ping & ping, SoundVelocityProfile & svp, Eigen::Matrix3d & boresightMatrix,Eigen::Matrix3d & imu2nav){
        
	double sinAz;
	double cosAz;
	double beta0;
        launchVectorParameters(ping, boresightMatrix, imu2nav, sinAz, cosAz, beta0);
        
        double currentLayerRaytraceTime = 0;
        double currentLayerDeltaZ = 0;
        double currentLayerDeltaR = 0;
        
        double cumulativeRaytraceTime = 0;
        double cumulativeRayX = 0;
        double cumulativeRayZ = 0;
        
        double oneWayTravelTime = (ping.getTwoWayTravelTime()/(double)2);
        
        Eigen::VectorXd depths = svp.getDepths();
        Eigen::VectorXd speeds = svp.getSpeeds();
        Eigen::VectorXd gradient = svp.getSoundSpeedGradient();

        
        //Snell's law's coefficient, using sound speed at transducer
        double snellConstant = cos(beta0)/ping.getSurfaceSoundSpeed();
        unsigned int svpCutoffIndex = svp.getLayerIndexForDepth(ping.getTransducerDepth());
        
        if(svpCutoffIndex < svp.getSize()) {
            // transducer depth is within svp bounds
            //Ray tracing in first layer using sound speed at transducer
            double gradientTransducerSvp = soundSpeedGradient(ping.getTransducerDepth(), ping.getSurfaceSoundSpeed(), depths(svpCutoffIndex), speeds(svpCutoffIndex));
        
            if(abs(gradientTransducerSvp) < gradientEpsilon) {
                constantCelerityRayTracing(
                    ping.getTransducerDepth(),
                    depths(svpCutoffIndex),
                    ping.getSurfaceSoundSpeed(),
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            } else {
                constantGradientRayTracing(
                    ping.getSurfaceSoundSpeed(),
                    speeds(svpCutoffIndex),
                    gradientTransducerSvp,
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            }

            //To ensure to work with the currentLayerIndex-1 cumulated travel time
            if (cumulativeRaytraceTime + currentLayerRaytraceTime <= oneWayTravelTime)
            {
                cumulativeRayX += currentLayerDeltaR;
                cumulativeRayZ += currentLayerDeltaZ;
                cumulativeRaytraceTime += currentLayerRaytraceTime;
                
                Eigen::Vector2d layerRay;
                layerRay << currentLayerDeltaR, currentLayerDeltaZ;
                layerRays.push_back(layerRay);
                
                layerTravelTimes.push_back(currentLayerRaytraceTime);
            }
        } else {
            // transducer is below deepest SVP sample
            // do nothing, last layer ray tracing after loop
        }
        
        unsigned int currentLayerIndex = svpCutoffIndex;
        while( (cumulativeRaytraceTime + currentLayerRaytraceTime)<=oneWayTravelTime //ray tracing time must be smaller than oneWayTravelTime
                && (currentLayerIndex<svp.getSize()-1) // stop before last layer
        ) {
            if (abs(gradient(currentLayerIndex)) < gradientEpsilon)
            {
                constantCelerityRayTracing(
                    depths(currentLayerIndex),
                    depths(currentLayerIndex+1),
                    speeds(currentLayerIndex),
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            }
            else {
                constantGradientRayTracing(
                    speeds(currentLayerIndex),
                    speeds(currentLayerIndex+1),
                    gradient(currentLayerIndex),
                    snellConstant,
                    currentLayerDeltaZ,
                    currentLayerDeltaR,
                    currentLayerRaytraceTime
                );
            }

            //To ensure to work with the currentLayerIndex-1 cumulated travel time
            if (cumulativeRaytraceTime + currentLayerRaytraceTime <=  oneWayTravelTime)
            {
                currentLayerIndex++;
                cumulativeRayX += currentLayerDeltaR;
                cumulativeRayZ += currentLayerDeltaZ;
                cumulativeRaytraceTime += currentLayerRaytraceTime;
                
                Eigen::Vector2d layerRay;
                layerRay << currentLayerDeltaR, currentLayerDeltaZ;
                layerRays.push_back(layerRay);
                
                layerTravelTimes.push_back(currentLayerRaytraceTime);
            } else {
                break; // this layer's travel time causes overshoot for onewaytraveltime
            }
        }
       
        // Last Layer Propagation
        double c_lastLayer;
        if(svpCutoffIndex < svp.getSize()) {
            c_lastLayer = speeds(currentLayerIndex);
        } else {
            // The transducer is deeper than last SVP sample
            c_lastLayer = ping.getSurfaceSoundSpeed();
        }
        
        double lastLayerTraveTime = oneWayTravelTime - cumulativeRaytraceTime;
        double dxf;
        double dzf;
        
        lastLayerPropagation(lastLayerTraveTime, c_lastLayer, snellConstant, dzf, dxf);
        
        // Output variable computation
        Eigen::Vector2d lastLayerRay;
        lastLayerRay << dxf, dzf;
        layerRays.push_back(lastLayerRay);
        layerTravelTimes.push_back(lastLayerTraveTime);
        
        double Xf = cumulativeRayX + dxf;
        double Zf = cumulativeRayZ + dzf;
        
        raytracedPing(0) = Xf;
        raytracedPing(1) = Zf;
    }
};

#endif
