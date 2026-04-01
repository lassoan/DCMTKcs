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

#ifndef DCMJP2KCS_DJ2KRPARAM_H
#define DCMJP2KCS_DJ2KRPARAM_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dcpixel.h"  /* for DcmRepresentationParameter */
#include "dcmtk/dcmjp2kcs/dj2kdefine.h"

/** representation parameter for JPEG 2000 encoding
 */
class DCMTK_DCMJP2KCS_EXPORT DJ2KRepresentationParameter : public DcmRepresentationParameter
{
public:

  /** constructor
   *  @param losslessProcess    true if lossless JPEG 2000 encoding should be used
   *  @param compressionRatio   compression ratio for lossy encoding (e.g. 10.0 means 10:1).
   *                            Only used when losslessProcess is OFFalse.
   *                            A value of 0.0 means "use default".
   */
  DJ2KRepresentationParameter(
    OFBool losslessProcess = OFTrue,
    double compressionRatio = 0.0);

  /// copy constructor
  DJ2KRepresentationParameter(const DJ2KRepresentationParameter& arg);

  /// destructor
  virtual ~DJ2KRepresentationParameter();

  /** this methods creates a copy of type DcmRepresentationParameter *
   *  it must be overwritten in every subclass.
   *  @return copy of this object
   */
  virtual DcmRepresentationParameter *clone() const;

  /** returns the class name as string.
   *  can be used as poor man's RTTI replacement.
   */
  virtual const char *className() const;

  /** returns an integer indicating equality of this
   *  representation parameter and the one given.
   *  @param arg representation parameter to compare with
   *  @return true if equal, false otherwise
   */
  virtual OFBool operator==(const DcmRepresentationParameter &arg) const;

  /** returns whether lossless JPEG 2000 encoding should be used
   *  @return OFTrue if lossless encoding, OFFalse if lossy
   */
  OFBool useLosslessProcess() const
  {
    return losslessProcess_;
  }

  /** returns the compression ratio for lossy encoding
   *  @return compression ratio (only meaningful when useLosslessProcess() == OFFalse)
   */
  double getCompressionRatio() const
  {
    return compressionRatio_;
  }

private:

  /// flag indicating whether lossless JPEG 2000 encoding is used
  OFBool losslessProcess_;

  /// compression ratio for lossy encoding (e.g. 10.0 for 10:1), 0.0 = default
  double compressionRatio_;

};

#endif
