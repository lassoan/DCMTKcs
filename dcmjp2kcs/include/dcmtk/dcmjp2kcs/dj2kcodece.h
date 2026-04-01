/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: codec classes for JPEG 2000 encoders.
 *
 */

#ifndef DCMJP2KCS_DJ2KCODECE_H
#define DCMJP2KCS_DJ2KCODECE_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dccodec.h"  /* for class DcmCodec */
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/dcmjp2kcs/dj2kdefine.h"

/* forward declarations */
class DJ2KCodecParameter;

/** abstract codec class for JPEG 2000 encoders.
 *  This abstract class contains most of the application logic
 *  needed for a dcmdata codec object that implements a JPEG 2000 encoder.
 *  This class only supports compression from uncompressed pixel data; it
 *  does not support transcoding from other compressed formats.
 */
class DCMTK_DCMJP2KCS_EXPORT DJ2KEncoderBase : public DcmCodec
{
public:

  /// default constructor
  DJ2KEncoderBase();

  /// destructor
  virtual ~DJ2KEncoderBase();

  /** decompresses the given pixel sequence and
   *  stores the result in the given uncompressedPixelData element.
   *  This codec only supports encoding, so this method always returns EC_IllegalCall.
   */
  virtual OFCondition decode(
    const DcmRepresentationParameter * fromRepParam,
    DcmPixelSequence * pixSeq,
    DcmPolymorphOBOW& uncompressedPixelData,
    const DcmCodecParameter * cp,
    const DcmStack & objStack,
    OFBool& removeOldRep) const;

  /** decompresses a single frame from the given pixel sequence.
   *  This codec only supports encoding, so this method always returns EC_IllegalCall.
   */
  virtual OFCondition decodeFrame(
    const DcmRepresentationParameter * fromParam,
    DcmPixelSequence * fromPixSeq,
    const DcmCodecParameter * cp,
    DcmItem *dataset,
    Uint32 frameNo,
    Uint32& startFragment,
    void *buffer,
    Uint32 bufSize,
    OFString& decompressedColorModel) const;

  /** compresses the given uncompressed DICOM image and stores
   *  the result in the given pixSeq element.
   *  @param pixelData pointer to the uncompressed image data in OW format
   *    and local byte order
   *  @param length length of the pixel data field in bytes
   *  @param toRepParam representation parameter describing the desired
   *    compressed representation
   *  @param pixSeq compressed pixel sequence (pointer to new DcmPixelSequence
   *    object allocated on heap) returned in this parameter upon success
   *  @param cp codec parameters for this codec
   *  @param objStack stack pointing to the location of the pixel data
   *    element in the current dataset
   *  @param removeOldRep flag set to OFTrue upon successful encoding
   *  @return EC_Normal if successful, an error code otherwise
   */
  virtual OFCondition encode(
    const Uint16 * pixelData,
    const Uint32 length,
    const DcmRepresentationParameter * toRepParam,
    DcmPixelSequence * & pixSeq,
    const DcmCodecParameter *cp,
    DcmStack & objStack,
    OFBool& removeOldRep) const;

  /** transcodes (re-compresses) the given compressed DICOM image.
   *  This codec does not support transcoding, so this method always returns EC_IllegalCall.
   */
  virtual OFCondition encode(
    const E_TransferSyntax fromRepType,
    const DcmRepresentationParameter * fromRepParam,
    DcmPixelSequence * fromPixSeq,
    const DcmRepresentationParameter * toRepParam,
    DcmPixelSequence * & toPixSeq,
    const DcmCodecParameter * cp,
    DcmStack & objStack,
    OFBool& removeOldRep) const;

  /** checks if this codec is able to convert from the
   *  given current transfer syntax to the given new transfer syntax.
   *  Returns true when oldRepType is a native (uncompressed) format
   *  and newRepType equals supportedTransferSyntax().
   *  @param oldRepType current transfer syntax
   *  @param newRepType desired new transfer syntax
   *  @return true if transformation is supported by this codec, false otherwise
   */
  virtual OFBool canChangeCoding(
    const E_TransferSyntax oldRepType,
    const E_TransferSyntax newRepType) const;

  /** determine color model of the decompressed image.
   *  This codec only supports encoding, so this method always returns EC_IllegalCall.
   */
  virtual OFCondition determineDecompressedColorModel(
    const DcmRepresentationParameter *fromParam,
    DcmPixelSequence *fromPixSeq,
    const DcmCodecParameter *cp,
    DcmItem *dataset,
    OFString &decompressedColorModel) const;

  /** returns the number of bits allocated for each sample of the decompressed image.
   *  This codec only supports encoding, so this method always returns 0.
   *  @param bitsAllocated current value of Bits Allocated (ignored)
   *  @param bitsStored current value of Bits Stored (ignored)
   *  @return 0 (encoding-only codec, decompression not supported)
   */
  virtual Uint16 decodedBitsAllocated(
    Uint16 bitsAllocated,
    Uint16 bitsStored) const;

private:

  /** returns the transfer syntax that this particular codec
   *  is able to encode to.
   *  @return supported transfer syntax
   */
  virtual E_TransferSyntax supportedTransferSyntax() const = 0;

  /** encodes a single image frame as JPEG 2000 using OpenJPEG.
   *  @param frameData     pointer to uncompressed frame data (byte array)
   *  @param width         image width in pixels
   *  @param height        image height in pixels
   *  @param samplesPerPixel number of samples (color components) per pixel
   *  @param bitsAllocated bits allocated per sample
   *  @param bitsStored    bits stored per sample
   *  @param isSigned      true if pixel data is signed
   *  @param planarByPlane true if input data is color-by-plane (planar configuration 1)
   *  @param lossless      true for lossless compression
   *  @param compressionRatio target compression ratio for lossy mode (ignored if lossless)
   *  @param compressedData on success, newly allocated buffer containing the J2K codestream
   *  @param compressedSize on success, size of compressedData in bytes
   *  @return EC_Normal if successful, an error code otherwise
   */
  static OFCondition encodeFrame(
    const Uint8 *frameData,
    Uint32 width,
    Uint32 height,
    Uint32 samplesPerPixel,
    Uint32 bitsAllocated,
    Uint32 bitsStored,
    OFBool isSigned,
    OFBool planarByPlane,
    OFBool lossless,
    double compressionRatio,
    Uint8 *& compressedData,
    size_t & compressedSize);

};

/** codec class for JPEG 2000 lossless-only encoding
 */
class DCMTK_DCMJP2KCS_EXPORT DJ2KLosslessEncoder : public DJ2KEncoderBase
{
  /** returns the transfer syntax that this particular codec
   *  is able to encode to (EXS_JPEG2000LosslessOnly).
   *  @return supported transfer syntax
   */
  virtual E_TransferSyntax supportedTransferSyntax() const;
};

/** codec class for JPEG 2000 (lossy or lossless) encoding
 */
class DCMTK_DCMJP2KCS_EXPORT DJ2KEncoder : public DJ2KEncoderBase
{
  /** returns the transfer syntax that this particular codec
   *  is able to encode to (EXS_JPEG2000).
   *  @return supported transfer syntax
   */
  virtual E_TransferSyntax supportedTransferSyntax() const;
};

#endif
