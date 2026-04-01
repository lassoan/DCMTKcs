/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: enumerations, error constants and helper functions for dcmjp2kcs
 *
 */

#ifndef DCMJP2KCS_DJ2KUTIL_H
#define DCMJP2KCS_DJ2KUTIL_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/ofcond.h"   /* for class OFCondition */
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/dcmjp2kcs/dj2kdefine.h"

// Module number for external dcmjp2kcs module (not in DCMTK's central dcerror.h)
const unsigned short OFM_dcmjp2kcs = 200;


// global definitions for logging mechanism provided by the oflog module

extern DCMTK_DCMJP2KCS_EXPORT OFLogger DCM_dcmjp2kcsLogger;

#define DCMJP2KCS_TRACE(msg) OFLOG_TRACE(DCM_dcmjp2kcsLogger, msg)
#define DCMJP2KCS_DEBUG(msg) OFLOG_DEBUG(DCM_dcmjp2kcsLogger, msg)
#define DCMJP2KCS_INFO(msg)  OFLOG_INFO(DCM_dcmjp2kcsLogger, msg)
#define DCMJP2KCS_WARN(msg)  OFLOG_WARN(DCM_dcmjp2kcsLogger, msg)
#define DCMJP2KCS_ERROR(msg) OFLOG_ERROR(DCM_dcmjp2kcsLogger, msg)
#define DCMJP2KCS_FATAL(msg) OFLOG_FATAL(DCM_dcmjp2kcsLogger, msg)


// include this file in doxygen documentation

/** @file dj2kutil.h
 *  @brief type definitions and constants for the dcmjp2kcs module
 */


/** describes the condition under which a compressed or decompressed image
 *  receives a new SOP instance UID.
 */
enum J2K_UIDCreation
{
  /** Upon decompression never assign new SOP instance UID.
   */
  EJ2KUC_default,

  /// always assign new SOP instance UID on decompression
  EJ2KUC_always,

  /// never assign new SOP instance UID
  EJ2KUC_never
};

/** describes how the decoder should handle planar configuration of
 *  decompressed color images.
 */
enum J2K_PlanarConfiguration
{
  /// restore planar configuration as indicated in data set
  EJ2KPC_restore,

  /** automatically determine whether color-by-plane is required from
   *  the SOP Class UID and decompressed photometric interpretation
   */
  EJ2KPC_auto,

  /// always create color-by-pixel planar configuration
  EJ2KPC_colorByPixel,

  /// always create color-by-plane planar configuration
  EJ2KPC_colorByPlane
};


// CONDITION CONSTANTS

/// error condition constant: Too small buffer used for image data (internal error)
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KUncompressedBufferTooSmall;

/// error condition constant: Too small buffer used for compressed image data (internal error)
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KCompressedBufferTooSmall;

/// error condition constant: The image uses some features which the codec does not support
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KCodecUnsupportedImageType;

/// error condition constant: The codec was fed with invalid parameters (e.g. height = -1)
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KCodecInvalidParameters;

/// error condition constant: The codec was fed with unsupported parameters (e.g. 32 bit per sample)
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KCodecUnsupportedValue;

/// error condition constant: The compressed image is invalid
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KInvalidCompressedData;

/// error condition constant: Unsupported bit depth in JPEG 2000 transfer syntax
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KUnsupportedBitDepth;

/// error condition constant: Cannot compute number of fragments for JPEG 2000 frame
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KCannotComputeNumberOfFragments;

/// error condition constant: Image data mismatch between DICOM header and JPEG 2000 bitstream
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KImageDataMismatch;

/// error condition constant: Unsupported photometric interpretation for JPEG 2000 compression
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KUnsupportedPhotometricInterpretation;

/// error condition constant: Unsupported pixel representation for JPEG 2000 compression
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KUnsupportedPixelRepresentation;

/// error condition constant: Unsupported type of image for JPEG 2000 compression
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KUnsupportedImageType;

/// error condition constant: OpenJPEG library not available
extern DCMTK_DCMJP2KCS_EXPORT const OFConditionConst EC_J2KOpenJPEGNotAvailable;

#endif
