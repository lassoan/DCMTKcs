/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: singleton class that registers encoders for all supported JPEG 2000 processes.
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2kcs/dj2kencode.h"
#include "dcmtk/dcmdata/dccodec.h"  /* for DcmCodecList */
#include "dcmtk/dcmjp2kcs/dj2kparam.h"
#include "dcmtk/dcmjp2kcs/dj2kcodece.h"

#ifdef WITH_OPENJPEG
#include <openjpeg.h>
#endif

// initialization of static members
OFBool DJ2KEncoderRegistration::registered_                = OFFalse;
DJ2KCodecParameter *DJ2KEncoderRegistration::cp_           = NULL;
DJ2KLosslessEncoder *DJ2KEncoderRegistration::losslessEncoder_ = NULL;
DJ2KEncoder *DJ2KEncoderRegistration::encoder_             = NULL;

void DJ2KEncoderRegistration::registerCodecs(
    OFBool createOffsetTable,
    Uint32 fragmentSize,
    J2K_UIDCreation uidcreation,
    OFBool convertToSC)
{
  if (! registered_)
  {
    cp_ = new DJ2KCodecParameter(createOffsetTable, fragmentSize, uidcreation, convertToSC);
    if (cp_)
    {
      losslessEncoder_ = new DJ2KLosslessEncoder();
      if (losslessEncoder_) DcmCodecList::registerCodec(losslessEncoder_, NULL, cp_);

      encoder_ = new DJ2KEncoder();
      if (encoder_) DcmCodecList::registerCodec(encoder_, NULL, cp_);
      registered_ = OFTrue;
    }
  }
}

void DJ2KEncoderRegistration::cleanup()
{
  if (registered_)
  {
    DcmCodecList::deregisterCodec(losslessEncoder_);
    DcmCodecList::deregisterCodec(encoder_);
    delete losslessEncoder_;
    delete encoder_;
    delete cp_;
    registered_ = OFFalse;
#ifdef DEBUG
    // not needed but useful for debugging purposes
    losslessEncoder_ = NULL;
    encoder_ = NULL;
    cp_ = NULL;
#endif
  }
}

OFString DJ2KEncoderRegistration::getLibraryVersionString()
{
#ifdef WITH_OPENJPEG
    OFString versionStr = "OpenJPEG, Version ";
    versionStr += opj_version();
    return versionStr;
#else
    return "OpenJPEG library not available";
#endif
}
