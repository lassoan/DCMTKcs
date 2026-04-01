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

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2kcs/dj2kcodece.h"

#include "dcmtk/ofstd/ofcast.h"      /* for casts */
#include "dcmtk/dcmdata/dcdatset.h"  /* for class DcmDataset */
#include "dcmtk/dcmdata/dcdeftag.h"  /* for tag constants */
#include "dcmtk/dcmdata/dcpixseq.h"  /* for class DcmPixelSequence */
#include "dcmtk/dcmdata/dcpxitem.h"  /* for class DcmPixelItem */
#include "dcmtk/dcmdata/dcvrpobw.h"  /* for class DcmPolymorphOBOW */
#include "dcmtk/dcmdata/dcuid.h"     /* for UIDs */
#include "dcmtk/dcmdata/dcerror.h"   /* for error constants */
#include "dcmtk/dcmdata/dcxfer.h"    /* for DcmXfer */
#include "dcmtk/dcmdata/dcofsetl.h"  /* for DcmOffsetList */
#include "dcmtk/dcmjp2kcs/dj2kparam.h"   /* for class DJ2KCodecParameter */
#include "dcmtk/dcmjp2kcs/dj2krparam.h"  /* for class DJ2KRepresentationParameter */
#include "dcmtk/dcmjp2kcs/dj2kutil.h"    /* for error codes */

#include <cstdlib>    /* for malloc, realloc, free */
#include <cstring>    /* for memcpy */

#ifdef WITH_OPENJPEG
#include <openjpeg.h>
#endif

// ============================================================================
// Memory stream helper for OpenJPEG output
// ============================================================================

#ifdef WITH_OPENJPEG

struct J2KMemStream
{
  Uint8  *data;
  size_t  size;      // current used size (bytes written so far)
  size_t  capacity;  // allocated capacity
  size_t  pos;       // current write/seek position
};

static OPJ_SIZE_T j2k_mem_write(void *buf, OPJ_SIZE_T nb, void *ud)
{
  J2KMemStream *ms = OFstatic_cast(J2KMemStream *, ud);
  if (ms->pos + nb > ms->capacity)
  {
    size_t newCapacity = ms->capacity;
    if (newCapacity == 0) newCapacity = 1024 * 1024;
    while (ms->pos + nb > newCapacity)
      newCapacity *= 2;
    Uint8 *newData = OFstatic_cast(Uint8 *, realloc(ms->data, newCapacity));
    if (newData == NULL) return OPJ_SIZE_T(-1);
    ms->data = newData;
    ms->capacity = newCapacity;
  }
  memcpy(ms->data + ms->pos, buf, nb);
  ms->pos += nb;
  if (ms->pos > ms->size) ms->size = ms->pos;
  return nb;
}

static OPJ_OFF_T j2k_mem_skip(OPJ_OFF_T nb, void *ud)
{
  J2KMemStream *ms = OFstatic_cast(J2KMemStream *, ud);
  ms->pos += OFstatic_cast(size_t, nb);
  if (ms->pos > ms->size) ms->size = ms->pos;
  return nb;
}

static OPJ_BOOL j2k_mem_seek(OPJ_OFF_T pos, void *ud)
{
  J2KMemStream *ms = OFstatic_cast(J2KMemStream *, ud);
  ms->pos = OFstatic_cast(size_t, pos);
  return OPJ_TRUE;
}

static void j2k_mem_free(void *ud)
{
  J2KMemStream *ms = OFstatic_cast(J2KMemStream *, ud);
  if (ms && ms->data)
  {
    free(ms->data);
    ms->data = NULL;
  }
}

#endif /* WITH_OPENJPEG */

// ============================================================================
// DJ2KLosslessEncoder / DJ2KEncoder
// ============================================================================

E_TransferSyntax DJ2KLosslessEncoder::supportedTransferSyntax() const
{
  return EXS_JPEG2000LosslessOnly;
}

E_TransferSyntax DJ2KEncoder::supportedTransferSyntax() const
{
  return EXS_JPEG2000;
}

// ============================================================================
// DJ2KEncoderBase
// ============================================================================

DJ2KEncoderBase::DJ2KEncoderBase()
: DcmCodec()
{
}

DJ2KEncoderBase::~DJ2KEncoderBase()
{
}

OFCondition DJ2KEncoderBase::decode(
    const DcmRepresentationParameter * /*fromRepParam*/,
    DcmPixelSequence * /*pixSeq*/,
    DcmPolymorphOBOW& /*uncompressedPixelData*/,
    const DcmCodecParameter * /*cp*/,
    const DcmStack & /*objStack*/,
    OFBool& /*removeOldRep*/) const
{
  return EC_IllegalCall;
}

OFCondition DJ2KEncoderBase::decodeFrame(
    const DcmRepresentationParameter * /*fromParam*/,
    DcmPixelSequence * /*fromPixSeq*/,
    const DcmCodecParameter * /*cp*/,
    DcmItem * /*dataset*/,
    Uint32 /*frameNo*/,
    Uint32& /*startFragment*/,
    void * /*buffer*/,
    Uint32 /*bufSize*/,
    OFString& /*decompressedColorModel*/) const
{
  return EC_IllegalCall;
}

OFCondition DJ2KEncoderBase::encode(
    const E_TransferSyntax /*fromRepType*/,
    const DcmRepresentationParameter * /*fromRepParam*/,
    DcmPixelSequence * /*fromPixSeq*/,
    const DcmRepresentationParameter * /*toRepParam*/,
    DcmPixelSequence * & /*toPixSeq*/,
    const DcmCodecParameter * /*cp*/,
    DcmStack & /*objStack*/,
    OFBool& /*removeOldRep*/) const
{
  return EC_IllegalCall;
}

OFBool DJ2KEncoderBase::canChangeCoding(
    const E_TransferSyntax oldRepType,
    const E_TransferSyntax newRepType) const
{
  // Can encode from any uncompressed (native) transfer syntax
  DcmXfer oldXfer(oldRepType);
  if (oldXfer.isPixelDataCompressed()) return OFFalse;
  return (newRepType == supportedTransferSyntax());
}

OFCondition DJ2KEncoderBase::determineDecompressedColorModel(
    const DcmRepresentationParameter * /*fromParam*/,
    DcmPixelSequence * /*fromPixSeq*/,
    const DcmCodecParameter * /*cp*/,
    DcmItem * /*dataset*/,
    OFString & /*decompressedColorModel*/) const
{
  return EC_IllegalCall;
}

Uint16 DJ2KEncoderBase::decodedBitsAllocated(
    Uint16 /* bitsAllocated */,
    Uint16 /* bitsStored */) const
{
  // encoding-only codec, decompression not supported
  return 0;
}

OFCondition DJ2KEncoderBase::encode(
    const Uint16 *pixelData,
    const Uint32 length,
    const DcmRepresentationParameter *toRepParam,
    DcmPixelSequence *& pixSeq,
    const DcmCodecParameter *cp,
    DcmStack & objStack,
    OFBool& removeOldRep) const
{
  removeOldRep = OFTrue;

  // Cast codec parameter
  const DJ2KCodecParameter *djcp = OFdynamic_cast(const DJ2KCodecParameter *, cp);
  if (djcp == NULL) return EC_IllegalCall;

  // Cast representation parameter (use defaults if NULL)
  DJ2KRepresentationParameter defaultRp(OFTrue, 0.0);
  const DJ2KRepresentationParameter *djrp = OFdynamic_cast(const DJ2KRepresentationParameter *, toRepParam);
  if (djrp == NULL) djrp = &defaultRp;

  // Retrieve the dataset from the stack
  // The stack top is the pixel data element; one level up is the dataset/item
  DcmStack localStack(objStack);
  (void) localStack.pop(); // pop pixel data element
  DcmObject *top = localStack.top();
  if (top == NULL) return EC_IllegalCall;

  DcmItem *dataset = OFdynamic_cast(DcmItem *, top);
  if (dataset == NULL) return EC_IllegalCall;

  // Read image attributes from dataset
  Uint16 rows = 0, cols = 0, spp = 0, bitsAlloc = 0, bitsStored = 0;
  Uint16 pixelRep = 0, planarConfig = 0;
  Sint32 numberOfFrames = 1;

  if (dataset->findAndGetUint16(DCM_Rows, rows).bad()) return EC_IllegalCall;
  if (dataset->findAndGetUint16(DCM_Columns, cols).bad()) return EC_IllegalCall;
  if (dataset->findAndGetUint16(DCM_SamplesPerPixel, spp).bad()) return EC_IllegalCall;
  if (dataset->findAndGetUint16(DCM_BitsAllocated, bitsAlloc).bad()) return EC_IllegalCall;
  if (dataset->findAndGetUint16(DCM_BitsStored, bitsStored).bad()) return EC_IllegalCall;
  dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRep);  // may be absent
  dataset->findAndGetUint16(DCM_PlanarConfiguration, planarConfig);  // optional
  dataset->findAndGetSint32(DCM_NumberOfFrames, numberOfFrames);
  if (numberOfFrames <= 0) numberOfFrames = 1;

  // Validate
  if (rows == 0 || cols == 0 || spp == 0 || bitsAlloc == 0 || bitsStored == 0)
    return EC_J2KCodecInvalidParameters;
  if (bitsAlloc != 8 && bitsAlloc != 16)
    return EC_J2KUnsupportedBitDepth;

  OFBool isSigned = (pixelRep != 0);
  OFBool planarByPlane = (planarConfig == 1) && (spp > 1);

  // Per-frame size in bytes
  size_t frameSize = OFstatic_cast(size_t, rows) * OFstatic_cast(size_t, cols)
                   * OFstatic_cast(size_t, spp) * (bitsAlloc / 8);

  // Determine whether to use lossless process
  OFBool lossless = (supportedTransferSyntax() == EXS_JPEG2000LosslessOnly) || djrp->useLosslessProcess();
  double compressionRatio = djrp->getCompressionRatio();
  if (lossless) compressionRatio = 0.0;
  else if (compressionRatio <= 0.0) compressionRatio = 10.0; // default lossy ratio

  // Encoder params
  OFBool createOffsetTable = djcp->getCreateOffsetTable();
  Uint32 fragmentSizeKB = djcp->getFragmentSize();
  Uint32 fragmentSizeBytes = (fragmentSizeKB > 0) ? (fragmentSizeKB * 1024) : 0;

  // Verify total pixel data length
  size_t totalExpected = frameSize * OFstatic_cast(size_t, numberOfFrames);
  if (OFstatic_cast(size_t, length) < totalExpected)
    return EC_J2KUncompressedBufferTooSmall;

  // Create output pixel sequence
  DcmPixelSequence *pixelSequence = new DcmPixelSequence(DCM_PixelSequenceTag);
  if (pixelSequence == NULL) return EC_MemoryExhausted;

  // Add empty offset table
  DcmPixelItem *offsetTable = new DcmPixelItem(DCM_PixelItemTag);
  if (offsetTable == NULL)
  {
    delete pixelSequence;
    return EC_MemoryExhausted;
  }
  pixelSequence->insert(offsetTable);

  DcmOffsetList offsetList;
  const Uint8 *framePtr = OFreinterpret_cast(const Uint8 *, pixelData);

  OFCondition result = EC_Normal;

  for (Sint32 frame = 0; frame < numberOfFrames && result.good(); ++frame)
  {
    Uint8 *compressedData = NULL;
    size_t compressedSize = 0;

    result = encodeFrame(
      framePtr + frame * frameSize,
      OFstatic_cast(Uint32, cols),
      OFstatic_cast(Uint32, rows),
      OFstatic_cast(Uint32, spp),
      OFstatic_cast(Uint32, bitsAlloc),
      OFstatic_cast(Uint32, bitsStored),
      isSigned,
      planarByPlane,
      lossless,
      compressionRatio,
      compressedData,
      compressedSize);

    if (result.good())
    {
      result = pixelSequence->storeCompressedFrame(
        offsetList,
        compressedData,
        OFstatic_cast(Uint32, compressedSize),
        fragmentSizeBytes);
      delete[] compressedData;
    }
  }

  if (result.bad())
  {
    delete pixelSequence;
    return result;
  }

  // Write offset table
  if (createOffsetTable)
  {
    result = offsetTable->createOffsetTable(offsetList);
  }

  if (result.bad())
  {
    delete pixelSequence;
    return result;
  }

  pixSeq = pixelSequence;

  // Update dataset metadata if this is a DcmDataset (not just a sequence item)
  DcmDataset *dcmDataset = OFdynamic_cast(DcmDataset *, dataset);

  if (dcmDataset != NULL)
  {
    J2K_UIDCreation uidCreation = djcp->getUIDCreation();
    OFBool convertToSC = djcp->getConvertToSC();

    if (!lossless)
    {
      // Mark image as lossy compressed
      dcmDataset->putAndInsertOFStringArray(DCM_LossyImageCompression, "01");
      dcmDataset->putAndInsertOFStringArray(DCM_LossyImageCompressionMethod, "ISO_15444_1");

      if (uidCreation != EJ2KUC_never)
      {
        result = DcmCodec::newInstance(dcmDataset);
        if (result.bad()) return result;
      }
    }
    else
    {
      if (uidCreation == EJ2KUC_always)
      {
        result = DcmCodec::newInstance(dcmDataset);
        if (result.bad()) return result;
      }
    }

    if (convertToSC)
    {
      result = DcmCodec::convertToSecondaryCapture(dcmDataset);
      if (result.bad()) return result;
    }
  }

  return EC_Normal;
}

OFCondition DJ2KEncoderBase::encodeFrame(
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
    size_t & compressedSize)
{
#ifdef WITH_OPENJPEG

  compressedData = NULL;
  compressedSize = 0;

  // Set up component parameters
  opj_image_cmptparm_t *cmptparms = new opj_image_cmptparm_t[samplesPerPixel];
  if (cmptparms == NULL) return EC_MemoryExhausted;

  memset(cmptparms, 0, sizeof(opj_image_cmptparm_t) * samplesPerPixel);
  for (Uint32 c = 0; c < samplesPerPixel; ++c)
  {
    cmptparms[c].prec = bitsStored;
    cmptparms[c].bpp  = bitsAllocated;
    cmptparms[c].sgnd = isSigned ? 1 : 0;
    cmptparms[c].dx   = 1;
    cmptparms[c].dy   = 1;
    cmptparms[c].w    = width;
    cmptparms[c].h    = height;
  }

  OPJ_COLOR_SPACE colorspace = (samplesPerPixel == 1) ? OPJ_CLRSPC_GRAY : OPJ_CLRSPC_SRGB;

  opj_image_t *image = opj_image_create(samplesPerPixel, cmptparms, colorspace);
  delete[] cmptparms;
  if (image == NULL) return EC_MemoryExhausted;

  image->x0 = 0;
  image->y0 = 0;
  image->x1 = width;
  image->y1 = height;

  Uint32 numPixels = width * height;

  // Fill image component data
  if (bitsAllocated == 8)
  {
    for (Uint32 comp = 0; comp < samplesPerPixel; ++comp)
    {
      OPJ_INT32 *dst = image->comps[comp].data;
      if (planarByPlane)
      {
        // Color-by-plane: each component is a separate plane
        const Uint8 *src = frameData + comp * numPixels;
        if (isSigned)
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, OFreinterpret_cast(const Sint8 *, src)[px]);
        }
        else
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, src[px]);
        }
      }
      else
      {
        // Color-by-pixel (interleaved): samples are interleaved
        const Uint8 *src = frameData + comp;
        if (isSigned)
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, OFreinterpret_cast(const Sint8 *, src)[px * samplesPerPixel]);
        }
        else
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, src[px * samplesPerPixel]);
        }
      }
    }
  }
  else // 16-bit
  {
    for (Uint32 comp = 0; comp < samplesPerPixel; ++comp)
    {
      OPJ_INT32 *dst = image->comps[comp].data;
      if (planarByPlane)
      {
        // Color-by-plane
        const Uint16 *src = OFreinterpret_cast(const Uint16 *, frameData) + comp * numPixels;
        if (isSigned)
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, OFstatic_cast(Sint16, src[px]));
        }
        else
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, src[px]);
        }
      }
      else
      {
        // Color-by-pixel (interleaved)
        const Uint16 *src = OFreinterpret_cast(const Uint16 *, frameData) + comp;
        if (isSigned)
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, OFstatic_cast(Sint16, src[px * samplesPerPixel]));
        }
        else
        {
          for (Uint32 px = 0; px < numPixels; ++px)
            dst[px] = OFstatic_cast(OPJ_INT32, src[px * samplesPerPixel]);
        }
      }
    }
  }

  // Set up encoder parameters
  opj_cparameters_t params;
  opj_set_default_encoder_parameters(&params);

  if (lossless)
  {
    params.irreversible = 0;
    params.tcp_rates[0] = 0;
    params.tcp_numlayers = 1;
    params.cp_disto_alloc = 1;
  }
  else
  {
    params.irreversible = 1;
    params.tcp_rates[0] = OFstatic_cast(float, compressionRatio);
    params.tcp_numlayers = 1;
    params.cp_disto_alloc = 1;
  }

  // Create codec
  opj_codec_t *codec = opj_create_compress(OPJ_CODEC_J2K);
  if (codec == NULL)
  {
    opj_image_destroy(image);
    return EC_MemoryExhausted;
  }

  // Suppress all messages
  opj_set_info_handler(codec, NULL, NULL);
  opj_set_warning_handler(codec, NULL, NULL);
  opj_set_error_handler(codec, NULL, NULL);

  if (!opj_setup_encoder(codec, &params, image))
  {
    opj_destroy_codec(codec);
    opj_image_destroy(image);
    return EC_J2KCodecInvalidParameters;
  }

  // Create memory stream for output
  J2KMemStream memStream;
  memStream.data     = NULL;
  memStream.size     = 0;
  memStream.capacity = 0;
  memStream.pos      = 0;

  opj_stream_t *stream = opj_stream_create(1 << 20, OPJ_FALSE);
  if (stream == NULL)
  {
    opj_destroy_codec(codec);
    opj_image_destroy(image);
    return EC_MemoryExhausted;
  }

  opj_stream_set_user_data(stream, &memStream, j2k_mem_free);
  opj_stream_set_user_data_length(stream, 0);
  opj_stream_set_write_function(stream, j2k_mem_write);
  opj_stream_set_skip_function(stream, j2k_mem_skip);
  opj_stream_set_seek_function(stream, j2k_mem_seek);

  OFBool encodeOk =
    opj_start_compress(codec, image, stream) &&
    opj_encode(codec, stream) &&
    opj_end_compress(codec, stream);

  if (!encodeOk)
  {
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    opj_image_destroy(image);
    // Note: j2k_mem_free is called by opj_stream_destroy for user data
    return EC_J2KInvalidCompressedData;
  }

  // Copy result from memStream into a newly allocated buffer
  size_t resultSize = memStream.size;
  Uint8 *resultData = NULL;
  if (resultSize > 0)
  {
    resultData = new Uint8[resultSize];
    if (resultData == NULL)
    {
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      opj_image_destroy(image);
      return EC_MemoryExhausted;
    }
    memcpy(resultData, memStream.data, resultSize);
  }

  // Cleanup (j2k_mem_free is invoked by opj_stream_destroy via the user data destructor)
  opj_stream_destroy(stream);
  opj_destroy_codec(codec);
  opj_image_destroy(image);

  compressedData = resultData;
  compressedSize = resultSize;

  return EC_Normal;

#else
  // Suppress unused parameter warnings
  (void)frameData; (void)width; (void)height; (void)samplesPerPixel;
  (void)bitsAllocated; (void)bitsStored; (void)isSigned; (void)planarByPlane;
  (void)lossless; (void)compressionRatio;
  compressedData = NULL;
  compressedSize = 0;
  return EC_J2KOpenJPEGNotAvailable;
#endif
}
