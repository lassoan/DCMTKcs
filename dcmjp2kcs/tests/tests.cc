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
 *  Purpose: main test program for dcmjp2kcs
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/ofstd/oftest.h"

OFTEST_REGISTER(dcmjp2kcs_codec_parameter);
OFTEST_REGISTER(dcmjp2kcs_representation_parameter);
OFTEST_REGISTER(dcmjp2kcs_decoder_registration);
OFTEST_REGISTER(dcmjp2kcs_encoder_registration);
OFTEST_REGISTER(dcmjp2kcs_roundtrip_lossless_grayscale8);
OFTEST_REGISTER(dcmjp2kcs_roundtrip_lossless_grayscale16);
OFTEST_REGISTER(dcmjp2kcs_roundtrip_lossy_grayscale8);

OFTEST_MAIN("dcmjp2kcs")
