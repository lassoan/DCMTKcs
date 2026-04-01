/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: codec parameter class for JPEG 2000 codecs
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2kcs/dj2kparam.h"

DJ2KCodecParameter::DJ2KCodecParameter(
    J2K_UIDCreation uidCreation,
    J2K_PlanarConfiguration planarConfiguration,
    OFBool ignoreOffsetTable,
    OFBool forceSingleFragmentPerFrame)
: DcmCodecParameter()
, uidCreation_(uidCreation)
, planarConfiguration_(planarConfiguration)
, ignoreOffsetTable_(ignoreOffsetTable)
, forceSingleFragmentPerFrame_(forceSingleFragmentPerFrame)
, createOffsetTable_(OFTrue)
, fragmentSize_(0)
, convertToSC_(OFFalse)
{
}

DJ2KCodecParameter::DJ2KCodecParameter(
    OFBool createOffsetTable,
    Uint32 fragmentSize,
    J2K_UIDCreation uidCreation,
    OFBool convertToSC)
: DcmCodecParameter()
, uidCreation_(uidCreation)
, planarConfiguration_(EJ2KPC_restore)
, ignoreOffsetTable_(OFFalse)
, forceSingleFragmentPerFrame_(OFFalse)
, createOffsetTable_(createOffsetTable)
, fragmentSize_(fragmentSize)
, convertToSC_(convertToSC)
{
}

DJ2KCodecParameter::DJ2KCodecParameter(const DJ2KCodecParameter& arg)
: DcmCodecParameter(arg)
, uidCreation_(arg.uidCreation_)
, planarConfiguration_(arg.planarConfiguration_)
, ignoreOffsetTable_(arg.ignoreOffsetTable_)
, forceSingleFragmentPerFrame_(arg.forceSingleFragmentPerFrame_)
, createOffsetTable_(arg.createOffsetTable_)
, fragmentSize_(arg.fragmentSize_)
, convertToSC_(arg.convertToSC_)
{
}

DJ2KCodecParameter::~DJ2KCodecParameter()
{
}

DcmCodecParameter *DJ2KCodecParameter::clone() const
{
  return new DJ2KCodecParameter(*this);
}

const char *DJ2KCodecParameter::className() const
{
  return "DJ2KCodecParameter";
}
