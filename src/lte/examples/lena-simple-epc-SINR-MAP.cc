/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
//#include "ns3/flow-monitor-module.h"
//#include "ns3/gtk-config-store.h"



#include <ns3/config-store-module.h>
#include <ns3/buildings-module.h>
#include <ns3/point-to-point-helper.h>



#include "ns3/spectrum-module.h"
#include <ns3/buildings-helper.h>
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/log.h"

#include "ns3/netanim-module.h"

#include <iomanip>
#include <ios>
#include <string>
#include <vector>

using namespace ns3;

//#define FLOW_MONITOR 1



static ns3::GlobalValue g_srsPeriodicity ("srsPeriodicity",
                                          "SRS Periodicity (has to be at least "
                                          "greater than the number of UEs per eNB)",
                                          ns3::UintegerValue (640),
                                          ns3::MakeUintegerChecker<uint16_t> ());

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");



void
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" /*<< uedev->GetImsi ()*/
                      << "\" at " << pos.x << "," << 1*pos.y << " left font \"Helvetica,4\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"

                      << std::endl;
            }
        }
    }
}


void 
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" /*<< enbdev->GetCellId ()*/
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

void 
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}


#ifdef FLOW_MONITOR
void throughput(Ptr<FlowMonitor> monitor)
{        
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
       // std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
       // std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout<<"Num clients = " << nSta << " "
                 << "  Throughput: " << i->second.rxBytes * 8.0 / 10.0 / 1024 / 1024  << " Mbps\n";
  }
  
  //monitor->SerializeToXmlFile ("results.xml",false,true);
  //monitor->SerializeToXmlFile ("result.dat",false,true);
  Simulator::Schedule(Seconds(1.0), &throughput, monitor);
}
#endif

int
main (int argc, char *argv[])
{

  uint16_t numberOfEnbNodes = 7;
  uint16_t numberOfUeNodes = 300;
  double simTime = 15.0;
  double appTrafficStartTime = 1.000;
  double appTrafficStopTime = 200.0;
  double distance = 1000.0;
  bool useCa = false;
  double nodeHeight = 1.5;
  int maxPackets = 2;
  int sectorNum = 3;
  double ueDistance = 7;
  uint32_t buildingWidth = 100;

  
  //uint32_t resourceId1;


  uint8_t DlBandwidth = 18;//25;
  uint8_t UlBandwidth = 18;//25;
  UintegerValue uintegerValue;
  double interPacketInterval = 20000000;//33000//20,000,000;//900,000,000



  GlobalValue::GetValueByName ("srsPeriodicity", uintegerValue);
  uint16_t srsPeriodicity = uintegerValue.Get ();
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (srsPeriodicity));
  double eNbTxPower = 46;//20;
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (eNbTxPower));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (20.0));

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfEnbNodes", "Number of eNodeBs", numberOfEnbNodes);
  cmd.AddValue("numberOfUeNodes", "Number of UE", numberOfUeNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [us])", interPacketInterval);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue("appTrafficStartTime", "time to generate aplication traffics [s].", appTrafficStartTime);
  cmd.AddValue("maxPackets", "number of app traffics [s].", maxPackets);
  cmd.AddValue("ueDistance", "distance between UEs [meter]", ueDistance);
  cmd.AddValue("buildingWidth", "distance between UEs [meter]", buildingWidth);
  
  cmd.Parse(argc, argv);

  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
   }


  double max = interPacketInterval;
  double min = interPacketInterval - 10000000;
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (min));
  x->SetAttribute ("Max", DoubleValue (max));

  //Box macroUeBox = Box (- distance * 1.5, distance * 1.5, - distance * 1.5, distance * 1.5, nodeHeight, nodeHeight);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  double dlEarfcn = 3450;
  double ulEarfcn = 21450;
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (dlEarfcn/*100*/));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (ulEarfcn/*100 + 18000*/));
  //lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (dlEarfcn/*3450*/));
  //lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (ulEarfcn/*21450*/));
  //lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (6525));
  //lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (24525));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (DlBandwidth));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (UlBandwidth));
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");



  // Create the Propagation Loss Model
  //lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));
  /*lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::HybridBuildingsPropagationLossModel"));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaExtWalls", DoubleValue (0));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaOutdoor", DoubleValue (1));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaIndoor", DoubleValue (1.5));
  // use always LOS model
  lteHelper->SetPathlossModelAttribute ("Los2NlosThr", DoubleValue (1e6));
  lteHelper->SetSpectrumChannelType ("ns3::MultiModelSpectrumChannel");*/
  /*
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::OkumuraHataPropagationLossModel"));
  lteHelper->SetPathlossModelAttribute("Environment", EnumValue (OpenAreasEnvironment)); //OpenAreasEnvironment//UrbanEnvironment//SubUrbanEnvironment
  lteHelper->SetPathlossModelAttribute("CitySize", EnumValue (MediumCity));
  */

  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::HybridBuildingsPropagationLossModel"));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaExtWalls", DoubleValue (0));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaOutdoor", DoubleValue (1));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaIndoor", DoubleValue (1.5));
  // use always LOS model
  lteHelper->SetPathlossModelAttribute ("Los2NlosThr", DoubleValue (1e6));
  lteHelper->SetSpectrumChannelType ("ns3::MultiModelSpectrumChannel");




  //======================Fading=========================================
  /*lteHelper->SetAttribute ("FadingModel", StringValue ("ns3::TraceFadingLossModel"));
  
  std::ifstream ifTraceFile;
  ifTraceFile.open ("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad", std::ifstream::in);
  if (ifTraceFile.good ())
    {
      // script launched by test.py
      lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }
  else
    {
      // script launched as an example
      lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }
    
  // these parameters have to setted only in case of the trace format 
  // differs from the standard one, that is
  // - 10 seconds length trace
  // - 10,000 samples
  // - 0.5 seconds for window size
  // - 100 RB
  lteHelper->SetFadingModelAttribute ("TraceLength", TimeValue (Seconds (10.0)));
  lteHelper->SetFadingModelAttribute ("SamplesNum", UintegerValue (10000));
  lteHelper->SetFadingModelAttribute ("WindowSize", TimeValue (Seconds (0.5)));
  lteHelper->SetFadingModelAttribute ("RbNum", UintegerValue (100));*/
  //===========================================================================

  // Geometry of the scenario (in meters)
  // Assume squared building
  
  uint32_t nEnbPerFloor = 1;
  double roomHeight = 3;
  //double roomLength = 8;
  uint32_t nFloors = 5;
  uint32_t buildingOffset = 200;
  uint32_t nRooms = std::ceil (std::sqrt (nEnbPerFloor));
  Ptr<Building> building;
  for(int i = 1; i < 8; i++)
  {
      building = CreateObject<Building> ();
      building->SetBoundaries (Box (0 + i*buildingOffset, 0 + i*buildingOffset + buildingWidth,
                                    -200, 100 + 200,
                                    0.0, nFloors* roomHeight));
      building->SetBuildingType (Building::Residential);
      building->SetExtWallsType (Building::ConcreteWithoutWindows);//ConcreteWithWindows//StoneBlocks//ConcreteWithoutWindows
      building->SetNFloors (nFloors);
      building->SetNRoomsX (nRooms);
      building->SetNRoomsY (nRooms);
  }

  for(int i = 1; i < 8; i++)
  {
      building = CreateObject<Building> ();
      building->SetBoundaries (Box (3000 + i*buildingOffset, 3000 + i*buildingOffset + 180,
                                    -200, 100 + 200,
                                    0.0, nFloors* roomHeight));
      building->SetBuildingType (Building::Residential);
      building->SetExtWallsType (Building::ConcreteWithoutWindows);
      building->SetNFloors (nFloors);
      building->SetNRoomsX (nRooms);
      building->SetNRoomsY (nRooms);
  }

  for(int i = 1; i < 8; i++)
  {
      building = CreateObject<Building> ();
      building->SetBoundaries (Box ((int)(-3000 + i*buildingOffset), (int)(-3000 + i*buildingOffset + 25),
                                    -200, 100 + 200,
                                    0.0, nFloors* roomHeight));
      building->SetBuildingType (Building::Residential);
      building->SetExtWallsType (Building::ConcreteWithoutWindows);
      building->SetNFloors (nFloors);
      building->SetNRoomsX (nRooms);
      building->SetNRoomsY (nRooms);
  }

  for(int i = 1; i < 8; i++)
  {
    building = CreateObject<Building> ();
    building->SetBoundaries (Box (1500 + i*buildingOffset, 1500 + i*buildingOffset + 100,
                                  3000 - 200, 3000 + 100 + 200,
                                  0.0, nFloors* roomHeight));
    building->SetBuildingType (Building::Residential);
    building->SetExtWallsType (Building::ConcreteWithoutWindows);
    building->SetNFloors (nFloors);
    building->SetNRoomsX (nRooms);
    building->SetNRoomsY (nRooms);
  }

  for(int i = 1; i < 8; i++)
  {
    building = CreateObject<Building> ();
    building->SetBoundaries (Box ((int)(-1500 + i*buildingOffset), (int)(-1500 + i*buildingOffset + 100),
                                  3000 - 200, 3000 + 100 + 200,
                                  0.0, nFloors* roomHeight));
    building->SetBuildingType (Building::Residential);
    building->SetExtWallsType (Building::ConcreteWithoutWindows);
    building->SetNFloors (nFloors);
    building->SetNRoomsX (nRooms);
    building->SetNRoomsY (nRooms);
  }

  for(int i = 1; i < 8; i++)
  {
    building = CreateObject<Building> ();
    building->SetBoundaries (Box ((int)(-1500 + i*buildingOffset), (int)(-1500 + i*buildingOffset + 50),
                                  (int)(-3000 - 200), (int)(-3000 + 100 + 200),
                                  0.0, nFloors* roomHeight));
    building->SetBuildingType (Building::Residential);
    building->SetExtWallsType (Building::ConcreteWithoutWindows);
    building->SetNFloors (nFloors);
    building->SetNRoomsX (nRooms);
    building->SetNRoomsY (nRooms);
  }

  for(int i = 1; i < 8; i++)
  {
    building = CreateObject<Building> ();
    building->SetBoundaries (Box (1500 + i*buildingOffset, 1500 + i*buildingOffset + 75,
                                  (int)(-3000 - 200), (int)(-3000 + 100 + 200),
                                  0.0, nFloors* roomHeight));
    building->SetBuildingType (Building::Residential);
    building->SetExtWallsType (Building::ConcreteWithoutWindows);
    building->SetNFloors (nFloors);
    building->SetNRoomsX (nRooms);
    building->SetNRoomsY (nRooms);
  }
  //===========================================================================

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  //double interSiteDistance = 5000;
  //uint32_t nMacroEnbSitesX = 2;
  //uint32_t nMacroEnbSites = 7;
  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(sectorNum * numberOfEnbNodes);

  ueNodes.Create(numberOfUeNodes);
  


  // Install Mobility Model
  double enbX[7][3] = {{1.5, -1.5, 0},  {3001.5, 2998.5, 3000.0}, {1501.5, 1498.5, 1500.0},   {-2998.5, -3001.5, -3000.0},  {-1498.5, -1501.5, -1500.0},  {1501.5, 1498.5, 1500.0},       {-1498.5, -1501.5, -1500.0}};
  double enbY[7][3] = {{0, 0, -2.59},      {0, 0, -2.59},               {3000.0, 3000.0, 2998.0},   {0, 0, -2.59},                   {3000.0, 3000.0, 2998.0},     {-3000.0, -3000.0, -3002.59},    {-3000.0, -3000.0, -3002.59}};

  //double enbX[7][1] = {{1.5, -1.5, 0},  {3001.5, 2998.5, 3000.0}, {1501.5, 1498.5, 1500.0},   {-2998.5, -3001.5, -3000.0},  {-1498.5, -1501.5, -1500.0},  {1501.5, 1498.5, 1500.0},       {-1498.5, -1501.5, -1500.0}};
  //double enbY[7][3] = {{0, 0, -2.59},      {0, 0, -2},               {3000.0, 3000.0, 2998.0},   {0, 0, -2},                   {3000.0, 3000.0, 2998.0},     {-3000.0, -3000.0, -3002.0},    {-3000.0, -3000.0, -3002.0}};


  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbNodes; i++)
    {
      for(uint16_t j = 0; j < sectorNum; j++)
       {
        positionAlloc->Add (Vector(enbX[i][j], enbY[i][j], 2.5));
        std::cout<<"nodeHeight = " << nodeHeight << " \n";
        Vector position = positionAlloc->GetNext ();
        std::cout<<"position.x = " <<  position.x << " y = " <<  position.y << " z = " <<  position.z <<" \n";
      }
    }
  //std::cout<<"Num positionAlloc = " << positionAlloc.GetN() << " \n";
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  BuildingsHelper::Install (enbNodes);



  /*
  double shift = 5;
    //double enbDistance = 1500;
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t n = 0; n < numberOfEnbNodes; n++)
  {
    for (int i = 0; i < (numberOfUeNodes/(sectorNum*numberOfEnbNodes)); i++)
      uePositionAlloc->Add (Vector ((enbX[n][0] + i*ueDistance+shift), enbY[n][0] + 0, nodeHeight));  // edgeUE1-100
    for (int i = 0; i < (numberOfUeNodes/(sectorNum*numberOfEnbNodes)); i++)
      uePositionAlloc->Add (Vector ((enbX[n][0] - i*ueDistance/2-shift), enbY[n][0] + 1.732*i*ueDistance/2+shift, nodeHeight));  // edgeUE1-100
    for (int i = 0; i < (numberOfUeNodes/(sectorNum*numberOfEnbNodes)); i++)
      uePositionAlloc->Add (Vector ((enbX[n][0] - i*ueDistance/2-shift), enbY[n][0] - 1.732*i*ueDistance/2-shift, nodeHeight));  // edgeUE1-100
  }
  mobility.SetPositionAllocator (uePositionAlloc);
  */


  // Beam width is made quite narrow so sectors can be noticed in the REM
  ///*
  NetDeviceContainer enbLteDevs;
  int cellTypeCounter = 0;
  int eNBCounter = 0;
  int orientation[3] = {120, 240, 0};
  lteHelper->SetFfrAlgorithmType ("ns3::LteFrHardAlgorithm");//LteFrNoOpAlgorithm//LteFrHardAlgorithm//LteFrStrictAlgorithm;
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (eNbTxPower));
  for (uint32_t i = 0; i < numberOfEnbNodes; i++)
  {
    for(uint16_t j = 0; j < sectorNum; j++)
      {
          //lteHelper->SetFfrAlgorithmAttribute ("DlSubBandOffset", UintegerValue (j*32));
          //lteHelper->SetFfrAlgorithmAttribute ("DlSubBandwidth", UintegerValue (32));
          //lteHelper->SetFfrAlgorithmAttribute ("UlSubBandOffset", UintegerValue (j*32));
          //lteHelper->SetFfrAlgorithmAttribute ("UlSubBandwidth", UintegerValue (32));
          Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (eNbTxPower));
          lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");   //CosineAntennaModel//ParabolicAntennaModel
          lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (orientation[j]));
          lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (60));
          //lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
          lteHelper->SetFfrAlgorithmAttribute ("FrCellTypeId", UintegerValue (j + 1));
          lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (DlBandwidth));
          lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (UlBandwidth));
          enbLteDevs.Add (lteHelper->InstallEnbDevice (enbNodes.Get (eNBCounter++)));
          std::cout<<"Add = " << cellTypeCounter << " ";
      }
  }


  /*mobility.Install (macroEnbs);
  BuildingsHelper::Install (macroEnbs);
  Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
  lteHexGridEnbTopologyHelper->SetLteHelper (lteHelper);
  lteHexGridEnbTopologyHelper->SetAttribute ("InterSiteDistance", DoubleValue (interSiteDistance));
  lteHexGridEnbTopologyHelper->SetAttribute ("MinX", DoubleValue (interSiteDistance/2));
  lteHexGridEnbTopologyHelper->SetAttribute ("GridWidth", UintegerValue (nMacroEnbSitesX));
  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (70));
  //lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (dlEarfcn));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (ulEarfcn));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (DlBandwidth));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (UlBandwidth));
  NetDeviceContainer macroEnbDevs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice (macroEnbs);*/

    

    /*lteHelper->SetFfrAlgorithmAttribute ("DlSubBandOffset", UintegerValue (12));
    lteHelper->SetFfrAlgorithmAttribute ("DlSubBandwidth", UintegerValue (12));
    lteHelper->SetFfrAlgorithmAttribute ("UlSubBandOffset", UintegerValue (25));
    lteHelper->SetFfrAlgorithmAttribute ("UlSubBandwidth", UintegerValue (25));
    Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (eNbTxPower));
    lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");  //CosineAntennaModel
    lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (2*360/3));
    lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (60));
    //lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
    lteHelper->SetFfrAlgorithmAttribute ("FrCellTypeId", UintegerValue (2));
    enbLteDevs.Add (lteHelper->InstallEnbDevice (enbNodes.Get (i*sectorNum+1)));
    std::cout<<"Add = " << i*sectorNum+1 << " ";



    lteHelper->SetFfrAlgorithmAttribute ("DlSubBandOffset", UintegerValue (24));
    lteHelper->SetFfrAlgorithmAttribute ("DlSubBandwidth", UintegerValue (12));
    lteHelper->SetFfrAlgorithmAttribute ("UlSubBandOffset", UintegerValue (50));
    lteHelper->SetFfrAlgorithmAttribute ("UlSubBandwidth", UintegerValue (25));
    Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (eNbTxPower));
    lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");  //CosineAntennaModel
    lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (0));
    lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (60));
    //lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
    lteHelper->SetFfrAlgorithmAttribute ("FrCellTypeId", UintegerValue (3));
    enbLteDevs.Add (lteHelper->InstallEnbDevice (enbNodes.Get (i*sectorNum+2)));
    std::cout<<"Add = " << i*sectorNum+2 << " \n";
  }*/


  ///*
  double distanceEnb = 3000.0;
  Box macroUeBox = Box (-distanceEnb * 1.5, distanceEnb * 1.5, -distanceEnb * 1.5, distanceEnb * 1.5, 1.5, 1.5);
  Ptr<RandomBoxPositionAllocator> randomUePositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
  Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
  xVal->SetAttribute ("Min", DoubleValue (macroUeBox.xMin));
  xVal->SetAttribute ("Max", DoubleValue (macroUeBox.xMax));
  randomUePositionAlloc->SetAttribute ("X", PointerValue (xVal));
  Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
  yVal->SetAttribute ("Min", DoubleValue (macroUeBox.yMin));
  yVal->SetAttribute ("Max", DoubleValue (macroUeBox.yMax));
  randomUePositionAlloc->SetAttribute ("Y", PointerValue (yVal));
  Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
  zVal->SetAttribute ("Min", DoubleValue (macroUeBox.zMin));
  zVal->SetAttribute ("Max", DoubleValue (macroUeBox.zMax));
  randomUePositionAlloc->SetAttribute ("Z", PointerValue (zVal));
  mobility.SetPositionAllocator (randomUePositionAlloc);


/*
  for (uint16_t i = 0; i < numberOfUeNodes; i++)
  {
    Vector position = randomUePositionAlloc->GetNext ();
    std::cout<<"position.x = " <<  position.x << " y = " <<  position.y << " z = " <<  position.z <<" \n";
  }*/


  
  /*Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable> ();
  posX->SetAttribute ("Min", DoubleValue (macroUeBox.xMin));
  posX->SetAttribute ("Max", DoubleValue (macroUeBox.xMax));
  Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable> ();
  posY->SetAttribute ("Min", DoubleValue (macroUeBox.yMin));
  posY->SetAttribute ("Max", DoubleValue (macroUeBox.yMax));
  positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint32_t j = 0; j < numberOfUeNodes; j++)
  {
    positionAlloc->Add (Vector (posX->GetValue (), posY->GetValue (), nodeHeight));
    mobility.SetPositionAllocator (positionAlloc);
  }*/

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  //mobility.SetMobilityModel("ns3::BuildingsMobilityModel"); 
  mobility.Install (ueNodes);
  BuildingsHelper::Install (ueNodes);
  BuildingsHelper::MakeMobilityModelConsistent ();

  // Install LTE Devices to the nodes
  //NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);




  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  /*for (uint16_t i = 0; i < numberOfEnbNodes; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
      }*/
  //lteHelper->Attach (ueLteDevs, enbLteDevs.Get (0));
  lteHelper->AttachToClosestEnb (ueLteDevs, enbLteDevs);


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (/*Seconds*/MicroSeconds(x->GetValue ())));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
      ulClient.SetAttribute ("PacketSize", UintegerValue (128));//12

      clientApps.Add (ulClient.Install (ueNodes.Get(u)));

    }
  serverApps.Start (Seconds (appTrafficStartTime));
  clientApps.Start (Seconds (appTrafficStartTime));
  serverApps.Stop (Seconds (appTrafficStopTime));
  clientApps.Stop (Seconds (appTrafficStopTime));

  //=============================================================================================
  /*OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address("10.1.1.2"), 9)));
  onoff.SetConstantRate (DataRate ("10kb/s"));
  onoff.SetAttribute ("PacketSize", UintegerValue (200));
  onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute ("DataRate", StringValue ("3000000bps"));
  onoff.SetAttribute ("StartTime", TimeValue (Seconds (1.000000)));
  ApplicationContainer apps = onoff.Install (ueNodes.Get(0));
  apps.Start (Seconds (3.0));
  apps.Stop (Seconds (10.0));*/
  //=============================================================================================
  #ifdef FLOW_MONITOR
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  monitor = flowmon.GetMonitor();
  #endif
  //============================================================================================= 

  PrintGnuplottableUeListToFile ("ues.txt");
  PrintGnuplottableEnbListToFile ("enbs.txt");
  PrintGnuplottableBuildingListToFile ("buildings.txt");


  Simulator::Stop(Seconds(simTime));

  // Configure Radio Environment Map (REM) output
  // for LTE-only simulations always use /ChannelList/0 which is the downlink channel

  ///*
  double distanceRem = 3000.0;
  Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
  remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/1"));
  remHelper->SetAttribute ("OutputFile", StringValue ("lena-simple-epc.rem"));
  remHelper->SetAttribute ("XMin", DoubleValue (-1.5*distanceRem));
  remHelper->SetAttribute ("XMax", DoubleValue (1.5*distanceRem));
  remHelper->SetAttribute ("XRes", UintegerValue (1000));
  remHelper->SetAttribute ("YMin", DoubleValue (-1.5*distanceRem));
  remHelper->SetAttribute ("YMax", DoubleValue (1.5*distanceRem));
  remHelper->SetAttribute ("Z", DoubleValue (1.0));
  //remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));
  remHelper->SetAttribute ("Earfcn", UintegerValue (dlEarfcn));
  remHelper->Install ();
  //*/


  //lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");


    //Spectrum analyzer
  NodeContainer spectrumAnalyzerNodes;
  spectrumAnalyzerNodes.Create (1);
  SpectrumAnalyzerHelper spectrumAnalyzerHelper;

  if (false/*generateSpectrumTrace*/)
    {
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
      //position of Spectrum Analyzer
//    positionAlloc->Add (Vector (0.0, 0.0, 0.0));                              // eNB1
//    positionAlloc->Add (Vector (distance,  0.0, 0.0));                        // eNB2
      //positionAlloc->Add (Vector (distance * 0.5, distance * 0.866, 0.0));          // eNB3
      positionAlloc->Add (Vector(0, 0, 2.5));

      MobilityHelper mobility;
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.SetPositionAllocator (positionAlloc);
      mobility.Install (spectrumAnalyzerNodes);

      //Ptr<LteSpectrumPhy> enbDlSpectrumPhy = enbLteDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetPhy ()->GetDownlinkSpectrumPhy ()->GetObject<LteSpectrumPhy> ();
      Ptr<LteSpectrumPhy> enbUlSpectrumPhy = enbLteDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetPhy ()->GetUplinkSpectrumPhy ()->GetObject<LteSpectrumPhy> ();
      //Ptr<SpectrumChannel> dlChannel = enbDlSpectrumPhy->GetChannel ();
      Ptr<SpectrumChannel> ulChannel = enbUlSpectrumPhy->GetChannel ();

      spectrumAnalyzerHelper.SetChannel (ulChannel);
      //spectrumAnalyzerHelper.SetRxSpectrumModel (SpectrumModelIsm2400MhzRes1Mhz);
      Ptr<SpectrumModel> sm = LteSpectrumValueHelper::GetSpectrumModel (ulEarfcn, 100);
      spectrumAnalyzerHelper.SetRxSpectrumModel (sm);
      spectrumAnalyzerHelper.SetPhyAttribute ("Resolution", TimeValue (MicroSeconds (1000)));
      spectrumAnalyzerHelper.SetPhyAttribute ("NoisePowerSpectralDensity", DoubleValue (1e-15));     // -120 dBm/Hz
      spectrumAnalyzerHelper.EnableAsciiAll ("spectrum-analyzer-output");
      spectrumAnalyzerHelper.Install (spectrumAnalyzerNodes);


    /*
      you can get a nice plot of the output of SpectrumAnalyzer with this gnuplot script:

      unset surface
      set pm3d at s 
      set palette
      set key off
      set view 50,50
      set xlabel "time (ms)"
      set ylabel "freq (MHz)"
      set zlabel "PSD (dBW/Hz)" offset 15,0,0
      splot "./spectrum-analyzer-output-3-0.tr" using ($1*1000.0):($2/1e6):(10*log10($3))
    */
    }

  
  /*AnimationInterface anim ("animation.xml");
  int32_t resourceId0 = anim.AddResource ("/home/osboxes/workspace/netAnimIDEALDemo/netanim/internet.png");
  int32_t resourceId1 = anim.AddResource ("/home/osboxes/workspace/netAnimIDEALDemo/netanim/SGW.png");
  int32_t resourceId2 = anim.AddResource ("/home/osboxes/workspace/netAnimIDEALDemo/netanim/enb.png");
  int32_t resourceId3 = anim.AddResource ("/home/osboxes/workspace/netAnimIDEALDemo/netanim/Sensor.png");
  //resourceId2 = anim->AddResource ("/home/osboxes/workspace/repos/ns-3-dev-git-private/enb.png");
  anim.SetBackgroundImage ("/home/osboxes/workspace/netAnimIDEALDemo/netanim/leuven.png", -712, -712, 4, 4, 1);
  anim.UpdateNodeImage (0, resourceId0);
  anim.UpdateNodeSize (0, 150, 250);
  anim.UpdateNodeImage (1, resourceId1);
  anim.UpdateNodeSize (1, 150, 250);
  anim.UpdateNodeImage (2, resourceId2);
  anim.UpdateNodeSize (2, 100, 100);
  for (uint32_t i = 3; i < (ueNodes.GetN () + 3); ++i)
    {
      //Ptr <Node> fromNode = NodeList::GetNode (i)
      anim.UpdateNodeImage (i, resourceId3);
      anim.UpdateNodeSize (i, 40, 40);
    }*/
  //lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (min/1000000 + appTrafficStartTime)));
  #ifdef FLOW_MONITOR
  Simulator::Schedule(Seconds(1.0), &throughput, monitor);
  #endif
  Simulator::Run();
  //Simulator::Schedule(Seconds(10.0), &throughput, monitor);
  //throughput(monitor);

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}

