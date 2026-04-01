/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmjp2kcs
 *
 *
 *  Purpose: Contains preprocessor definitions
 *
 */


#ifndef DCMJP2KCS_DJ2KDEFINE_H
#define DCMJP2KCS_DJ2KDEFINE_H

#include "dcmtk/config/osconfig.h"

#include "dcmtk/ofstd/ofdefine.h"


#ifdef dcmjp2kcs_EXPORTS
#define DCMTK_DCMJP2KCS_EXPORT DCMTK_DECL_EXPORT
#else
#define DCMTK_DCMJP2KCS_EXPORT DCMTK_DECL_IMPORT
#endif


#endif
