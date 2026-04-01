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
 *  Purpose: singleton class that registers the encoder for all supported JPEG 2000 processes.
 *
 */

#ifndef DCMJP2KCS_DJ2KENCODE_H
#define DCMJP2KCS_DJ2KENCODE_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/oftypes.h"      /* for OFBool */
#include "dcmtk/ofstd/ofstring.h"     /* for OFString */
#include "dcmtk/dcmjp2kcs/dj2kutil.h"   /* for enums */

class DJ2KCodecParameter;
class DJ2KLosslessEncoder;
class DJ2KEncoder;

/** singleton class that registers encoders for all supported JPEG 2000 processes.
 */
class DCMTK_DCMJP2KCS_EXPORT DJ2KEncoderRegistration
{
public:

  /** registers encoder for all supported JPEG 2000 processes.
   *  If already registered, call is ignored unless cleanup() has
   *  been performed before.
   *  @param createOffsetTable  flag indicating whether an offset table should be created
   *  @param fragmentSize       maximum fragment size in kbytes (0 = unlimited)
   *  @param uidcreation        mode for SOP Instance UID creation
   *  @param convertToSC        flag indicating whether the image should be converted to
   *                            Secondary Capture upon encoding
   */
  static void registerCodecs(
    OFBool createOffsetTable = OFTrue,
    Uint32 fragmentSize = 0,
    J2K_UIDCreation uidcreation = EJ2KUC_default,
    OFBool convertToSC = OFFalse);

  /** deregisters encoders.
   *  Attention: Must not be called while other threads might still use
   *  the registered codecs, e.g. because they are currently encoding
   *  DICOM data sets through dcmdata.
   */
  static void cleanup();

  /** get version information of the OpenJPEG library.
   *  @return name and version number of the OpenJPEG library
   */
  static OFString getLibraryVersionString();

private:

  /// flag indicating whether the encoders are already registered.
  static OFBool registered_;

  /// pointer to codec parameter shared by all encoders
  static DJ2KCodecParameter *cp_;

  /// pointer to lossless encoder
  static DJ2KLosslessEncoder *losslessEncoder_;

  /// pointer to lossy/lossless encoder
  static DJ2KEncoder *encoder_;

};

#endif
