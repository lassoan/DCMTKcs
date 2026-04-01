/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: codec classes for JPEG 2000 decoders.
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2kcs/dj2kcodecd.h"

#include "dcmtk/ofstd/ofstream.h"    /* for ofstream */
#include "dcmtk/ofstd/ofcast.h"      /* for casts */
#include "dcmtk/ofstd/offile.h"      /* for class OFFile */
#include "dcmtk/ofstd/ofstd.h"       /* for class OFStandard */
#include "dcmtk/dcmdata/dcdatset.h"  /* for class DcmDataset */
#include "dcmtk/dcmdata/dcdeftag.h"  /* for tag constants */
#include "dcmtk/dcmdata/dcpixseq.h"  /* for class DcmPixelSequence */
#include "dcmtk/dcmdata/dcpxitem.h"  /* for class DcmPixelItem */
#include "dcmtk/dcmdata/dcvrpobw.h"  /* for class DcmPolymorphOBOW */
#include "dcmtk/dcmdata/dcswap.h"    /* for swapIfNecessary() */
#include "dcmtk/dcmdata/dcuid.h"     /* for dcmGenerateUniqueIdentifer()*/
#include "dcmtk/dcmdata/dcerror.h"   /* for error constants */
#include "dcmtk/dcmjp2kcs/dj2kparam.h" /* for class DJ2KCodecParameter */

#ifdef WITH_OPENJPEG
#include <openjpeg.h>
#endif

// JPEG 2000 magic bytes: Start of Codestream (SOC) marker 0xFF4F
// or JP2 file signature: 0x0000000C 6A502020 0D0A870A (for JP2 file format)
static const Uint8 J2K_SOC_MARKER[] = { 0xFF, 0x4F };
static const Uint8 JP2_SIGNATURE[] = { 0x00, 0x00, 0x00, 0x0C, 0x6A, 0x50, 0x20, 0x20, 0x0D, 0x0A, 0x87, 0x0A };

E_TransferSyntax DJ2KLosslessDecoder::supportedTransferSyntax() const
{
  return EXS_JPEG2000LosslessOnly;
}

OFBool DJ2KLosslessDecoder::isLosslessProcess() const
{
  return OFTrue;
}

E_TransferSyntax DJ2KDecoder::supportedTransferSyntax() const
{
  return EXS_JPEG2000;
}

OFBool DJ2KDecoder::isLosslessProcess() const
{
  return OFFalse;
}

// --------------------------------------------------------------------------

DJ2KDecoderBase::DJ2KDecoderBase()
: DcmCodec()
{
}


DJ2KDecoderBase::~DJ2KDecoderBase()
{
}


OFBool DJ2KDecoderBase::canChangeCoding(
    const E_TransferSyntax oldRepType,
    const E_TransferSyntax newRepType) const
{
  // this codec only handles conversion from JPEG 2000 to uncompressed.
  DcmXfer newRep(newRepType);
  if (newRep.usesNativeFormat() && (oldRepType == supportedTransferSyntax()))
     return OFTrue;

  return OFFalse;
}

#ifdef WITH_OPENJPEG

// OpenJPEG error callback
static void opj_error_callback(const char *msg, void * /* client_data */)
{
  DCMJP2KCS_ERROR("OpenJPEG error: " << msg);
}

// OpenJPEG warning callback
static void opj_warning_callback(const char *msg, void * /* client_data */)
{
  DCMJP2KCS_WARN("OpenJPEG warning: " << msg);
}

// OpenJPEG info callback
static void opj_info_callback(const char *msg, void * /* client_data */)
{
  DCMJP2KCS_DEBUG("OpenJPEG info: " << msg);
}

/** Helper structure for memory-based stream operations in OpenJPEG */
struct opj_memory_stream
{
  Uint8 *data;        /**< pointer to the data */
  size_t size;        /**< size of the data */
  size_t offset;      /**< current read offset */
};

/** Read function for OpenJPEG memory stream */
static OPJ_SIZE_T opj_memory_stream_read(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data)
{
  opj_memory_stream *stream = OFreinterpret_cast(opj_memory_stream*, p_user_data);
  OPJ_SIZE_T bytesToRead = p_nb_bytes;

  if (stream->offset >= stream->size)
    return (OPJ_SIZE_T)-1;

  if (bytesToRead > (stream->size - stream->offset))
    bytesToRead = stream->size - stream->offset;

  if (bytesToRead > 0)
  {
    memcpy(p_buffer, stream->data + stream->offset, bytesToRead);
    stream->offset += bytesToRead;
  }

  return bytesToRead;
}

/** Seek function for OpenJPEG memory stream */
static OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void *p_user_data)
{
  opj_memory_stream *stream = OFreinterpret_cast(opj_memory_stream*, p_user_data);

  if (p_nb_bytes < 0 || OFstatic_cast(size_t, p_nb_bytes) > stream->size)
    return OPJ_FALSE;

  stream->offset = OFstatic_cast(size_t, p_nb_bytes);
  return OPJ_TRUE;
}

/** Skip function for OpenJPEG memory stream */
static OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void *p_user_data)
{
  opj_memory_stream *stream = OFreinterpret_cast(opj_memory_stream*, p_user_data);

  if (p_nb_bytes < 0)
    return -1;

  size_t newOffset = stream->offset + OFstatic_cast(size_t, p_nb_bytes);
  if (newOffset > stream->size)
    newOffset = stream->size;

  OPJ_OFF_T skipped = OFstatic_cast(OPJ_OFF_T, newOffset - stream->offset);
  stream->offset = newOffset;

  return skipped;
}

/** Free function for OpenJPEG memory stream (no-op since we manage memory ourselves) */
static void opj_memory_stream_free(void * /* p_user_data */)
{
  // We don't free here; the caller manages the memory
}

/** Decode a JPEG 2000 codestream using OpenJPEG
 *  @param j2kData pointer to the JPEG 2000 codestream
 *  @param j2kSize size of the JPEG 2000 codestream in bytes
 *  @param buffer output buffer for decompressed pixel data
 *  @param bufSize size of the output buffer
 *  @param expectedWidth expected image width
 *  @param expectedHeight expected image height
 *  @param expectedComponents expected number of components
 *  @param bytesPerSample expected bytes per sample (1 or 2)
 *  @param pixelRepresentation 0 for unsigned, 1 for signed (from DICOM PixelRepresentation)
 *  @return EC_Normal on success, error code otherwise
 */
static OFCondition decodeJPEG2000(
    Uint8 *j2kData,
    size_t j2kSize,
    void *buffer,
    Uint32 bufSize,
    Uint16 expectedWidth,
    Uint16 expectedHeight,
    Uint16 expectedComponents,
    Uint16 bytesPerSample,
    Uint16 pixelRepresentation)
{
  OFCondition result = EC_Normal;

  DCMJP2KCS_DEBUG("decodeJPEG2000: j2kSize=" << j2kSize << ", bufSize=" << bufSize 
                << ", expected " << expectedWidth << "x" << expectedHeight << "x" << expectedComponents
                << ", bytesPerSample=" << bytesPerSample << ", pixelRep=" << pixelRepresentation);

  // Log first bytes for debugging
  if (j2kSize >= 4)
  {
    DCMJP2KCS_DEBUG("First 4 bytes of J2K data: 0x" 
                  << std::hex << (int)j2kData[0] << " 0x" << (int)j2kData[1] << " 0x" 
                  << (int)j2kData[2] << " 0x" << (int)j2kData[3] << std::dec);
  }

  // Determine codec type (J2K raw codestream or JP2 file format)
  OPJ_CODEC_FORMAT codecFormat = OPJ_CODEC_J2K;
  if (j2kSize >= 12 && memcmp(j2kData, JP2_SIGNATURE, 12) == 0)
  {
    codecFormat = OPJ_CODEC_JP2;
    DCMJP2KCS_DEBUG("Detected JP2 file format");
  }
  else if (j2kSize >= 2 && memcmp(j2kData, J2K_SOC_MARKER, 2) == 0)
  {
    codecFormat = OPJ_CODEC_J2K;
    DCMJP2KCS_DEBUG("Detected J2K codestream format");
  }
  else
  {
    DCMJP2KCS_ERROR("Unable to determine JPEG 2000 codec format - first bytes don't match known signatures");
    return EC_J2KInvalidCompressedData;
  }

  // Create decompressor
  opj_codec_t *codec = opj_create_decompress(codecFormat);
  if (!codec)
  {
    DCMJP2KCS_ERROR("Failed to create OpenJPEG decompressor");
    return EC_J2KInvalidCompressedData;
  }

  // Set up callbacks
  opj_set_error_handler(codec, opj_error_callback, NULL);
  opj_set_warning_handler(codec, opj_warning_callback, NULL);
  opj_set_info_handler(codec, opj_info_callback, NULL);

  // Set default decoding parameters
  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);

  if (!opj_setup_decoder(codec, &parameters))
  {
    DCMJP2KCS_ERROR("Failed to setup OpenJPEG decoder");
    opj_destroy_codec(codec);
    return EC_J2KInvalidCompressedData;
  }

  // Create memory stream
  opj_memory_stream memStream;
  memStream.data = j2kData;
  memStream.size = j2kSize;
  memStream.offset = 0;

  opj_stream_t *stream = opj_stream_create(j2kSize, OPJ_TRUE);
  if (!stream)
  {
    DCMJP2KCS_ERROR("Failed to create OpenJPEG stream");
    opj_destroy_codec(codec);
    return EC_J2KInvalidCompressedData;
  }

  opj_stream_set_user_data(stream, &memStream, opj_memory_stream_free);
  opj_stream_set_user_data_length(stream, j2kSize);
  opj_stream_set_read_function(stream, opj_memory_stream_read);
  opj_stream_set_seek_function(stream, opj_memory_stream_seek);
  opj_stream_set_skip_function(stream, opj_memory_stream_skip);

  // Read header
  opj_image_t *image = NULL;
  if (!opj_read_header(stream, codec, &image))
  {
    DCMJP2KCS_ERROR("Failed to read JPEG 2000 header");
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    return EC_J2KInvalidCompressedData;
  }

  // Get actual image dimensions
  OPJ_UINT32 width = image->x1 - image->x0;
  OPJ_UINT32 height = image->y1 - image->y0;

  DCMJP2KCS_DEBUG("JPEG 2000 image: " << width << "x" << height << ", " << image->numcomps << " components");
  for (OPJ_UINT32 c = 0; c < image->numcomps; c++)
  {
    DCMJP2KCS_DEBUG("  Component " << c << ": " << image->comps[c].w << "x" << image->comps[c].h
                  << ", prec=" << image->comps[c].prec << ", sgnd=" << image->comps[c].sgnd
                  << ", dx=" << image->comps[c].dx << ", dy=" << image->comps[c].dy);
  }

  // Validate image parameters
  if (width != expectedWidth ||
      height != expectedHeight ||
      image->numcomps != expectedComponents)
  {
    DCMJP2KCS_ERROR("JPEG 2000 image dimensions mismatch: expected "
                  << expectedWidth << "x" << expectedHeight << "x" << expectedComponents
                  << ", got " << width << "x" << height << "x" << image->numcomps);
    opj_image_destroy(image);
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    return EC_J2KImageDataMismatch;
  }

  // Decode the image
  if (!opj_decode(codec, stream, image))
  {
    DCMJP2KCS_ERROR("Failed to decode JPEG 2000 image");
    opj_image_destroy(image);
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    return EC_J2KInvalidCompressedData;
  }

  if (!opj_end_decompress(codec, stream))
  {
    DCMJP2KCS_WARN("opj_end_decompress failed, but image may still be valid");
  }

  // Get dimensions from first component
  OPJ_UINT32 compWidth = image->comps[0].w;
  OPJ_UINT32 compHeight = image->comps[0].h;
  OPJ_UINT32 numComponents = image->numcomps;
  OPJ_UINT32 pixelCount = compWidth * compHeight;

  DCMJP2KCS_DEBUG("Component dimensions: " << compWidth << "x" << compHeight << ", pixelCount=" << pixelCount);

  // Check if component data is valid
  if (image->comps[0].data == NULL)
  {
    DCMJP2KCS_ERROR("Component data is NULL after decoding");
    opj_image_destroy(image);
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    return EC_J2KInvalidCompressedData;
  }

  // Log some sample values for debugging
  if (pixelCount > 0)
  {
    OPJ_INT32 minVal = image->comps[0].data[0];
    OPJ_INT32 maxVal = image->comps[0].data[0];
    for (OPJ_UINT32 i = 1; i < pixelCount && i < 100000; i++)
    {
      if (image->comps[0].data[i] < minVal) minVal = image->comps[0].data[i];
      if (image->comps[0].data[i] > maxVal) maxVal = image->comps[0].data[i];
    }
    DCMJP2KCS_DEBUG("Component 0 value range (sampled): min=" << minVal << ", max=" << maxVal);
  }

  // Calculate required buffer size
  Uint32 requiredSize = pixelCount * numComponents * bytesPerSample;
  if (requiredSize > bufSize)
  {
    DCMJP2KCS_ERROR("Output buffer too small: need " << requiredSize << " bytes, have " << bufSize);
    opj_image_destroy(image);
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    return EC_J2KUncompressedBufferTooSmall;
  }

  // Copy pixel data
  // OpenJPEG stores components separately, DICOM expects interleaved for color images
  // OpenJPEG returns OPJ_INT32 values - we convert to the appropriate output size
  
  // The key insight: OpenJPEG's sgnd flag tells us how data is stored in J2K
  // DICOM's PixelRepresentation tells us how the output should be interpreted
  // For lossless round-trip, we should just copy the values directly
  
  if (bytesPerSample == 1)
  {
    Uint8 *outPtr = OFreinterpret_cast(Uint8*, buffer);
    
    for (OPJ_UINT32 i = 0; i < pixelCount; i++)
    {
      for (OPJ_UINT32 c = 0; c < numComponents; c++)
      {
        OPJ_INT32 val = image->comps[c].data[i];
        
        // For signed J2K data going to unsigned DICOM output, add offset
        // For unsigned J2K data going to signed DICOM output, subtract offset
        if (image->comps[c].sgnd && pixelRepresentation == 0)
        {
          // J2K is signed, DICOM wants unsigned - add offset
          val += (1 << (image->comps[c].prec - 1));
        }
        else if (!image->comps[c].sgnd && pixelRepresentation == 1)
        {
          // J2K is unsigned, DICOM wants signed - subtract offset
          val -= (1 << (image->comps[c].prec - 1));
        }
        // else: both match, no conversion needed
        
        // Clamp to 8-bit range
        if (pixelRepresentation == 0)
        {
          // Unsigned output
          if (val < 0) val = 0;
          if (val > 255) val = 255;
        }
        else
        {
          // Signed output (stored as unsigned byte, interpreted as signed)
          if (val < -128) val = -128;
          if (val > 127) val = 127;
          val = val & 0xFF;  // Store as unsigned byte
        }
        
        *outPtr++ = OFstatic_cast(Uint8, val);
      }
    }
  }
  else // bytesPerSample == 2
  {
    Uint16 *outPtr = OFreinterpret_cast(Uint16*, buffer);
    
    for (OPJ_UINT32 i = 0; i < pixelCount; i++)
    {
      for (OPJ_UINT32 c = 0; c < numComponents; c++)
      {
        OPJ_INT32 val = image->comps[c].data[i];
        
        // For signed J2K data going to unsigned DICOM output, add offset
        // For unsigned J2K data going to signed DICOM output, subtract offset
        if (image->comps[c].sgnd && pixelRepresentation == 0)
        {
          // J2K is signed, DICOM wants unsigned - add offset
          val += (1 << (image->comps[c].prec - 1));
        }
        else if (!image->comps[c].sgnd && pixelRepresentation == 1)
        {
          // J2K is unsigned, DICOM wants signed - subtract offset
          val -= (1 << (image->comps[c].prec - 1));
        }
        // else: both match, no conversion needed
        
        // Clamp to 16-bit range
        if (pixelRepresentation == 0)
        {
          // Unsigned output
          if (val < 0) val = 0;
          if (val > 65535) val = 65535;
        }
        else
        {
          // Signed output
          if (val < -32768) val = -32768;
          if (val > 32767) val = 32767;
          val = val & 0xFFFF;  // Store as unsigned word
        }
        
        *outPtr++ = OFstatic_cast(Uint16, val);
      }
    }
  }

  DCMJP2KCS_DEBUG("Successfully decoded " << pixelCount << " pixels to buffer");

  // Clean up
  opj_image_destroy(image);
  opj_stream_destroy(stream);
  opj_destroy_codec(codec);

  return result;
}

#endif // WITH_OPENJPEG


OFCondition DJ2KDecoderBase::decode(
    const DcmRepresentationParameter * /* fromRepParam */,
    DcmPixelSequence * pixSeq,
    DcmPolymorphOBOW& uncompressedPixelData,
    const DcmCodecParameter * cp,
    const DcmStack& objStack,
    OFBool& removeOldRep) const
{
#ifndef WITH_OPENJPEG
  return EC_J2KOpenJPEGNotAvailable;
#else

  // this codec may modify the DICOM header such that the previous pixel
  // representation is not valid anymore. Indicate this to the caller
  // to trigger removal.
  removeOldRep = OFTrue;

  // retrieve pointer to dataset from parameter stack
  DcmStack localStack(objStack);
  (void)localStack.pop();  // pop pixel data element from stack
  DcmObject *dobject = localStack.pop(); // this is the item in which the pixel data is located
  if ((!dobject)||((dobject->ident()!= EVR_dataset) && (dobject->ident()!= EVR_item))) return EC_InvalidTag;
  DcmItem *dataset = OFstatic_cast(DcmItem *, dobject);
  OFBool numberOfFramesPresent = OFFalse;

  // determine properties of uncompressed dataset
  Uint16 imageSamplesPerPixel = 0;
  if (dataset->findAndGetUint16(DCM_SamplesPerPixel, imageSamplesPerPixel).bad()) return EC_TagNotFound;

  // we only handle one or three samples per pixel
  if ((imageSamplesPerPixel != 3) && (imageSamplesPerPixel != 1)) return EC_InvalidTag;

  Uint16 imageRows = 0;
  if (dataset->findAndGetUint16(DCM_Rows, imageRows).bad()) return EC_TagNotFound;
  if (imageRows < 1) return EC_InvalidTag;

  Uint16 imageColumns = 0;
  if (dataset->findAndGetUint16(DCM_Columns, imageColumns).bad()) return EC_TagNotFound;
  if (imageColumns < 1) return EC_InvalidTag;

  // number of frames is an optional attribute - we don't mind if it isn't present.
  Sint32 imageFrames = 0;
  if (dataset->findAndGetSint32(DCM_NumberOfFrames, imageFrames).good()) numberOfFramesPresent = OFTrue;

  if (imageFrames >= OFstatic_cast(Sint32, pixSeq->card()))
    imageFrames = pixSeq->card() - 1; // limit number of frames to number of pixel items - 1
  if (imageFrames < 1)
    imageFrames = 1; // default in case the number of frames attribute is absent or contains garbage

  Uint16 imageBitsStored = 0;
  if (dataset->findAndGetUint16(DCM_BitsStored, imageBitsStored).bad()) return EC_TagNotFound;

  Uint16 imageBitsAllocated = 0;
  if (dataset->findAndGetUint16(DCM_BitsAllocated, imageBitsAllocated).bad()) return EC_TagNotFound;

  Uint16 imageHighBit = 0;
  if (dataset->findAndGetUint16(DCM_HighBit, imageHighBit).bad()) return EC_TagNotFound;

  // we only support up to 16 bits per sample
  if ((imageBitsStored < 1) || (imageBitsStored > 16)) return EC_J2KUnsupportedBitDepth;

  // determine the number of bytes per sample (bits allocated) for the de-compressed object.
  Uint16 bytesPerSample = 1;
  if (imageBitsStored > 8) bytesPerSample = 2;
  else if (imageBitsAllocated > 8) bytesPerSample = 2;

  // compute size of uncompressed frame, in bytes
  Uint32 frameSize = bytesPerSample * imageRows * imageColumns * imageSamplesPerPixel;

  // check for overflow
  if (imageRows != 0 && frameSize / imageRows != (OFstatic_cast(Uint32, bytesPerSample) * imageColumns * imageSamplesPerPixel))
  {
    DCMJP2KCS_WARN("cannot decompress image because uncompressed representation would exceed maximum possible size of PixelData attribute");
    return EC_ElemLengthExceeds32BitField;
  }

  // compute size of pixel data attribute, in bytes
  Uint32 totalSize = frameSize * imageFrames;

  // check for overflow
  if (totalSize == 0xFFFFFFFF || (frameSize != 0 && totalSize / frameSize != OFstatic_cast(Uint32, imageFrames)))
  {
    DCMJP2KCS_WARN("cannot decompress image because uncompressed representation would exceed maximum possible size of PixelData attribute");
    return EC_ElemLengthExceeds32BitField;
  }

  if (totalSize & 1) totalSize++; // align on 16-bit word boundary

  // assume we can cast the codec parameter to what we need
  const DJ2KCodecParameter *djcp = OFreinterpret_cast(const DJ2KCodecParameter *, cp);

  // allocate space for uncompressed pixel data element
  Uint16 *pixeldata16 = NULL;
  OFCondition result = uncompressedPixelData.createUint16Array(totalSize/sizeof(Uint16), pixeldata16);
  if (result.bad()) return result;

  Uint8 *pixeldata8 = OFreinterpret_cast(Uint8 *, pixeldata16);
  Sint32 currentFrame = 0;
  Uint32 currentItem = 1; // item 0 contains the offset table
  OFBool done = OFFalse;
  OFBool forceSingleFragmentPerFrame = djcp->getForceSingleFragmentPerFrame();

  while (result.good() && !done)
  {
      DCMJP2KCS_DEBUG("JPEG 2000 decoder processes frame " << (currentFrame+1));

      result = decodeFrame(pixSeq, djcp, dataset, currentFrame, currentItem, pixeldata8, frameSize,
          imageFrames, imageColumns, imageRows, imageSamplesPerPixel, bytesPerSample);

      // check if we should enforce "one fragment per frame" while
      // decompressing a multi-frame image even if stream suspension occurs
      if ((result == EC_J2KInvalidCompressedData) && forceSingleFragmentPerFrame)
      {
        // frame is incomplete. Nevertheless skip to next frame.
        // This permits decompression of faulty multi-frame images.
        DCMJP2KCS_WARN("JPEG 2000 bitstream invalid or incomplete, ignoring (but image is likely to be incomplete)");
        result = EC_Normal;
      }

      if (result.good())
      {
        // increment frame number, check if we're finished
        if (++currentFrame == imageFrames) done = OFTrue;
        pixeldata8 += frameSize;
      }
  }

  // Number of Frames might have changed in case the previous value was wrong
  if (result.good() && (numberOfFramesPresent || (imageFrames > 1)))
  {
    char numBuf[20];
    OFStandard::snprintf(numBuf, sizeof(numBuf), "%ld", OFstatic_cast(long, imageFrames));
    result = ((DcmItem *)dataset)->putAndInsertString(DCM_NumberOfFrames, numBuf);
  }

  if (result.good() && (dataset->ident() == EVR_dataset))
  {
    DcmItem *ditem = OFreinterpret_cast(DcmItem*, dataset);

    if (djcp->getUIDCreation() == EJ2KUC_always)
    {
        // create new SOP instance UID
        result = DcmCodec::newInstance(ditem, NULL, NULL, NULL);
    }

    // set Lossy Image Compression to "01" if this was lossy
    if (result.good() && !isLosslessProcess())
    {
      result = ditem->putAndInsertString(DCM_LossyImageCompression, "01");
    }
  }

  return result;
#endif // WITH_OPENJPEG
}


OFCondition DJ2KDecoderBase::decodeFrame(
    const DcmRepresentationParameter * /* fromParam */,
    DcmPixelSequence *fromPixSeq,
    const DcmCodecParameter *cp,
    DcmItem *dataset,
    Uint32 frameNo,
    Uint32& currentItem,
    void * buffer,
    Uint32 bufSize,
    OFString& decompressedColorModel) const
{
#ifndef WITH_OPENJPEG
  return EC_J2KOpenJPEGNotAvailable;
#else
  OFCondition result = EC_Normal;

  // assume we can cast the codec parameter to what we need
  const DJ2KCodecParameter *djcp = OFreinterpret_cast(const DJ2KCodecParameter *, cp);

  // determine properties of uncompressed dataset
  Uint16 imageSamplesPerPixel = 0;
  if (dataset->findAndGetUint16(DCM_SamplesPerPixel, imageSamplesPerPixel).bad()) return EC_TagNotFound;
  // we only handle one or three samples per pixel
  if ((imageSamplesPerPixel != 3) && (imageSamplesPerPixel != 1)) return EC_InvalidTag;

  Uint16 imageRows = 0;
  if (dataset->findAndGetUint16(DCM_Rows, imageRows).bad()) return EC_TagNotFound;
  if (imageRows < 1) return EC_InvalidTag;

  Uint16 imageColumns = 0;
  if (dataset->findAndGetUint16(DCM_Columns, imageColumns).bad()) return EC_TagNotFound;
  if (imageColumns < 1) return EC_InvalidTag;

  Uint16 imageBitsStored = 0;
  if (dataset->findAndGetUint16(DCM_BitsStored, imageBitsStored).bad()) return EC_TagNotFound;

  Uint16 imageBitsAllocated = 0;
  if (dataset->findAndGetUint16(DCM_BitsAllocated, imageBitsAllocated).bad()) return EC_TagNotFound;

  // we only support up to 16 bits per sample
  if ((imageBitsStored < 1) || (imageBitsStored > 16)) return EC_J2KUnsupportedBitDepth;

  // determine the number of bytes per sample (bits allocated) for the de-compressed object.
  Uint16 bytesPerSample = 1;
  if (imageBitsStored > 8) bytesPerSample = 2;
  else if (imageBitsAllocated > 8) bytesPerSample = 2;

  // number of frames is an optional attribute - we don't mind if it isn't present.
  Sint32 imageFrames = 0;
  dataset->findAndGetSint32(DCM_NumberOfFrames, imageFrames).good();

  if (imageFrames >= OFstatic_cast(Sint32, fromPixSeq->card()))
    imageFrames = fromPixSeq->card() - 1;
  if (imageFrames < 1)
    imageFrames = 1;

  // if the user has provided this information, we trust him.
  // If the user has passed a zero, try to find out ourselves.
  if (currentItem == 0)
  {
    result = determineStartFragment(frameNo, imageFrames, fromPixSeq, currentItem);
  }

  if (result.good())
  {
    DCMJP2KCS_DEBUG("starting to decode frame " << frameNo << " with fragment " << currentItem);
    result = decodeFrame(fromPixSeq, djcp, dataset, frameNo, currentItem, buffer, bufSize,
        imageFrames, imageColumns, imageRows, imageSamplesPerPixel, bytesPerSample);
  }

  if (result.good())
  {
    // retrieve color model from given dataset
    result = dataset->findAndGetOFString(DCM_PhotometricInterpretation, decompressedColorModel);
  }

  return result;
#endif
}

OFCondition DJ2KDecoderBase::decodeFrame(
    DcmPixelSequence * fromPixSeq,
    const DJ2KCodecParameter *cp,
    DcmItem *dataset,
    Uint32 frameNo,
    Uint32& currentItem,
    void * buffer,
    Uint32 bufSize,
    Sint32 imageFrames,
    Uint16 imageColumns,
    Uint16 imageRows,
    Uint16 imageSamplesPerPixel,
    Uint16 bytesPerSample)
{
#ifndef WITH_OPENJPEG
  return EC_J2KOpenJPEGNotAvailable;
#else
  DcmPixelItem *pixItem = NULL;
  Uint8 * j2kData = NULL;
  Uint8 * j2kFragmentData = NULL;
  Uint32 fragmentLength = 0;
  size_t compressedSize = 0;
  Uint32 fragmentsForThisFrame = 0;
  OFCondition result = EC_Normal;
  OFBool ignoreOffsetTable = cp->ignoreOffsetTable();

  // Get PixelRepresentation (0 = unsigned, 1 = signed)
  Uint16 pixelRepresentation = 0;
  dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation);

  // compute the number of JPEG 2000 fragments we need in order to decode the next frame
  fragmentsForThisFrame = computeNumberOfFragments(imageFrames, frameNo, currentItem, ignoreOffsetTable, fromPixSeq);
  if (fragmentsForThisFrame == 0) result = EC_J2KCannotComputeNumberOfFragments;

  // determine planar configuration for uncompressed data
  OFString imageSopClass;
  OFString imagePhotometricInterpretation;
  dataset->findAndGetOFString(DCM_SOPClassUID, imageSopClass);
  dataset->findAndGetOFString(DCM_PhotometricInterpretation, imagePhotometricInterpretation);
  Uint16 imagePlanarConfiguration = 0; // 0 is color-by-pixel, 1 is color-by-plane

  if (imageSamplesPerPixel > 1)
  {
    // get planar configuration from dataset
    imagePlanarConfiguration = 2; // invalid value
    if (dataset->findAndGetUint16(DCM_PlanarConfiguration, imagePlanarConfiguration).good() && (imagePlanarConfiguration != 0))
      DCMJP2KCS_WARN("invalid value for PlanarConfiguration " << DCM_PlanarConfiguration << ", should be \"0\"");

    switch (cp->getPlanarConfiguration())
    {
      case EJ2KPC_restore:
        // determine auto default if not found or invalid
        if (imagePlanarConfiguration > 1)
          imagePlanarConfiguration = determinePlanarConfiguration(imageSopClass, imagePhotometricInterpretation);
        break;
      case EJ2KPC_auto:
        imagePlanarConfiguration = determinePlanarConfiguration(imageSopClass, imagePhotometricInterpretation);
        break;
      case EJ2KPC_colorByPixel:
        imagePlanarConfiguration = 0;
        break;
      case EJ2KPC_colorByPlane:
        imagePlanarConfiguration = 1;
        break;
    }
  }

  // get the size of all the fragments
  if (result.good())
  {
    Uint32 fragmentsForThisFrame2 = fragmentsForThisFrame;
    Uint32 currentItem2 = currentItem;

    while (result.good() && fragmentsForThisFrame2--)
    {
      result = fromPixSeq->getItem(pixItem, currentItem2++);
      if (result.good() && pixItem)
      {
        fragmentLength = pixItem->getLength();
        if (result.good())
          compressedSize += fragmentLength;
      }
    }
  }

  // get the compressed data
  if (result.good())
  {
    Uint32 offset = 0;
    j2kData = new Uint8[compressedSize];

    while (result.good() && fragmentsForThisFrame--)
    {
      result = fromPixSeq->getItem(pixItem, currentItem++);
      if (result.good() && pixItem)
      {
        fragmentLength = pixItem->getLength();
        result = pixItem->getUint8Array(j2kFragmentData);
        if (result.good() && j2kFragmentData)
        {
          memcpy(&j2kData[offset], j2kFragmentData, fragmentLength);
          offset += fragmentLength;
        }
      }
    }
  }

  if (result.good())
  {
    // Decode using OpenJPEG
    result = decodeJPEG2000(j2kData, compressedSize, buffer, bufSize,
                           imageColumns, imageRows, imageSamplesPerPixel, bytesPerSample,
                           pixelRepresentation);

    if (result.good())
    {
      // Handle planar configuration conversion if needed for color images
      if (imageSamplesPerPixel == 3 && imagePlanarConfiguration == 1)
      {
        // Need to convert from color-by-pixel (OpenJPEG output) to color-by-plane
        DCMJP2KCS_DEBUG("converting to planar configuration 1");
        if (bytesPerSample == 1)
          result = createPlanarConfiguration1Byte(OFreinterpret_cast(Uint8*, buffer), imageColumns, imageRows);
        else
          result = createPlanarConfiguration1Word(OFreinterpret_cast(Uint16*, buffer), imageColumns, imageRows);
      }

      // Handle byte swapping for 16-bit data
      if (result.good() && bytesPerSample == 2)
      {
        result = swapIfNecessary(gLocalByteOrder, EBO_LittleEndian, buffer, bufSize, sizeof(Uint16));
      }

      // update planar configuration if we are decoding a color image
      if (result.good() && (imageSamplesPerPixel > 1))
      {
        dataset->putAndInsertUint16(DCM_PlanarConfiguration, imagePlanarConfiguration);
      }
    }
  }

  delete[] j2kData;
  return result;
#endif
}


OFCondition DJ2KDecoderBase::encode(
    const Uint16 * /* pixelData */,
    const Uint32 /* length */,
    const DcmRepresentationParameter * /* toRepParam */,
    DcmPixelSequence * & /* pixSeq */,
    const DcmCodecParameter * /* cp */,
    DcmStack & /* objStack */,
    OFBool& /* removeOldRep */) const
{
  // we are a decoder only
  return EC_IllegalCall;
}


OFCondition DJ2KDecoderBase::encode(
    const E_TransferSyntax /* fromRepType */,
    const DcmRepresentationParameter * /* fromRepParam */,
    DcmPixelSequence * /* fromPixSeq */,
    const DcmRepresentationParameter * /* toRepParam */,
    DcmPixelSequence * & /* toPixSeq */,
    const DcmCodecParameter * /* cp */,
    DcmStack & /* objStack */,
    OFBool& /* removeOldRep */) const
{
  // we don't support re-coding for now.
  return EC_IllegalCall;
}


OFCondition DJ2KDecoderBase::determineDecompressedColorModel(
    const DcmRepresentationParameter * /* fromParam */,
    DcmPixelSequence * /* fromPixSeq */,
    const DcmCodecParameter * /* cp */,
    DcmItem * dataset,
    OFString & decompressedColorModel) const
{
  OFCondition result = EC_IllegalParameter;
  if (dataset != NULL)
  {
    // retrieve color model from given dataset
    result = dataset->findAndGetOFString(DCM_PhotometricInterpretation, decompressedColorModel);
    if (result == EC_TagNotFound)
    {
        DCMJP2KCS_WARN("mandatory element PhotometricInterpretation " << DCM_PhotometricInterpretation << " is missing");
        result = EC_MissingAttribute;
    }
    else if (result.bad())
    {
        DCMJP2KCS_WARN("cannot retrieve value of element PhotometricInterpretation " << DCM_PhotometricInterpretation << ": " << result.text());
    }
    else if (decompressedColorModel.empty())
    {
        DCMJP2KCS_WARN("no value for mandatory element PhotometricInterpretation " << DCM_PhotometricInterpretation);
        result = EC_MissingValue;
    }
  }
  return result;
}


Uint16 DJ2KDecoderBase::determinePlanarConfiguration(
  const OFString& sopClassUID,
  const OFString& photometricInterpretation)
{
  // Hardcopy Color Image always requires color-by-plane
  if (sopClassUID == UID_RETIRED_HardcopyColorImageStorage) return 1;

  // The 1996 Ultrasound Image IODs require color-by-plane if color model is YBR_FULL.
  if (photometricInterpretation == "YBR_FULL")
  {
    if ((sopClassUID == UID_UltrasoundMultiframeImageStorage)
       ||(sopClassUID == UID_UltrasoundImageStorage)) return 1;
  }

  // default for all other cases
  return 0;
}

Uint32 DJ2KDecoderBase::computeNumberOfFragments(
  Sint32 numberOfFrames,
  Uint32 currentFrame,
  Uint32 startItem,
  OFBool ignoreOffsetTable,
  DcmPixelSequence * pixSeq)
{
  unsigned long numItems = pixSeq->card();
  DcmPixelItem *pixItem = NULL;

  // We first check the simple cases
  if ((numberOfFrames <= 1) || (currentFrame + 1 == OFstatic_cast(Uint32, numberOfFrames)))
  {
    // single-frame image or last frame. All remaining fragments belong to this frame
    return (numItems - startItem);
  }
  if (OFstatic_cast(Uint32, numberOfFrames + 1) == numItems)
  {
    // multi-frame image with one fragment per frame
    return 1;
  }

  OFCondition result = EC_Normal;
  if (! ignoreOffsetTable)
  {
    // Check the offset table if present
    result = pixSeq->getItem(pixItem, 0);
    if (result.good() && pixItem)
    {
      Uint32 offsetTableLength = pixItem->getLength();
      if (offsetTableLength == (OFstatic_cast(Uint32, numberOfFrames) * 4))
      {
        Uint8 *offsetData = NULL;
        result = pixItem->getUint8Array(offsetData);
        if (result.good() && offsetData)
        {
          Uint32 *offsetData32 = OFreinterpret_cast(Uint32 *, offsetData);
          Uint32 offset = offsetData32[currentFrame+1];
          swapIfNecessary(gLocalByteOrder, EBO_LittleEndian, &offset, sizeof(Uint32), sizeof(Uint32));

          Uint32 byteCount = 0;
          Uint32 fragmentIndex = 1;
          while ((byteCount < offset) && (fragmentIndex < numItems))
          {
             pixItem = NULL;
             result = pixSeq->getItem(pixItem, fragmentIndex++);
             if (result.good() && pixItem)
             {
               byteCount += pixItem->getLength() + 8;
               if ((byteCount == offset) && (fragmentIndex > startItem))
               {
                 return fragmentIndex - startItem;
               }
             }
             else break;
          }
        }
      }
    }
  }

  // Try to detect JPEG 2000 SOC markers
  Uint32 nextItem = startItem;
  Uint8 *fragmentData = NULL;
  while (++nextItem < numItems)
  {
    pixItem = NULL;
    result = pixSeq->getItem(pixItem, nextItem);
    if (result.good() && pixItem)
    {
        fragmentData = NULL;
        result = pixItem->getUint8Array(fragmentData);
        if (result.good() && fragmentData && (pixItem->getLength() > 3))
        {
          if (isJPEG2000StartOfCodestream(fragmentData, pixItem->getLength()))
          {
            return (nextItem - startItem);
          }
        }
        else break;
    }
    else break;
  }

  // No way to determine the number of fragments per frame.
  return 0;
}


Uint16 DJ2KDecoderBase::decodedBitsAllocated(
    Uint16 /* bitsAllocated */,
    Uint16 bitsStored) const
{
  // this codec does not support images with less than 2 bits per sample
  if (bitsStored < 2) return 0;

  // for images with 2..8 bits per sample, BitsAllocated will be 8
  if (bitsStored <= 8) return 8;

  // for images with 9..16 bits per sample, BitsAllocated will be 16
  if (bitsStored <= 16) return 16;

  // this codec does not support images with more than 16 bits per sample
  return 0;
}


OFBool DJ2KDecoderBase::isJPEG2000StartOfCodestream(Uint8 *fragmentData, Uint32 fragmentLength)
{
  if (fragmentLength < 2) return OFFalse;

  // Check for JPEG 2000 codestream (SOC marker: 0xFF 0x4F)
  if (fragmentData[0] == 0xFF && fragmentData[1] == 0x4F)
    return OFTrue;

  // Check for JP2 file format signature
  if (fragmentLength >= 12 && memcmp(fragmentData, JP2_SIGNATURE, 12) == 0)
    return OFTrue;

  return OFFalse;
}


OFCondition DJ2KDecoderBase::createPlanarConfiguration1Byte(
  Uint8 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *buf = new Uint8[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels));
    Uint8 *s = buf;
    Uint8 *r = imageFrame;
    Uint8 *g = imageFrame + numPixels;
    Uint8 *b = imageFrame + (2*numPixels);
    for (unsigned long i=numPixels; i; i--)
    {
      *r++ = *s++;
      *g++ = *s++;
      *b++ = *s++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}


OFCondition DJ2KDecoderBase::createPlanarConfiguration1Word(
  Uint16 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint16 *buf = new Uint16[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels*sizeof(Uint16)));
    Uint16 *s = buf;
    Uint16 *r = imageFrame;
    Uint16 *g = imageFrame + numPixels;
    Uint16 *b = imageFrame + (2*numPixels);
    for (unsigned long i=numPixels; i; i--)
    {
      *r++ = *s++;
      *g++ = *s++;
      *b++ = *s++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}

OFCondition DJ2KDecoderBase::createPlanarConfiguration0Byte(
  Uint8 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *buf = new Uint8[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels));
    Uint8 *t = imageFrame;
    Uint8 *r = buf;
    Uint8 *g = buf + numPixels;
    Uint8 *b = buf + (2*numPixels);
    for (unsigned long i=numPixels; i; i--)
    {
      *t++ = *r++;
      *t++ = *g++;
      *t++ = *b++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}


OFCondition DJ2KDecoderBase::createPlanarConfiguration0Word(
  Uint16 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint16 *buf = new Uint16[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels*sizeof(Uint16)));
    Uint16 *t = imageFrame;
    Uint16 *r = buf;
    Uint16 *g = buf + numPixels;
    Uint16 *b = buf + (2*numPixels);
    for (unsigned long i=numPixels; i; i--)
    {
      *t++ = *r++;
      *t++ = *g++;
      *t++ = *b++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}
