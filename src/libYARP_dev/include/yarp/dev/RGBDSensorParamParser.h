/*
 * Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/os/Searchable.h>
#include <yarp/sig/Matrix.h>
#include <yarp/dev/api.h>

#include <vector>

#ifndef YARP_DEV_RGBDSENSORPARAMPARSER_H
#define YARP_DEV_RGBDSENSORPARAMPARSER_H

namespace yarp {
namespace dev {

class YARP_dev_API RGBDSensorParamParser
{
public:
    struct YARP_dev_API IntrinsicParams
    {
        struct YARP_dev_API plum_bob
        {
            double k1;
            double k2;
            double t1;
            double t2;
            double k3;
            plum_bob(): k1(0.0), k2(0.0),
                        t1(0.0), t2(0.0),
                        k3(0.0) {}
        };
        double   principalPointX;
        double   principalPointY;
        double   focalLengthX;
        double   focalLengthY;
        plum_bob distortionModel;
        bool     isOptional;
        IntrinsicParams(): principalPointX(0.0), principalPointY(0.0),
                           focalLengthX(0.0), focalLengthY(0.0),
                           distortionModel(), isOptional(false) {}

        IntrinsicParams(const yarp::os::Searchable &intrinsic)
        {
            yAssert(intrinsic.check("focalLengthX")    &&
                    intrinsic.check("focalLengthY")    &&
                    intrinsic.check("principalPointX") &&
                    intrinsic.check("principalPointY"));
            focalLengthX    = intrinsic.find("focalLengthX").asFloat64();
            focalLengthY    = intrinsic.find("focalLengthY").asFloat64();
            principalPointX = intrinsic.find("principalPointX").asFloat64();
            principalPointY = intrinsic.find("principalPointY").asFloat64();
            // The distortion parameters are optional
            distortionModel.k1 = intrinsic.check("k1", yarp::os::Value(0.0)).asFloat64();
            distortionModel.k2 = intrinsic.check("k2", yarp::os::Value(0.0)).asFloat64();
            distortionModel.t1 = intrinsic.check("t1", yarp::os::Value(0.0)).asFloat64();
            distortionModel.t2 = intrinsic.check("t2", yarp::os::Value(0.0)).asFloat64();
            distortionModel.k3 = intrinsic.check("k3", yarp::os::Value(0.0)).asFloat64();

        }

        void toProperty(yarp::os::Property& intrinsic) const
        {
            intrinsic.put("focalLengthX",       focalLengthX);
            intrinsic.put("focalLengthY",       focalLengthY);
            intrinsic.put("principalPointX",    principalPointX);
            intrinsic.put("principalPointY",    principalPointY);

            intrinsic.put("distortionModel", "plumb_bob");
            intrinsic.put("k1", distortionModel.k1);
            intrinsic.put("k2", distortionModel.k2);
            intrinsic.put("t1", distortionModel.t1);
            intrinsic.put("t2", distortionModel.t2);
            intrinsic.put("k3", distortionModel.k3);

            intrinsic.put("stamp", yarp::os::Time::now());
        }
    };
    struct YARP_dev_API RGBDParam
    {
        RGBDParam() : name("unknown"), isSetting(false), isDescription(false), size(1)
        {
            val.resize(size);
        }

        RGBDParam(const std::string& _name, const int _size) : name(_name), isSetting(false),
                                              isDescription(false), size(_size)
        {
            val.resize(size);
        }


        YARP_SUPPRESS_DLL_INTERFACE_WARNING_ARG(std::string) name;
        bool         isSetting;
        bool         isDescription;
        size_t          size;

        YARP_SUPPRESS_DLL_INTERFACE_WARNING_ARG(std::vector<yarp::os::Value>) val;
    };

    IntrinsicParams         depthIntrinsic;
    IntrinsicParams         rgbIntrinsic;
    yarp::sig::Matrix       transformationMatrix;
    bool                    isOptionalExtrinsic;

    RGBDSensorParamParser(): depthIntrinsic(), rgbIntrinsic(),
                             transformationMatrix(4,4), isOptionalExtrinsic(true) {
        transformationMatrix.eye();
    }
    bool parseParam(yarp::os::Searchable& config, std::vector<RGBDParam *> &params);
};

} // dev
} // yarp

#endif




