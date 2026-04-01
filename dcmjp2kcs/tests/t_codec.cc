/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: Tests for JPEG 2000 codec parameter classes, registration, and
 *           encode/decode round-trips.
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/ofstd/oftest.h"
#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcxfer.h"

#include "dcmtk/dcmjp2kcs/dj2kparam.h"
#include "dcmtk/dcmjp2kcs/dj2krparam.h"
#include "dcmtk/dcmjp2kcs/dj2kdecode.h"
#include "dcmtk/dcmjp2kcs/dj2kencode.h"
#include "dcmtk/dcmjp2kcs/dj2kutil.h"

#include <cstring> /* for memcmp */


// ============================================================================
// Helper: build a minimal grayscale DICOM dataset with synthetic pixel data.
// Returns false if something goes wrong.
// ============================================================================

static OFBool buildGrayscaleDataset(DcmDataset &dataset,
                                    Uint16 rows, Uint16 cols,
                                    Uint16 bitsAllocated)
{
    OFCHECK(dataset.putAndInsertUint16(DCM_Rows, rows).good());
    OFCHECK(dataset.putAndInsertUint16(DCM_Columns, cols).good());
    OFCHECK(dataset.putAndInsertUint16(DCM_BitsAllocated, bitsAllocated).good());
    OFCHECK(dataset.putAndInsertUint16(DCM_BitsStored, bitsAllocated).good());
    OFCHECK(dataset.putAndInsertUint16(DCM_HighBit, (Uint16)(bitsAllocated - 1)).good());
    OFCHECK(dataset.putAndInsertUint16(DCM_SamplesPerPixel, 1).good());
    OFCHECK(dataset.putAndInsertUint16(DCM_PixelRepresentation, 0).good());
    OFCHECK(dataset.putAndInsertOFStringArray(DCM_PhotometricInterpretation, "MONOCHROME2").good());
    OFCHECK(dataset.putAndInsertString(DCM_SOPClassUID, UID_CTImageStorage).good());
    OFCHECK(dataset.putAndInsertString(DCM_SOPInstanceUID, "1.2.3.4.5.6.7.8.9").good());

    const unsigned long numPixels = (unsigned long)rows * cols;

    if (bitsAllocated == 8)
    {
        Uint8 *pixels = new Uint8[numPixels];
        for (unsigned long i = 0; i < numPixels; i++)
            pixels[i] = OFstatic_cast(Uint8, i % 256);
        OFCondition result = dataset.putAndInsertUint8Array(DCM_PixelData, pixels, numPixels);
        delete[] pixels;
        OFCHECK(result.good());
        return result.good();
    }
    else /* 16-bit */
    {
        Uint16 *pixels = new Uint16[numPixels];
        for (unsigned long i = 0; i < numPixels; i++)
            pixels[i] = OFstatic_cast(Uint16, i % 65536);
        OFCondition result = dataset.putAndInsertUint16Array(DCM_PixelData, pixels, numPixels);
        delete[] pixels;
        OFCHECK(result.good());
        return result.good();
    }
}


// ============================================================================
// Test: DJ2KCodecParameter construction and accessors
// ============================================================================

OFTEST(dcmjp2kcs_codec_parameter)
{
    // Decoder constructor (existing)
    DJ2KCodecParameter p1(EJ2KUC_always, EJ2KPC_colorByPixel, OFTrue, OFTrue);
    OFCHECK_EQUAL(p1.getUIDCreation(), EJ2KUC_always);
    OFCHECK_EQUAL(p1.getPlanarConfiguration(), EJ2KPC_colorByPixel);
    OFCHECK(p1.ignoreOffsetTable());
    OFCHECK(p1.getForceSingleFragmentPerFrame());

    // Default decoder constructor
    DJ2KCodecParameter p2;
    OFCHECK_EQUAL(p2.getUIDCreation(), EJ2KUC_default);
    OFCHECK_EQUAL(p2.getPlanarConfiguration(), EJ2KPC_restore);
    OFCHECK(!p2.ignoreOffsetTable());
    OFCHECK(!p2.getForceSingleFragmentPerFrame());

    // Encoder constructor
    DJ2KCodecParameter p3(OFTrue /*createOffsetTable*/, 0 /*fragmentSize*/,
                          EJ2KUC_never, OFFalse /*convertToSC*/);
    OFCHECK(p3.getCreateOffsetTable());
    OFCHECK_EQUAL(p3.getFragmentSize(), 0U);
    OFCHECK_EQUAL(p3.getUIDCreation(), EJ2KUC_never);
    OFCHECK(!p3.getConvertToSC());

    // Clone
    DcmCodecParameter *clone = p3.clone();
    OFCHECK(clone != NULL);
    delete clone;
}


// ============================================================================
// Test: DJ2KRepresentationParameter construction and equality
// ============================================================================

OFTEST(dcmjp2kcs_representation_parameter)
{
    // Default: lossless
    DJ2KRepresentationParameter rp1;
    OFCHECK(rp1.useLosslessProcess());
    OFCHECK_EQUAL(rp1.getCompressionRatio(), 0.0);

    // Lossless explicit
    DJ2KRepresentationParameter rp2(OFTrue, 0.0);
    OFCHECK(rp2.useLosslessProcess());
    OFCHECK(rp1 == rp2);

    // Lossy
    DJ2KRepresentationParameter rp3(OFFalse, 10.0);
    OFCHECK(!rp3.useLosslessProcess());
    OFCHECK_EQUAL(rp3.getCompressionRatio(), 10.0);
    OFCHECK(!(rp1 == rp3));

    // Lossy with different ratio
    DJ2KRepresentationParameter rp4(OFFalse, 20.0);
    OFCHECK(!(rp3 == rp4));

    // Clone
    DcmRepresentationParameter *clone = rp3.clone();
    OFCHECK(clone != NULL);
    OFCHECK(*clone == rp3);
    delete clone;

    // className
    OFCHECK(OFString(rp1.className()) == "DJ2KRepresentationParameter");
}


// ============================================================================
// Test: DJ2KDecoderRegistration register / cleanup
// ============================================================================

OFTEST(dcmjp2kcs_decoder_registration)
{
    // Register codecs
    DJ2KDecoderRegistration::registerCodecs();

    // Re-registering must be safe (silently ignored)
    DJ2KDecoderRegistration::registerCodecs();

    // getLibraryVersionString must return a non-empty string
    OFString version = DJ2KDecoderRegistration::getLibraryVersionString();
    OFCHECK(!version.empty());

    // Cleanup
    DJ2KDecoderRegistration::cleanup();

    // Double cleanup must be safe
    DJ2KDecoderRegistration::cleanup();
}


// ============================================================================
// Test: DJ2KEncoderRegistration register / cleanup
// ============================================================================

OFTEST(dcmjp2kcs_encoder_registration)
{
    DJ2KEncoderRegistration::registerCodecs();
    DJ2KEncoderRegistration::registerCodecs(); // idempotent

    OFString version = DJ2KEncoderRegistration::getLibraryVersionString();
    OFCHECK(!version.empty());

    DJ2KEncoderRegistration::cleanup();
    DJ2KEncoderRegistration::cleanup(); // safe to call twice
}


// ============================================================================
// Round-trip helper: encode then decode a dataset, verify pixel data matches.
// ============================================================================

static void doRoundTrip(DcmDataset &dataset, E_TransferSyntax compressedXfer,
                        const DJ2KRepresentationParameter &rp,
                        OFBool checkPixelExact)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: "
                     DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    // Save a copy of the original pixel data before encoding
    const Uint8 *origPixels8 = NULL;
    const Uint16 *origPixels16 = NULL;
    unsigned long origLength = 0;
    Uint16 bitsAllocated = 0;
    dataset.findAndGetUint16(DCM_BitsAllocated, bitsAllocated);

    if (bitsAllocated == 8)
        dataset.findAndGetUint8Array(DCM_PixelData, origPixels8, &origLength);
    else
        dataset.findAndGetUint16Array(DCM_PixelData, origPixels16, &origLength);

    // Make our own copy since the dataset will overwrite the buffer
    Uint8 *savedPixels = new Uint8[origLength * (bitsAllocated / 8)];
    if (bitsAllocated == 8 && origPixels8)
        memcpy(savedPixels, origPixels8, origLength);
    else if (bitsAllocated == 16 && origPixels16)
        memcpy(savedPixels, origPixels16, origLength * 2);

    // --- Encode ---
    DJ2KEncoderRegistration::registerCodecs();
    OFCondition result = dataset.chooseRepresentation(compressedXfer,
                                                      &rp);
    OFCHECK(result.good());
    if (result.good())
    {
        OFCHECK(dataset.canWriteXfer(compressedXfer));
    }
    DJ2KEncoderRegistration::cleanup();

    if (result.bad())
    {
        delete[] savedPixels;
        return;
    }

    // --- Decode ---
    DJ2KDecoderRegistration::registerCodecs();
    result = dataset.chooseRepresentation(EXS_LittleEndianExplicit, NULL);
    OFCHECK(result.good());
    DJ2KDecoderRegistration::cleanup();

    if (result.bad())
    {
        delete[] savedPixels;
        return;
    }

    // Verify pixel data
    if (checkPixelExact)
    {
        if (bitsAllocated == 8)
        {
            const Uint8 *decodedPixels = NULL;
            unsigned long decodedLength = 0;
            OFCHECK(dataset.findAndGetUint8Array(DCM_PixelData, decodedPixels,
                                                 &decodedLength).good());
            OFCHECK_EQUAL(decodedLength, origLength);
            if (decodedPixels && decodedLength == origLength)
                OFCHECK(memcmp(savedPixels, decodedPixels, origLength) == 0);
        }
        else
        {
            const Uint16 *decodedPixels = NULL;
            unsigned long decodedLength = 0;
            OFCHECK(dataset.findAndGetUint16Array(DCM_PixelData, decodedPixels,
                                                  &decodedLength).good());
            OFCHECK_EQUAL(decodedLength, origLength);
            if (decodedPixels && decodedLength == origLength)
                OFCHECK(memcmp(savedPixels, decodedPixels, origLength * 2) == 0);
        }
    }

    delete[] savedPixels;
}


// ============================================================================
// Test: lossless round-trip with 8-bit grayscale
// ============================================================================

OFTEST(dcmjp2kcs_roundtrip_lossless_grayscale8)
{
#ifndef WITH_OPENJPEG
    OFCHECK_FAIL("test skipped: OpenJPEG support not compiled in");
#else
    DcmDataset dataset;
    if (!buildGrayscaleDataset(dataset, 16, 16, 8))
        return;

    DJ2KRepresentationParameter rp(OFTrue); // lossless
    doRoundTrip(dataset, EXS_JPEG2000LosslessOnly, rp, OFTrue /*exact*/);
#endif
}


// ============================================================================
// Test: lossless round-trip with 16-bit grayscale
// ============================================================================

OFTEST(dcmjp2kcs_roundtrip_lossless_grayscale16)
{
#ifndef WITH_OPENJPEG
    OFCHECK_FAIL("test skipped: OpenJPEG support not compiled in");
#else
    DcmDataset dataset;
    if (!buildGrayscaleDataset(dataset, 16, 16, 16))
        return;

    DJ2KRepresentationParameter rp(OFTrue); // lossless
    doRoundTrip(dataset, EXS_JPEG2000LosslessOnly, rp, OFTrue /*exact*/);
#endif
}


// ============================================================================
// Test: lossy round-trip with 8-bit grayscale (pixel exactness not checked)
// ============================================================================

OFTEST(dcmjp2kcs_roundtrip_lossy_grayscale8)
{
#ifndef WITH_OPENJPEG
    OFCHECK_FAIL("test skipped: OpenJPEG support not compiled in");
#else
    DcmDataset dataset;
    if (!buildGrayscaleDataset(dataset, 16, 16, 8))
        return;

    DJ2KRepresentationParameter rp(OFFalse, 5.0); // lossy, ratio 5:1
    doRoundTrip(dataset, EXS_JPEG2000, rp, OFFalse /*not exact*/);
#endif
}
