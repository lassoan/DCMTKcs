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

#ifndef DCMJP2KCS_DJ2KPARAM_H
#define DCMJP2KCS_DJ2KPARAM_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dccodec.h" /* for DcmCodecParameter */
#include "dcmtk/dcmjp2kcs/dj2kutil.h" /* for enums */

/** codec parameter for JPEG 2000 codecs
 */
class DCMTK_DCMJP2KCS_EXPORT DJ2KCodecParameter: public DcmCodecParameter
{
public:

  /** constructor, for use with decoders.
   *  @param uidCreation                 mode for SOP Instance UID creation
   *  @param planarConfiguration         flag describing how planar configuration of decompressed color images should be handled
   *  @param ignoreOffsetTable           flag indicating whether to ignore the offset table when decompressing multiframe images
   *  @param forceSingleFragmentPerFrame while decompressing a multiframe image, assume one fragment per frame even if the JPEG
   *                                     data for some frame is incomplete
   */
  DJ2KCodecParameter(
    J2K_UIDCreation uidCreation = EJ2KUC_default,
    J2K_PlanarConfiguration planarConfiguration = EJ2KPC_restore,
    OFBool ignoreOffsetTable = OFFalse,
    OFBool forceSingleFragmentPerFrame = OFFalse);

  /** constructor, for use with encoders.
   *  @param createOffsetTable  flag indicating whether an offset table should be created
   *  @param fragmentSize       maximum fragment size in kbytes (0 = unlimited)
   *  @param uidCreation        mode for SOP Instance UID creation
   *  @param convertToSC        flag indicating whether the image should be converted to Secondary Capture
   */
  DJ2KCodecParameter(
    OFBool createOffsetTable,
    Uint32 fragmentSize,
    J2K_UIDCreation uidCreation,
    OFBool convertToSC);

  /// copy constructor
  DJ2KCodecParameter(const DJ2KCodecParameter& arg);

  /// destructor
  virtual ~DJ2KCodecParameter();

  /** this methods creates a copy of type DcmCodecParameter *
   *  it must be overwritten in every subclass.
   *  @return copy of this object
   */
  virtual DcmCodecParameter *clone() const;

  /** returns the class name as string.
   *  can be used as poor man's RTTI replacement.
   */
  virtual const char *className() const;

  /** returns mode for SOP Instance UID creation
   *  @return mode for SOP Instance UID creation
   */
  J2K_UIDCreation getUIDCreation() const
  {
    return uidCreation_;
  }

  /** returns mode for handling planar configuration
   *  @return mode for handling planar configuration
   */
  J2K_PlanarConfiguration getPlanarConfiguration() const
  {
    return planarConfiguration_;
  }

  /** returns true if the offset table should be ignored when decompressing multiframe images
   *  @return true if the offset table should be ignored when decompressing multiframe images
   */
  OFBool ignoreOffsetTable() const
  {
    return ignoreOffsetTable_;
  }

  /** returns flag indicating whether one fragment per frame should be enforced while decoding
   *  @return flag indicating whether one fragment per frame should be enforced while decoding
   */
  OFBool getForceSingleFragmentPerFrame() const
  {
    return forceSingleFragmentPerFrame_;
  }

  /** returns flag indicating whether an offset table should be created during encoding
   *  @return OFTrue if offset table should be created
   */
  OFBool getCreateOffsetTable() const
  {
    return createOffsetTable_;
  }

  /** returns maximum fragment size for encoding in kbytes (0 = unlimited)
   *  @return fragment size in kbytes
   */
  Uint32 getFragmentSize() const
  {
    return fragmentSize_;
  }

  /** returns flag indicating whether the image should be converted to Secondary Capture
   *  @return OFTrue if conversion to Secondary Capture is requested
   */
  OFBool getConvertToSC() const
  {
    return convertToSC_;
  }

private:

  /// private undefined copy assignment operator
  DJ2KCodecParameter& operator=(const DJ2KCodecParameter&);

  /// mode for SOP Instance UID creation (used both for encoding and decoding)
  J2K_UIDCreation uidCreation_;

  /// flag describing how planar configuration of decompressed color images should be handled
  J2K_PlanarConfiguration planarConfiguration_;

  /// flag indicating if the offset table should be ignored when decompressing multiframe images
  OFBool ignoreOffsetTable_;

  /** while decompressing a multiframe image,
   *  assume one fragment per frame even if the JPEG data for some frame is incomplete
   */
  OFBool forceSingleFragmentPerFrame_;

  /// flag indicating whether to create offset table during encoding
  OFBool createOffsetTable_;

  /// maximum fragment size in kbytes during encoding (0 = unlimited)
  Uint32 fragmentSize_;

  /// flag indicating whether to convert to Secondary Capture during encoding
  OFBool convertToSC_;

};


#endif
