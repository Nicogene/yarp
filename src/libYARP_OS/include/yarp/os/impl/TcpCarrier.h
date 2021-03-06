/*
 * Copyright (C) 2006 RobotCub Consortium
 * Authors: Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */

#ifndef YARP_OS_IMPL_TCPCARRIER_H
#define YARP_OS_IMPL_TCPCARRIER_H

#include <yarp/os/AbstractCarrier.h>

namespace yarp {
    namespace os {
        namespace impl {
            class TcpCarrier;
        }
    }
}

/**
 * Communicating between two ports via TCP.
 */
class yarp::os::impl::TcpCarrier : public AbstractCarrier
{
public:

    TcpCarrier(bool requireAckFlag = true);

    virtual Carrier *create() override;

    virtual ConstString getName() override;

    virtual int getSpecifierCode();

    virtual bool checkHeader(const yarp::os::Bytes& header) override;
    virtual void getHeader(const yarp::os::Bytes& header) override;
    virtual void setParameters(const yarp::os::Bytes& header) override;
    virtual bool requireAck() override;
    virtual bool isConnectionless() override;
    virtual bool respondToHeader(yarp::os::ConnectionState& proto) override;
    virtual bool expectReplyToHeader(yarp::os::ConnectionState& proto) override;

private:
    bool requireAckFlag;
};

#endif // YARP_OS_IMPL_TCPCARRIER_H
