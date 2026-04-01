/*
 *
 *  Copyright (C) DCMTKcs community
 *
 *
 *  Module:  dcmcjp2kcs
 *
 *
 *  Purpose: Encode DICOM file to JPEG 2000 transfer syntax
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"      /* for dcmtk version name */
#include "dcmtk/dcmimage/diregist.h"  /* include to support color images */
#include "dcmtk/dcmjp2kcs/dj2kutil.h"    /* for enums */
#include "dcmtk/dcmjp2kcs/dj2kencode.h"  /* for class DJ2KEncoderRegistration */
#include "dcmtk/dcmjp2kcs/dj2krparam.h"  /* for class DJ2KRepresentationParameter */

#ifdef USE_LICENSE_FILE
#include "oflice.h"
#endif

#ifndef OFFIS_CONSOLE_APPLICATION
#define OFFIS_CONSOLE_APPLICATION "dcmcjp2kcs"
#endif

static OFLogger dcmcjp2kcsLogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_APPLICATION);

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

// ********************************************


#define SHORTCOL 3
#define LONGCOL 21

int main(int argc, char *argv[])
{
  const char *opt_ifname = NULL;
  const char *opt_ofname = NULL;

  // input options
  E_FileReadMode opt_readMode = ERM_autoDetect;
  E_TransferSyntax opt_ixfer = EXS_Unknown;

  // JPEG 2000 encoding options
  E_TransferSyntax opt_oxfer = EXS_JPEG2000LosslessOnly;
  OFBool opt_useLosslessProcess = OFTrue;
  double opt_compressionRatio = 10.0;

  // encapsulated pixel data encoding options
  OFCmdUnsignedInt opt_fragmentSize = 0; // 0=unlimited
  OFBool           opt_createOffsetTable = OFTrue;
  J2K_UIDCreation  opt_uidcreation = EJ2KUC_default;
  OFBool           opt_secondarycapture = OFFalse;

  // output options
  E_GrpLenEncoding opt_oglenc = EGL_recalcGL;
  E_EncodingType opt_oenctype = EET_ExplicitLength;
  E_PaddingEncoding opt_opadenc = EPD_noChange;
  OFCmdUnsignedInt opt_filepad = 0;
  OFCmdUnsignedInt opt_itempad = 0;

#ifdef USE_LICENSE_FILE
LICENSE_FILE_DECLARATIONS
#endif

  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, "Encode DICOM file to JPEG 2000 transfer syntax", rcsid);
  OFCommandLine cmd;
  cmd.setOptionColumns(LONGCOL, SHORTCOL);
  cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

  cmd.addParam("dcmfile-in",  "DICOM input filename to be converted\n(\"-\" for stdin)");
  cmd.addParam("dcmfile-out", "DICOM output filename\n(\"-\" for stdout)");

  cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
    cmd.addOption("--help",                     "-h",     "print this help text and exit", OFCommandLine::AF_Exclusive);
    cmd.addOption("--version",                            "print version information and exit", OFCommandLine::AF_Exclusive);
    OFLog::addOptions(cmd);

#ifdef USE_LICENSE_FILE
LICENSE_FILE_DECLARE_COMMAND_LINE_OPTIONS
#endif

  cmd.addGroup("input options:");
    cmd.addSubGroup("input file format:");
      cmd.addOption("--read-file",              "+f",     "read file format or data set (default)");
      cmd.addOption("--read-file-only",         "+fo",    "read file format only");
      cmd.addOption("--read-dataset",           "-f",     "read data set without file meta information");
    cmd.addSubGroup("input transfer syntax:");
      cmd.addOption("--read-xfer-auto",         "-t=",    "use TS recognition (default)");
      cmd.addOption("--read-xfer-detect",       "-td",    "ignore TS specified in the file meta header");
      cmd.addOption("--read-xfer-little",       "-te",    "read with explicit VR little endian TS");
      cmd.addOption("--read-xfer-big",          "-tb",    "read with explicit VR big endian TS");
      cmd.addOption("--read-xfer-implicit",     "-ti",    "read with implicit VR little endian TS");

  cmd.addGroup("JPEG 2000 encoding options:");
    cmd.addSubGroup("JPEG 2000 process:");
      cmd.addOption("--encode-lossless",        "+el",    "encode JPEG 2000 lossless (default)");
      cmd.addOption("--encode-lossy",           "+ey",    "encode JPEG 2000 lossy");
    cmd.addSubGroup("JPEG 2000 compression ratio (lossy only):");
      cmd.addOption("--compression-ratio",      "+cr", 1, "[r]atio: float (default: 10.0)",
                                                          "set lossy compression ratio (e.g. 10.0 for 10:1)");

  cmd.addGroup("encapsulated pixel data encoding options:");
    cmd.addSubGroup("pixel data fragmentation:");
      cmd.addOption("--fragment-per-frame",     "+ff",    "encode each frame as one fragment (default)");
      cmd.addOption("--fragment-size",          "+fs", 1, "[s]ize: integer",
                                                          "limit fragment size to s kbytes");
    cmd.addSubGroup("basic offset table encoding:");
      cmd.addOption("--offset-table-create",    "+ot",    "create offset table (default)");
      cmd.addOption("--offset-table-empty",     "-ot",    "leave offset table empty");
    cmd.addSubGroup("SOP Class UID:");
      cmd.addOption("--class-default",          "+cd",    "keep SOP Class UID (default)");
      cmd.addOption("--class-sc",               "+cs",    "convert to Secondary Capture Image\n(implies --uid-always)");
    cmd.addSubGroup("SOP Instance UID:");
      cmd.addOption("--uid-default",            "+ud",    "assign new UID if lossy compression (default)");
      cmd.addOption("--uid-always",             "+ua",    "always assign new UID");
      cmd.addOption("--uid-never",              "+un",    "never assign new UID");

  cmd.addGroup("output options:");
    cmd.addSubGroup("post-1993 value representations:");
      cmd.addOption("--enable-new-vr",          "+u",     "enable support for new VRs (UN/UT) (default)");
      cmd.addOption("--disable-new-vr",         "-u",     "disable support for new VRs, convert to OB");
    cmd.addSubGroup("group length encoding:");
      cmd.addOption("--group-length-recalc",    "+g=",    "recalculate group lengths if present (default)");
      cmd.addOption("--group-length-create",    "+g",     "always write with group length elements");
      cmd.addOption("--group-length-remove",    "-g",     "always write without group length elements");
    cmd.addSubGroup("length encoding in sequences and items:");
      cmd.addOption("--length-explicit",        "+e",     "write with explicit lengths (default)");
      cmd.addOption("--length-undefined",       "-e",     "write with undefined lengths");
    cmd.addSubGroup("data set trailing padding:");
      cmd.addOption("--padding-retain",         "-p=",    "do not change padding (default)");
      cmd.addOption("--padding-off",            "-p",     "no padding");
      cmd.addOption("--padding-create",         "+p",  2, "[f]ile-pad [i]tem-pad: integer",
                                                          "align file on multiple of f bytes\nand items on multiple of i bytes");

    /* evaluate command line */
    prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
    if (app.parseCommandLine(cmd, argc, argv))
    {
      /* check exclusive options first */
      if (cmd.hasExclusiveOption())
      {
        if (cmd.findOption("--version"))
        {
          app.printHeader(OFTrue /*print host identifier*/);
          COUT << OFendl << "External libraries used:" << OFendl;
          COUT << "- " << DJ2KEncoderRegistration::getLibraryVersionString() << OFendl;
          return 0;
        }
      }

      /* command line parameters */

      cmd.getParam(1, opt_ifname);
      cmd.getParam(2, opt_ofname);

      // general options
      OFLog::configureFromCommandLine(cmd, app);

#ifdef USE_LICENSE_FILE
LICENSE_FILE_EVALUATE_COMMAND_LINE_OPTIONS
#endif

      // input options
      // input file format
      cmd.beginOptionBlock();
      if (cmd.findOption("--read-file")) opt_readMode = ERM_autoDetect;
      if (cmd.findOption("--read-file-only")) opt_readMode = ERM_fileOnly;
      if (cmd.findOption("--read-dataset")) opt_readMode = ERM_dataset;
      cmd.endOptionBlock();

      // input transfer syntax
      cmd.beginOptionBlock();
      if (cmd.findOption("--read-xfer-auto"))
        opt_ixfer = EXS_Unknown;
      if (cmd.findOption("--read-xfer-detect"))
        dcmAutoDetectDatasetXfer.set(OFTrue);
      if (cmd.findOption("--read-xfer-little"))
      {
        app.checkDependence("--read-xfer-little", "--read-dataset", opt_readMode == ERM_dataset);
        opt_ixfer = EXS_LittleEndianExplicit;
      }
      if (cmd.findOption("--read-xfer-big"))
      {
        app.checkDependence("--read-xfer-big", "--read-dataset", opt_readMode == ERM_dataset);
        opt_ixfer = EXS_BigEndianExplicit;
      }
      if (cmd.findOption("--read-xfer-implicit"))
      {
        app.checkDependence("--read-xfer-implicit", "--read-dataset", opt_readMode == ERM_dataset);
        opt_ixfer = EXS_LittleEndianImplicit;
      }
      cmd.endOptionBlock();

      // JPEG 2000 encoding options
      cmd.beginOptionBlock();
      if (cmd.findOption("--encode-lossless"))
      {
        opt_oxfer = EXS_JPEG2000LosslessOnly;
        opt_useLosslessProcess = OFTrue;
      }
      if (cmd.findOption("--encode-lossy"))
      {
        opt_oxfer = EXS_JPEG2000;
        opt_useLosslessProcess = OFFalse;
      }
      cmd.endOptionBlock();

      // compression ratio (lossy only)
      if (cmd.findOption("--compression-ratio"))
      {
        app.checkConflict("--compression-ratio", "--encode-lossless", opt_oxfer == EXS_JPEG2000LosslessOnly);
        OFCmdFloat ratio = 10.0;
        app.checkValue(cmd.getValueAndCheckMin(ratio, 1.0));
        opt_compressionRatio = OFstatic_cast(double, ratio);
      }

      // encapsulated pixel data encoding options
      // pixel data fragmentation options
      cmd.beginOptionBlock();
      if (cmd.findOption("--fragment-per-frame")) opt_fragmentSize = 0;
      if (cmd.findOption("--fragment-size"))
      {
        app.checkValue(cmd.getValueAndCheckMin(opt_fragmentSize, OFstatic_cast(OFCmdUnsignedInt, 1)));
      }
      cmd.endOptionBlock();

      // basic offset table encoding options
      cmd.beginOptionBlock();
      if (cmd.findOption("--offset-table-create")) opt_createOffsetTable = OFTrue;
      if (cmd.findOption("--offset-table-empty")) opt_createOffsetTable = OFFalse;
      cmd.endOptionBlock();

      // SOP Class UID options
      cmd.beginOptionBlock();
      if (cmd.findOption("--class-default")) opt_secondarycapture = OFFalse;
      if (cmd.findOption("--class-sc")) opt_secondarycapture = OFTrue;
      cmd.endOptionBlock();

      // SOP Instance UID options
      cmd.beginOptionBlock();
      if (cmd.findOption("--uid-default")) opt_uidcreation = EJ2KUC_default;
      if (cmd.findOption("--uid-always")) opt_uidcreation = EJ2KUC_always;
      if (cmd.findOption("--uid-never")) opt_uidcreation = EJ2KUC_never;
      cmd.endOptionBlock();

      // output options
      // post-1993 value representations
      cmd.beginOptionBlock();
      if (cmd.findOption("--enable-new-vr")) dcmEnableGenerationOfNewVRs();
      if (cmd.findOption("--disable-new-vr")) dcmDisableGenerationOfNewVRs();
      cmd.endOptionBlock();

      // group length encoding
      cmd.beginOptionBlock();
      if (cmd.findOption("--group-length-recalc")) opt_oglenc = EGL_recalcGL;
      if (cmd.findOption("--group-length-create")) opt_oglenc = EGL_withGL;
      if (cmd.findOption("--group-length-remove")) opt_oglenc = EGL_withoutGL;
      cmd.endOptionBlock();

      // length encoding in sequences and items
      cmd.beginOptionBlock();
      if (cmd.findOption("--length-explicit")) opt_oenctype = EET_ExplicitLength;
      if (cmd.findOption("--length-undefined")) opt_oenctype = EET_UndefinedLength;
      cmd.endOptionBlock();

      // data set trailing padding
      cmd.beginOptionBlock();
      if (cmd.findOption("--padding-retain")) opt_opadenc = EPD_noChange;
      if (cmd.findOption("--padding-off")) opt_opadenc = EPD_withoutPadding;
      if (cmd.findOption("--padding-create"))
      {
        app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
        app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
        opt_opadenc = EPD_withPadding;
      }
      cmd.endOptionBlock();
    }

    /* print resource identifier */
    OFLOG_DEBUG(dcmcjp2kcsLogger, rcsid << OFendl);

    // register global compression codecs
    DJ2KEncoderRegistration::registerCodecs(
      opt_createOffsetTable,
      OFstatic_cast(Uint32, opt_fragmentSize),
      opt_uidcreation,
      opt_secondarycapture);

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFLOG_WARN(dcmcjp2kcsLogger, "no data dictionary loaded, "
           << "check environment variable: "
           << DCM_DICT_ENVIRONMENT_VARIABLE);
    }

    // open input file
    if ((opt_ifname == NULL) || (strlen(opt_ifname) == 0))
    {
      OFLOG_FATAL(dcmcjp2kcsLogger, "invalid filename: <empty string>");
      return 1;
    }

    OFLOG_INFO(dcmcjp2kcsLogger, "reading input file " << opt_ifname);

    DcmFileFormat fileformat;
    OFCondition error = fileformat.loadFile(opt_ifname, opt_ixfer, EGL_noChange, DCM_MaxReadLength, opt_readMode);
    if (error.bad())
    {
      OFLOG_FATAL(dcmcjp2kcsLogger, error.text() << ": reading file: " <<  opt_ifname);
      return 1;
    }
    DcmDataset *dataset = fileformat.getDataset();

    DcmXfer original_xfer(dataset->getOriginalXfer());
    if (original_xfer.isPixelDataCompressed())
    {
      OFLOG_INFO(dcmcjp2kcsLogger, "DICOM file is already compressed, converting to uncompressed transfer syntax first");
      if (EC_Normal != dataset->chooseRepresentation(EXS_LittleEndianExplicit, NULL))
      {
        OFLOG_FATAL(dcmcjp2kcsLogger, "no conversion from compressed original to uncompressed transfer syntax possible");
        return 1;
      }
    }

    OFString sopClass;
    if (fileformat.getMetaInfo()->findAndGetOFString(DCM_MediaStorageSOPClassUID, sopClass).good())
    {
        /* check for DICOMDIR files */
        if (sopClass == UID_MediaStorageDirectoryStorage)
        {
            OFLOG_FATAL(dcmcjp2kcsLogger, "DICOMDIR files (Media Storage Directory Storage SOP Class) cannot be compressed!");
            return 1;
        }
    }

    OFLOG_INFO(dcmcjp2kcsLogger, "converting DICOM file to JPEG 2000 compressed transfer syntax");

    // create representation parameter
    DJ2KRepresentationParameter rp(opt_useLosslessProcess, opt_compressionRatio);
    DcmXfer opt_oxferSyn(opt_oxfer);

    // perform encoding process
    OFCondition result = dataset->chooseRepresentation(opt_oxfer, &rp);
    if (result.bad())
    {
      OFLOG_FATAL(dcmcjp2kcsLogger, result.text() << ": encoding file: " << opt_ifname);
      return 1;
    }
    if (dataset->canWriteXfer(opt_oxfer))
    {
      OFLOG_INFO(dcmcjp2kcsLogger, "output transfer syntax " << opt_oxferSyn.getXferName() << " can be written");
    } else {
      OFLOG_FATAL(dcmcjp2kcsLogger, "no conversion to transfer syntax " << opt_oxferSyn.getXferName() << " possible");
      return 1;
    }

    OFLOG_INFO(dcmcjp2kcsLogger, "creating output file " << opt_ofname);

    fileformat.loadAllDataIntoMemory();
    error = fileformat.saveFile(opt_ofname, opt_oxfer, opt_oenctype, opt_oglenc, opt_opadenc,
      OFstatic_cast(Uint32, opt_filepad), OFstatic_cast(Uint32, opt_itempad), EWM_updateMeta);

    if (error.bad())
    {
      OFLOG_FATAL(dcmcjp2kcsLogger, error.text() << ": writing file: " <<  opt_ofname);
      return 1;
    }

    OFLOG_INFO(dcmcjp2kcsLogger, "conversion successful");

    // deregister global codecs
    DJ2KEncoderRegistration::cleanup();

    return 0;
}
