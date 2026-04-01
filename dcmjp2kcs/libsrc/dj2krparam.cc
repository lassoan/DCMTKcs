/*
 *
 *  Copyright (C) 2024, OFFIS e.V.
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation were developed by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module:  dcmjp2kcs
 *
 *  Author:  DCMTK Team
 *
 *  Purpose: representation parameter for JPEG 2000 encoding
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2kcs/dj2krparam.h"

DJ2KRepresentationParameter::DJ2KRepresentationParameter(
    OFBool losslessProcess,
    double compressionRatio)
: DcmRepresentationParameter()
, losslessProcess_(losslessProcess)
, compressionRatio_(compressionRatio)
{
}

DJ2KRepresentationParameter::DJ2KRepresentationParameter(const DJ2KRepresentationParameter& arg)
: DcmRepresentationParameter(arg)
, losslessProcess_(arg.losslessProcess_)
, compressionRatio_(arg.compressionRatio_)
{
}

DJ2KRepresentationParameter::~DJ2KRepresentationParameter()
{
}

DcmRepresentationParameter *DJ2KRepresentationParameter::clone() const
{
  return new DJ2KRepresentationParameter(*this);
}

const char *DJ2KRepresentationParameter::className() const
{
  return "DJ2KRepresentationParameter";
}

OFBool DJ2KRepresentationParameter::operator==(const DcmRepresentationParameter &arg) const
{
  const DJ2KRepresentationParameter *rhs = OFdynamic_cast(const DJ2KRepresentationParameter *, &arg);
  if (rhs == NULL) return OFFalse;
  return (losslessProcess_ == rhs->losslessProcess_) && (compressionRatio_ == rhs->compressionRatio_);
}
