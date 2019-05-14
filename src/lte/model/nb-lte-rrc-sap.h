/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Firenze (UNIFI)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Samuele Foni <samuele.foni@stud.unifi.it> (NB-IOT)
 */

#ifndef SRC_LTE_MODEL_NB_LTE_RRC_SAP_H_
#define SRC_LTE_MODEL_NB_LTE_RRC_SAP_H_

#include <list>
#include <stdint.h>
#include <string>
#include <ns3/nb-ff-mac-common.h>

namespace ns3 {

/*
 * This class goal is to define the structs needed to the NB-IOT system.
 *
 * The NB-LTE fork is the first name of the NB-IOT standard, as specified
 * by the 3GPP release 12.
 */

class NbLteRrcSap {
public:
  NbLteRrcSap();
  virtual ~NbLteRrcSap();

  // BEGIN structs for MIB-NB

  struct ChannelRasterOffsetNb{
    enum
    {
      khz_7dot5,
      khz_2dot5,
      khz2dot5,
      khz7dot5

    } cr;
  };

  static double ConvertChannelRasterOffsetNb2Double (ChannelRasterOffsetNb channelRasterOffset)
  {
    double res = 0;
    switch (channelRasterOffset.cr)
    {
      case ChannelRasterOffsetNb::khz_7dot5:
        res = -7.5;
        break;

      case ChannelRasterOffsetNb::khz_2dot5:
        res = -2.5;
        break;

      case ChannelRasterOffsetNb::khz2dot5:
        res = 2.5;
        break;

      case ChannelRasterOffsetNb::khz7dot5:
        res = 7.5;
        break;
      default:
        break;
    }
    return res;
  }

  struct InbandSamePciNb
  {
    uint8_t eutraCrsSequenceInfo;  // [ 0 to 31]
  };

  struct InbandDifferentPciNb
  {
    uint8_t eutraNumberCrsPorts;    // {Same, four}
    ChannelRasterOffsetNb rasterOffset;
    //spare bit string size 2;
  };

  struct GuardbandNb
  {
    ChannelRasterOffsetNb rasterOffset;
    //spare bit string size 3;
  };

  struct StandaloneNb
  {
    //spare bit string size 5;
  };

  struct OperationModeInfo
  {
    InbandSamePciNb inbandSamePci;
    InbandDifferentPciNb inbandDifferentPci;
    GuardbandNb guardband;
    StandaloneNb standalone;
  };

  // END structs for MIB-NB

  // BEGIN structs for SIB1

  struct CellAccessRelatedInfoR13
  {
    //PlmnIdentityListR13 plmnIdentityList;
    uint16_t trackingareacode;  		//
    uint32_t cellIdentity;  //Size 28
    bool cellBarred; 				//{barred, notBarred},
    bool intraFreqReselection; 		// {allowed, notAllowed}

  };

  struct CellSelectionInfoR13
  {
    int8_t qRxLevMin; ///< INTEGER (-70..-22), actual value = IE value * 2 [dBm].
    int8_t qQualMin; ///< INTEGER (-34..-3), actual value = IE value [dB].

  };

  struct PMax {};
  struct NsPmaxListNb {};
  struct MultiBandInfoListNb {};
  struct DlBitmapNb {};

  struct EutraControlRegionSize
  {
    enum {
      N1,
      N2,
      N3
    }ecrs; // Number of OFDM signals
  };

  struct NrsCrsPowerOffset
  {
    enum {
      dB_6,
      dB_4dot77,
      dB_3,
      dB_1dot77,
      dB0,
      dB1,
      dB1dot23,
      dB2,
      dB3,
      dB4,
      dB4dot23,
      dB5,
      dB6,
      dB7,
      dB8,
      dB9
    }crs;
  };

  static double ConvertNrsCrsPowerOffset2Double (NrsCrsPowerOffset nrsCrsPowerOffset)
  {
    double res = 0;
    switch (nrsCrsPowerOffset.crs)
    {
      case NrsCrsPowerOffset::dB_6:
        res = -6.0;
        break;

      case NrsCrsPowerOffset::dB_4dot77:
        res = -4.77;
        break;

      case NrsCrsPowerOffset::dB_3:
        res = -3.0;
        break;

      case NrsCrsPowerOffset::dB_1dot77:
        res = -1.77;
        break;

      case NrsCrsPowerOffset::dB0:
        res = 0.0;
        break;

      case NrsCrsPowerOffset::dB1:
        res = 1.0;
        break;

      case NrsCrsPowerOffset::dB1dot23:
        res = 1.23;
        break;

      case NrsCrsPowerOffset::dB2:
        res = 2.0;
        break;

      case NrsCrsPowerOffset::dB3:
        res = 3.0;
        break;

      case NrsCrsPowerOffset::dB4:
        res = 4.0;
        break;

      case NrsCrsPowerOffset::dB4dot23:
        res = 4.23;
        break;

      case NrsCrsPowerOffset::dB5:
        res = 5.0;
        break;

      case NrsCrsPowerOffset::dB6:
        res = 6.0;
        break;

      case NrsCrsPowerOffset::dB7:
        res = 7.0;
        break;

      case NrsCrsPowerOffset::dB8:
        res = 8.0;
        break;

      case NrsCrsPowerOffset::dB9:
        res = 9.0;
        break;

      default:
        break;
    }
    return res;
  }

  // Need changes
  struct SchedulingInfoList {
    uint32_t siTb;
  };

  struct SiWindowLength
  {
    enum
    {
      ms160,
      ms320,
      ms480,
      ms640,
      ms960,
      ms1280,
      ms1600,
      spare1
    } swl;
  };

  static double ConvertSiWindowLength2Double (SiWindowLength siWindowLength)
  {
    double res = 0;
    switch (siWindowLength.swl)
    {
      case SiWindowLength::ms160:
        res = 160.0;
        break;

      case SiWindowLength::ms320:
        res = 320.0;
        break;

      case SiWindowLength::ms480:
        res = 480.0;
        break;

      case SiWindowLength::ms640:
        res = 640.0;
        break;

      case SiWindowLength::ms960:
        res = 960.0;
        break;

      case SiWindowLength::ms1280:
        res = 1280.0;
        break;

      case SiWindowLength::ms1600:
        res = 1600.0;
        break;

      default:
        break;
    }
    return res;
  }

  // Need to update
  struct SystemInfoValueTagList {};
  struct LateNonCriticalExtension {};
  struct NonCriticalExtension {};

  // END structs for SIB1

  // BEGIN structs for SIB2

  enum PdcchPeriods // Number of PDCCH periods
  {
    pp1,
    pp2,
    pp3,
    pp4,
    pp5,
    pp6,
    pp7,
    pp8,
    pp10,
    pp16,
    pp32,
    pp64
  };

  struct RachInfoNb
  {
    PdcchPeriods raResponseWindowSize; // List of possibles enable values: pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp10

    PdcchPeriods macContentionResolutionTimer; // List of possibles enable values: pp1, pp2, pp3, pp4, pp8, pp16, pp32, pp64
  };

  struct NprachParametersNb
  {
    enum
    {
      MS40,
      MS80,
      MS160,
      MS240,
      MS320,
      MS640,
      MS1280,
      MS2560
    } nprachPeriodicity;

    enum
    {
      ms8,
      ms16,
      ms32,
      ms64,
      ms128,
      ms256,
      ms512,
      ms1024
    } nprachStartTime;


    enum NumberOfSubcarriers
    {
      n0,
      n2,
      n12,
      n18,
      n24,
      n34,
      n36,
      n48,
      spare1
    };

    NumberOfSubcarriers nprachSubcarrierOffset; // List of possibles enable values: n0, n12, n24, n36, n2, n18, n34, spare1

    NumberOfSubcarriers nprachNumSubcarriers; // List of possibles enable values: n12, n24, n36, n48


    enum
    {
      zero,
      oneThird,
      twoThird,
      one
    } nprachSubcarrierMsg3RangeStart;


    enum NumberOfPreambles
    {
      N1,
      N2,
      N3,
      N4,
      N5,
      N6,
      N7,
      N8,
      N10,
      N16,
      N32,
      N64,
      N128,
      SPARE1
    };


    NumberOfPreambles maxNumPreambleAttemptCe;  // Maximum number of preamble transmission attempts per NPRACH resource. See TS 36.321 [6].
                                                // List of possibles enable values: n3, n4, n5, n6, n7, n8, n10, spare1


    NumberOfPreambles numRepetitionsPerPreambleAttempt; // Number of NPRACH repetitions per attempt for each NPRACH resource, See TS 36.211 [21, 10.1.6].
                                                        // List of possibles enable values: n1, n2, n4, n8, n16, n32, n64, n128


    enum
    {
      r1,
      r2,
      r4,
      r8,
      r16,
      r32,
      r64,
      r128,
      r256,
      r512,
      r1024,
      r2048,
      spare_4,
      spare_3,
      spare_2,
      spare_1
    } npdcchNumRepetitionsRa;

    enum
    {
      v1dot5,
      v2,
      v4,
      v8,
      v16,
      v32,
      v48,
      v64
    } npdcchStartSfCssRa;

    enum
    {
      ZERO,
      ONE_EIGHTH,
      ONE_FOURTH,
      THREE_EIGHTH
    } npdcchOffsetRa;
  }; // End of struct NprachParametersNb

  struct RsrpThresholdsNprachInfoListNb
  {
    // SEQUENCE (SIZE[1..2]) OF RsrpRange (0..97)
  };

  struct NprachParametersListNb
  {
    // SEQUENCE (SIZE [1.. max NprachResourcesNb])OF NprachParametersNb
      uint16_t  nprachPeriodicity;      // in ms: [40,80,160,240,320,640,1280,2560]
      uint16_t  nprachStartTime;        // in ms: [8,16,32,64,128,256,512,1024]
      uint16_t  nprachSubcarrierOffset; // in n: [0,2, 12,18,24,34,36]
      uint16_t  nprachNumSubcarriers;   // in n: [12,24,36,48]
      float     nprachSubcarrierMSG3RangeStart;     // in n: [0, 1/3, 2/3, 1]
      uint8_t   maxNumPreambleAttemptCE;            // in n: [3,4,5,6,7,8,10]
      uint8_t   numRepetitionsPerPreambleAttempt;   // in n: [1,2,4,8,16,32,64,128]
      uint16_t  npdcchNumRepetitionsRA;             // in r: [1,2,4,8,16,32,64,128,256,512,1024,2048]
      float     npdcchStartSFCSSRA;                 // in v: [1.5, 2, 4,8,16,32,48,64]
      float     npdcchOffsetRA;                     // [0, 1/8, 1/4, 3/8]
  };

  struct NprachConfigSibNb
  {
    enum
    {
      us66dot7,
      us266dot7
    } nprachCpLength;
    RsrpThresholdsNprachInfoListNb rsrpThresholdsPrachInfoList;
    NprachParametersListNb nprachParametersList;
  };

  struct NpdschConfigCommonNb
  {
      int16_t   nrspower;   // [-60..50]
  };

  struct DLGapConfigNb
  {
      uint16_t    GapThreshold;     // 32, 64, 128, 256
      uint16_t    GapPeriodicity;   // 64, 128, 256, 512
      float       GapDurationCoeff; //1/8, 1/4, 3/8, 1/2

  };
  struct PowerRampingParameters {
    enum
    {
      dB0,
      dB2,
      dB4,
      dB6
    } powerRampingStep;

    enum
    {
      dBm_120,
      dBm_118,
      dBm_116,
      dBm_114,
      dBm_112,
      dBm_110,
      dBm_108,
      dBm_106,
      dBm_104,
      dBm_102,
      dBm_100,
      dBm_98,
      dBm_96,
      dBm_94,
      dBm_92,
      dBm_90
    } preambleInitialReceivedTargetPower;
  };

  struct RachInfoListNb
  {
      uint8_t raResponseWindowSize;    // In pp: [2,3,4,5,6,7,8,10]
      uint16_t  macContentionResolutionTimer;   // In PP: [1,2,3,4,8,16,32,64]
  };
  struct RachConfigCommonNb
  {
    uint16_t PreambleTransMax;                  // [3,4,5,6,7,8,10,20,50,100,200]
    //RaSupervisionInfo raSupervisionInfo;
    PowerRampingParameters powerRampingParameters;
    RachInfoListNb rachInfoListNb; // SEQUENCE [SIZE (1 .. max NprachResourcesNb)] OF RachInfoNb
    uint8_t connEstFailOffset; // --Need OP. [0 to 15]

  };

  struct BcchConfigNb
  {
      uint16_t    modificationPeriodCoeff;      // [16,32,64,128]
  };
  struct PcchConfigNb
  {
      uint16_t    defaultPagingCycle;           // [128, 256, 512, 1024]
      float       nb;                           // In T: [ 4, 2, 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128, 1/256, 1/512, 1/1024]
      uint16_t    npdcchNumRepetitionPaging;    // In r: [1,2,4,8,16,32,128,256,512,1024,2048]
  };

  struct PdschConfigCommon
  {
    int8_t referenceSignalPower;  // INTEGER (-60..50),
    int8_t pb;                    // INTEGER (0..3),
  };

  struct RadioResourceConfigCommonSibNb
  {
    RachConfigCommonNb  rachConfigCommonNb;
    BcchConfigNb        bcchConfig;
    PcchConfigNb        pcchConfig;
    NprachConfigSibNb   nprachConfig;
    NpdschConfigCommonNb    npdschConfigCommon;
    //npusch-ConfigCommon-r13                 NPUSCH-ConfigCommon-NB-r13,
    DLGapConfigNb       dlGap;
    //uplinkPowerControlCommon-r13            UplinkPowerControlCommon-NB-r13,
  };

  struct UeTimersAndConstantsNb {};

  struct FreqInfo
  {
    uint16_t ulCarrierFreqNb;
    uint8_t ulBandwidth; // Not needed in this context.
    uint8_t additionalSpectrumEmission; // May be unused...
  };

  struct TimeAlignmentTimer {};
  struct MultiBandInfoList {};

  // END structs for SIB2

  /*
   * MasterInformationBlock-NB also known as MIB-NB carries relatively large set of information on an LTE-NB environment,
   * while in LTE its structure is very simple. This feature represent one of the best differences between LTE-NB and
   * legacy LTE.
   */


  struct MasterInformationBlockNb
  {
    uint8_t systemFrameNumberMsb;  // size 4
    uint8_t hyperSfnLsb;
    uint8_t schedulingInfoSib1;    // [0 to 15]
    uint8_t systemInfoValueTag;    // [0 to 31]
    bool abEnabled;
    OperationModeInfo operationMode;
    //spare bit string size 11;
  };

  struct SystemInformationBlockType1Nb
  {
    uint8_t	                    hyperSfnMsbR13;  							//size 8
    //bool enable_edrx; 									                    // eDRX is enable or disable
    CellAccessRelatedInfoR13    cellAccessRelatedInfo;
    CellSelectionInfoR13        cellSelectionInfo;
    int8_t 				        pMaxR13; 					                // [-30, 33] Need OP  -- not used in this context
    int8_t                      freqbandIndicator;                           // FBI = [1...256] -- not used in this context

    NsPmaxListNb                freqBandInfo;  // -- Need OR
    MultiBandInfoListNb         multiBandInfoList;  // -- Need OR
    DlBitmapNb                  downlinkBitmap;  //  -- Need OP
    EutraControlRegionSize      eutraControlRegionSize;                     // Optional. This value is applied only to in-band Operation mode.
                                                                            //It indicates how many OFDM symbols are used for control region.
    NrsCrsPowerOffset           nrsCrsPowerOffset;                          // Optional. This value is applied only to in-band Operation mode (Same PCI).
    SchedulingInfoList          schedulingInfoListNb;
    SiWindowLength              siWindowLength;
    uint8_t                     siRadioFrameOffset;                         // [1,15] Optional
    SystemInfoValueTagList      systemInfoValueTagList;  // -- Need OR
    LateNonCriticalExtension    lateNonCriticalExtension;                   // Optional. Not needed.
    NonCriticalExtension        nonCriticalExtension;                       // Optional. Not needed.
  };

  struct SystemInformationBlockType2Nb
  {
    RadioResourceConfigCommonSibNb radioResourceConfigCommonSibNb;
    UeTimersAndConstantsNb ueTimersAndConstantsNb;
    FreqInfo freqInfo;
    TimeAlignmentTimer timeAlignmentTimer;
    MultiBandInfoList multiBandInfoList;
    LateNonCriticalExtension lateNonCriticalExtension;
  };


  /// SystemInformation structure
   struct SystemInformationNb
   {
     bool haveSib2; ///< have SIB2?
     SystemInformationBlockType2Nb sib2; ///< SIB2
   };





struct UlSCHConfignb
{
    uint16_t periodicBSRTimer;    // In SF: [5,10, 16,20,32,40,64,80,128,160,320,640,1280,2560,infinite]
    uint32_t retxBSRTimer;       // In SF: [320,640,1280,2560,5120,10240]
};



struct MACMainConfigNb
{
    UlSCHConfignb   ulSCHConfig;                    // Optional
    DrxConfigNB_s   drxConfig;                      // Optional
    uint32_t        timeAlignmentTimerDedicated;    // [500,750,1280,1920,2560,5120,10240,infinite]
    uint16_t        logicalChannelSRProhibitTimer; // Optional in PP: [2,8,32,128,512,1024,2048]
    uint16_t        dataInactivityTimer;            // Optional in s: [1,2,3,5,7,10,15,20,40,50,60,80,100,120,150,180]
};










/// PhysicalConfigDedicated structure
struct NpdcchConfigDedicatedNb
{
    uint16_t npdcchNumRepetitions;  // [1,2,3,8,16,32,64,128,256,512,1024,2048]
    uint16_t npdcchStartSFUSS;      // [1.5,2,4,8,16,32,48,64]
    float    npdcchOffsetUSS;       // [0,1/8,1/4,3/8]
};

struct NpuschConfigDedicatedNb
{
    uint16_t acknackNumRepetitions;  // [1,2,4,8,16,32,64,128]
    bool npuschAllSymbols;
    bool groupHoppingDisabled;

};

struct PhysicalConfigDedicated
{
    //bool havecarrierConfigDedicated;
    //CarrierConfigDedicatedNb carrierConfigDedicated;

    bool haveNpcchConfigDedicated;
    NpdcchConfigDedicatedNb npdcchConfigDedicated;

    bool haveNuschConfigDedicated;
    NpuschConfigDedicatedNb npuschConfigDedicated;

    int8_t p0UENPUSCH; ///< [-8 to 7]
};

/// DrbToAddMod structure
/// RlcConfig structure
struct RlcConfig
{
  /// the direction choice
  enum direction
  {
    AM,
    UM_BI_DIRECTIONAL,
    UM_UNI_DIRECTIONAL_UL,
    UM_UNI_DIRECTIONAL_DL
  } choice; ///< direction choice
};
/// LogicalChannelConfig structure
struct LogicalChannelConfigNb
{
  uint8_t priority; ///< priority [1 to 16]
  bool logicalChannelSRProhibit;
};

struct DrbToAddModNb
{
  uint8_t epsBearerIdentity; ///< EPS bearer identity
  uint8_t drbIdentity; ///< DRB identity
  RlcConfig rlcConfig; ///< RLC config
  uint8_t logicalChannelIdentity; ///< logical channel identify [3 to 10]
  LogicalChannelConfigNb logicalChannelConfig; ///< logical channel config
};


/// SrbToAddMod structure
 struct SrbToAddModNb
 {
   uint8_t srbIdentity; ///< SB identity
   LogicalChannelConfigNb logicalChannelConfig; ///< logical channel config
 };

/// RadioResourceConfigDedicated structure
struct RadioResourceConfigDedicatedNb
{
  SrbToAddModNb srbToAddModList; ///< SRB to add mod list
  std::list<DrbToAddModNb> drbToAddModList; ///< DRB to add mod list
  std::list<uint8_t> drbToReleaseList; ///< DRB to release list
  bool havePhysicalConfigDedicated; ///< have physical config dedicated?
  PhysicalConfigDedicated physicalConfigDedicated; ///< physical config dedicated
  bool haveMACConfig; ///< have MAC config ?
  MACMainConfigNb   macconfig;
};

/// RrcConnectionSetup structure
  struct RrcConnectionSetupNb
  {
    uint8_t rrcTransactionIdentifier; ///< RRC transaction identifier
    RadioResourceConfigDedicatedNb radioResourceConfigDedicated; ///< radio resource config dedicated
  };

  /// RrcConnectionRequest structure
  struct RrcConnectionRequestNb
  {
    uint64_t ueIdentity; ///< UE identity
    bool ismultiToneSupport;
    bool ismultiCarrierSupport;
  };

  /// AsConfig structure
   struct AsConfigNb
   {
     RadioResourceConfigDedicatedNb sourceRadioResourceConfig; ///< source radio resource config
     uint16_t sourceUeIdentity; ///< source UE identity
     uint32_t sourceDlCarrierFreq; ///< source DL carrier frequency - max 262143
   };

   /// RrcConnectionReconfiguration structure
   struct RrcConnectionReconfigurationNb
   {
       uint8_t rrcTransactionIdentifier; ///< RRC transaction identifier (0...3)
       bool haveRadioResourceConfigDedicated; ///< have radio resource config dedicated
       RadioResourceConfigDedicatedNb radioResourceConfigDedicated; ///< radio resource config dedicated
   };

   /// RrcConnectionReestablishment structure
   struct RrcConnectionReestablishmentNb
   {
     uint8_t rrcTransactionIdentifier; ///< RRC transaction identifier
     RadioResourceConfigDedicatedNb radioResourceConfigDedicated; ///< radio resource config dedicated
     uint8_t nextHopChainingCount;  // 0 to 7
   };

private:
   void SendSystemInformationNb();


}; // Class end

} /* namespace ns3 */

#endif /* SRC_LTE_MODEL_NB_LTE_RRC_SAP_H_ */
