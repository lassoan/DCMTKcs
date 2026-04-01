/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: singleton class that registers decoders for all supported JPEG 2000 processes.
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2kcs/dj2kdecode.h"
#include "dcmtk/dcmdata/dccodec.h"  /* for DcmCodecStruct */
#include "dcmtk/dcmjp2kcs/dj2kparam.h"
#include "dcmtk/dcmjp2kcs/dj2kcodecd.h"

#ifdef WITH_OPENJPEG
#include <openjpeg.h>
#endif

// initialization of static members
OFBool DJ2KDecoderRegistration::registered_            = OFFalse;
DJ2KCodecParameter *DJ2KDecoderRegistration::cp_       = NULL;
DJ2KLosslessDecoder *DJ2KDecoderRegistration::losslessDecoder_ = NULL;
DJ2KDecoder *DJ2KDecoderRegistration::decoder_         = NULL;

void DJ2KDecoderRegistration::registerCodecs(
    J2K_UIDCreation uidcreation,
    J2K_PlanarConfiguration planarconfig,
    OFBool ignoreOffsetTable,
    OFBool forceSingleFragmentPerFrame)
{
  if (! registered_)
  {
    cp_ = new DJ2KCodecParameter(uidcreation, planarconfig, ignoreOffsetTable, forceSingleFragmentPerFrame);
    if (cp_)
    {
      losslessDecoder_ = new DJ2KLosslessDecoder();
      if (losslessDecoder_) DcmCodecList::registerCodec(losslessDecoder_, NULL, cp_);

      decoder_ = new DJ2KDecoder();
      if (decoder_) DcmCodecList::registerCodec(decoder_, NULL, cp_);
      registered_ = OFTrue;
    }
  }
}

void DJ2KDecoderRegistration::cleanup()
{
  if (registered_)
  {
    DcmCodecList::deregisterCodec(losslessDecoder_);
    DcmCodecList::deregisterCodec(decoder_);
    delete losslessDecoder_;
    delete decoder_;
    delete cp_;
    registered_ = OFFalse;
#ifdef DEBUG
    // not needed but useful for debugging purposes
    losslessDecoder_ = NULL;
    decoder_ = NULL;
    cp_ = NULL;
#endif
  }
}

OFString DJ2KDecoderRegistration::getLibraryVersionString()
{
#ifdef WITH_OPENJPEG
    OFString versionStr = "OpenJPEG, Version ";
    versionStr += opj_version();
    return versionStr;
#else
    return "OpenJPEG library not available";
#endif
}
