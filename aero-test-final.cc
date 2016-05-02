/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:"nill"; -*- */
/*
 * Copyright (c) 2012 Amir Reda
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
 *Based on AeroRP 
 *Authors: Dr/ Sherif Khatab <s.khattab@fci-cu.edu.eg>
 *         Eng/ Amir mohamed Reda <amiralex32@gmail.com>
 *
 * this scenario is described as follows two main parts
 * 1- ground station
 *  a- fixed mobility model
 *  b- routing protocol aerorp with flag true to use it in routing protocol and fixed IP 
 *  c- it acts like a server for appliction clint server
 * 2- airborne nodes
 *  a- gauss markov mobility model
 *  b- routing protocol aerorp 
 *  c- TDMA tx range is 28 
 *  d- client and adhoc protocol for application 
 */
 #include "ns3/core-module.h"
 #include "ns3/netanim-module.h"
 #include "ns3/network-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/config-store-module.h"
 #include "ns3/wifi-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/flow-monitor-module.h"
 #include "ns3/stats-module.h"
 #include "ns3/simple-wireless-tdma-module.h"
 #include "ns3/timestamponoff.h"
 #include "ns3/aerorp-module.h"
#include "ns3/wifi-mac.h"
 //#include "ns3/random-variable-stream.h"
 //#include "ns3/UniformRandomVariable.h"
 //#include "ns3/ExponentialRandomVariable.h"
 #include <iostream>
 #include <cmath>

using namespace ns3;

uint16_t port = 5000;

NS_LOG_COMPONENT_DEFINE ("AeroRPSimulationExample");

DataCollector data;
uint32_t packetsTransmitted = 0;
uint64_t n = 0;
Time t_delay = Seconds(0.0), t2_delay = Seconds(0.0);
Ptr<TimeMinMaxAvgTotalCalculator> m_macDelay;
uint64_t helloRx;
uint64_t GsRx;
uint64_t helloTx;
uint64_t GsTx;

void TxCallback (Ptr<CounterCalculator<uint32_t> > datac, std::string path, Ptr<const Packet> packet) 
{
  NS_LOG_INFO ("Sent packet counted in " <<
               datac->GetKey ());
  datac->Update ();
  // end TxCallback
}

void
OnoffTxTrace (std::string context, Ptr<const Packet> p)
{
	packetsTransmitted +=1;
}

void
AeroPacketHelloTrace (std::string context, Ptr<const Packet> p)
{
	helloRx += 1;
}

void
AeroPacketGsTrace (std::string context, Ptr<const Packet> p)
{
	GsRx += 1;
}

void
AeroPacketTxHelloTrace (std::string context, Ptr<const Packet> p)
{
	helloTx += 1;
}

void
AeroPacketTxGsTrace (std::string context, Ptr<const Packet> p)
{
	GsTx += 1;
}

template <class T> inline std::string to_string (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

class AeroRPSimulation
{
public:
  AeroRPSimulation ();
  void PrintTime();	
  void CaseRun (uint32_t nWifis,
                uint32_t nSinks,
                uint32_t nodeSpeed,
                bool securityMode,
                bool attackMode,
                double totalTime,
                double dataStart,
                bool printRoutes,
                std::string rate,
                std::string CSVfileName,
                std::string format,
            	std::string experiment,
            	std::string strategy,
            	std::string input,
            	uint32_t runID);

	void SetOnoffDelayTracker(Ptr<TimeMinMaxAvgTotalCalculator> onoffDelay);
	void SetCounter(Ptr<CounterCalculator<> > calc);
	void SetTxCounter(Ptr<PacketCounterCalculator> calc);
	void SetRoutingPacketCounter(Ptr<CounterCalculator<> > calc);
        uint32_t GetReceivedPackets ();

private:
  ///define data members 
  //no of total nodes
  uint32_t m_nWifis;
  //no of sink nodes
  uint32_t m_nSinks;
  double m_totalTime;
  //rate of physical layer 8kbbs 
  std::string m_rate;
  //speed of nodes
  uint32_t m_nodeSpeed;
  // mode of operation secure or not secure
  bool   m_securityMode;
  // mode of operation with attack or not
  bool   m_attackMode;
  // time of start of data transmission
  double m_dataStart;
  //total received bytes
  uint32_t bytesTotal;
  //total packets received
  uint32_t packetsReceived;
  //to calculate the on off application delay
  Ptr<TimeMinMaxAvgTotalCalculator> m_onoffDelay;
  //calculate the data packets transmitted
  Ptr<PacketCounterCalculator> m_txcalc;
  //calculate the data packets received
  Ptr<CounterCalculator<> > m_calc;
  //calculate the routing packets
  Ptr<CounterCalculator<> > m_routingCalc;
  //print routes to neighbors
  bool m_printRoutes;
  //the file to save
  std::string m_CSVfileName;


  ///define data members of nodes attributes
  NodeContainer anNodes;
  NodeContainer gNodes;
  NetDeviceContainer devices;
  NetDeviceContainer gdevices;
 
  //second set of devices
  NetDeviceContainer devices2;
  NetDeviceContainer gdevices2;
 

 Ipv4InterfaceContainer interfaces;
 Ipv4InterfaceContainer authInterfaces;

private:
  ///behaviors for creating network
  void CreateNodes ();
  ///tr_name is the name of the file 
  void CreateDevices (std::string tr_name);
  ///the file name tr_name of print routes
  void InstallInternetStack (std::string tr_name);
  void InstallApplications ();
  void SetupMobility ();
  //method used to receive packet and return the packets received 
  void ReceivePacket (Ptr <Socket> );
  //return socket
  Ptr <Socket> SetupPacketReceive (Ipv4Address, Ptr <Node> );
  //calculate throughput
  void CheckThroughput ();

};

int main (int argc, char **argv)
{
  //Packet::EnablePrinting();
  AeroRPSimulation test;
  uint32_t nWifis = 4;
  uint32_t nSinks = 1;
  uint32_t seed = 12345;
  uint32_t runID = 1;
  std::string format("db");
  std::string experiment("main-scenario");
  std::string strategy("attack-status");
  std::string input;
  std::string runId;
  double totalTime = 1500.0;
  std::string rate ("8kbps");
  uint32_t nodeSpeed = 1200; // in m/s
  bool securitystatus = false;
  bool attackstatus = false;
  double dataStart = 100.0;
  bool printRoutingTable = false;
  std::string CSVfileName = "AeroRPSimulation.csv";

  {
    stringstream sstr;
    sstr << "run-" << time (NULL);
    runId = sstr.str ();
  }

  CommandLine cmd;
  cmd.AddValue ("seed", "Random seed [Default:12345]", seed);
  cmd.AddValue ("nWifis", "Number of wifi nodes[Default:60]", nWifis);
  cmd.AddValue ("nSinks", "Number of wifi sink nodes[Default:10]", nSinks);
  cmd.AddValue ("totalTime", "Total Simulation time[Default:1000]", totalTime);
  cmd.AddValue ("rate", "CBR traffic rate[Default:4kbps]", rate);
  cmd.AddValue ("nodeSpeed", "Node speed in RandomWayPoint model[Default:1200]", nodeSpeed);
  cmd.AddValue ("securestatus", "security mode on or off[Default:false]", securitystatus);
  cmd.AddValue ("attackstatus", "attack mode on or off[Default:false]", attackstatus);
  cmd.AddValue ("dataStart", "Time at which nodes start to transmit data[Default=100.0]", dataStart);
  cmd.AddValue ("printRoutingTable", "print routing table for nodes[Default:1]", printRoutingTable);
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name[Default:AeroRPSimulation.csv]", CSVfileName);
  cmd.AddValue("format", "Format to use for data output.[Default:db]", format);
  cmd.AddValue ("run", "Identifier for run.[Default:1]", runId);
  cmd.AddValue ("seed", "Set seed.[Default:12345]", seed);
  cmd.Parse (argc, argv);

  {
    stringstream sstr ("");
    sstr << nWifis;
    input = sstr.str ();
  }

  {
    stringstream sstr ("");
    sstr << attackstatus;
    strategy = sstr.str ();
  }
/*
  {
    stringstream sstr ("");
    sstr << securitystatus;
    strategy = sstr.str ();
  }
*/
  std::ofstream out (CSVfileName.c_str ());
  out << "SimulationSecond," <<
  "throughput," <<
  "PacketsReceived," <<
  "PacketsTransmitted," <<
  "TimeDelay," <<
  "NumberOfSinks," <<
  std::endl;
  out.close ();
/*
   LogComponentEnable("AeroRPRoutingProtocol", LOG_LEVEL_ALL);
   LogComponentEnable("AeroRPGcmConverter", LOG_LEVEL_ALL);
   LogComponentEnable("AeroRPCertificateAuthority", LOG_LEVEL_ALL);
   //LogComponentEnable("AeroRPPacket", LOG_LEVEL_ALL);
   LogComponentEnable("AeroRPNeighborTable", LOG_LEVEL_ALL);
   LogComponentEnable("AeroRPPositionTable", LOG_LEVEL_ALL);
   LogComponentEnable("AeroRPPacketQueue", LOG_LEVEL_ALL);
//*/
/* 
   LogComponentEnable("AeroRPRoutingProtocol", LOG_NONE);
   LogComponentEnable("AeroRPNeighborTable", LOG_NONE);
   LogComponentEnable("AeroRPPositionTable", LOG_NONE);
*/
  //SeedManager::SetSeed (12345);
   cout << "Seeting seed to: " << seed << endl;
   RngSeedManager::SetSeed (seed);

   //Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
   //Ptr<ExponentialRandomVariable> y = CreateObject<ExponentialRandomVarlable> ();


  Config::SetDefault ("ns3::AeroRP::NeighborTable::MaxRange", DoubleValue (27800));
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", StringValue ("1000"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (rate));

  test = AeroRPSimulation ();

  //Stats and Data Collection 
  data.DescribeRun(experiment, strategy, input, runId);
  //Set seed and run value
  //SeedManager::SetSeed(seed);
 //SeedManager::SetRun(runID);
  data.AddMetadata("seed", seed);
  data.AddMetadata("runid", runID);
	
  //Datacalculator for transmitted packets from Onoff-application
  Ptr<PacketCounterCalculator> apptx = CreateObject<PacketCounterCalculator>();
  apptx->SetKey("onoffTx");
  apptx->SetContext ("transmitted-packets");
  test.SetTxCounter(apptx);
  data.AddDataCalculator(apptx);

   //Datacalculator for received packets from Onoff-application
  Ptr<CounterCalculator<> > appRx = CreateObject<CounterCalculator<> >();
  appRx->SetKey("onoffRx");
  appRx->SetContext ("received-packets");
  test.SetCounter(appRx);
  data.AddDataCalculator(appRx);
/*
  //Datacalculator for packetdelay to Onoff packets
  Ptr<TimeMinMaxAvgTotalCalculator> onoff_delay = CreateObject<TimeMinMaxAvgTotalCalculator>();
  onoff_delay->SetKey("onoffDelay");
  test.SetOnoffDelayTracker(onoff_delay);
  data.AddDataCalculator(onoff_delay);
*/

  //Datacalculator for transmitted HELLO packets
//  Ptr<PacketSizeMinMaxAvgTotalCalculator> helloTx = CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
//  helloTx->SetKey("helloTx");
// Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/TxHello", MakeCallback(&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate, helloTx));
// data.AddDataCalculator(helloTx);

  //Datacalculator for received HELLO packets
 // Ptr<PacketSizeMinMaxAvgTotalCalculator> helloRx = CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
 // helloRx->SetKey("helloRx");
 //Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/RxHello", MakeCallback(&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate, helloRx));
// data.AddDataCalculator(helloRx);

  //Datacalculator for transmitted GS packets
//  Ptr<PacketSizeMinMaxAvgTotalCalculator> GSTx = CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
//  GSTx->SetKey("GSTx");
// Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/TxGstation", MakeCallback(&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate, GSTx));
// data.AddDataCalculator(GSTx);

  //Datacalculator for received GS packets
//  Ptr<PacketSizeMinMaxAvgTotalCalculator> GSRx = CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
//  GSRx->SetKey("GSRx");
// Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/RxGstation", MakeCallback(&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate, GSRx));
// data.AddDataCalculator(GSRx);

  test.CaseRun (nWifis,nSinks,nodeSpeed,securitystatus ,attackstatus ,totalTime,dataStart, printRoutingTable, rate, CSVfileName,format, experiment, strategy, input, runID);

  std::cout << "Trace on off Tx Packets:\t" << packetsTransmitted << "\n\n";
  std::cout << "Trace on off Rx Packets:\t" << test.GetReceivedPackets() << "\n\n";
  std::cout << "Trace PDR:\t" << (static_cast<double>(test.GetReceivedPackets()/packetsTransmitted)) << "\n\n";
  std::cout << "Trace on off packet Delay:\t" << (t_delay.GetSeconds())/n << "\n\n";
  std::cout << "Trace AeroRP Tx HELLO:\t" << helloTx << "\n\n";
  std::cout << "Trace AeroRP Rx HELLO:\t" << helloRx << "\n\n";
  std::cout << "Trace AeroRP Tx GS:\t" << GsTx << "\n\n";
  std::cout << "Trace AeroRP Rx GS:\t" << GsRx << "\n\n";

  Simulator::Destroy ();
  //Pick an output writer based in the requested format
  Ptr<DataOutputInterface> output = 0;
   #ifdef STATS_HAS_SQLITE3
     NS_LOG_INFO("Creating sqlite formatted data output.");
     output = CreateObject<SqliteDataOutput>();
   #endif
   //Writer interrogate the DataCollector and save the results
	if (output != 0)
	output->Output(data);

  return 0;
}

void 
AeroRPSimulation::PrintTime()
{
   cout << "Time: " << Simulator::Now().GetSeconds() << endl;
   Simulator::Schedule(Seconds(100.0), &AeroRPSimulation::PrintTime, this);
}

AeroRPSimulation::AeroRPSimulation ()
  : bytesTotal (0),
    packetsReceived (0),
    m_onoffDelay (0)
{
}

void
AeroRPSimulation::SetOnoffDelayTracker(Ptr<TimeMinMaxAvgTotalCalculator> onoffDelay)
{
  m_onoffDelay = onoffDelay;
}

void
AeroRPSimulation::SetCounter(Ptr<CounterCalculator<> > calc)
{
	m_calc = calc;
}

void
AeroRPSimulation::SetTxCounter(Ptr<PacketCounterCalculator> calc)
{
	m_txcalc = calc;
}

void 
AeroRPSimulation :: SetRoutingPacketCounter(Ptr<CounterCalculator<> > calc)
{
  m_routingCalc = calc;
}

uint32_t
AeroRPSimulation :: GetReceivedPackets ()
{
  return packetsReceived;
}

void
AeroRPSimulation::ReceivePacket (Ptr <Socket> socket)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << " Received one packet!");
  Ptr <Packet> packet;
  while (packet = socket->Recv ())
    {
      		TimeStampOnOff tag;
		if (packet->PeekPacketTag(tag))
		{
			Time tx = tag.GetTimestamp();
	        	t_delay = t_delay + (Simulator::Now() - tx);
			n++;
			if (m_onoffDelay != 0)
			{
				m_onoffDelay->Update((Simulator::Now() - tx));
			}
		}
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      m_calc->Update();
    }
}

void
AeroRPSimulation::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1000;
  bytesTotal = 0;

  std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

  out << (Simulator::Now ()).GetSeconds () << "," << kbs << "," << packetsReceived << ","<< packetsTransmitted << ","<< (t_delay.GetSeconds())/n << ","<< m_nSinks << std::endl;

  out.close ();

  //packetsReceived = 0;
  //packetsTransmitted = 0;

  Simulator::Schedule (Seconds (1.0), &AeroRPSimulation::CheckThroughput, this);
}

Ptr <Socket>
AeroRPSimulation::SetupPacketReceive (Ipv4Address addr, Ptr <Node> node)
{

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr <Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback ( &AeroRPSimulation::ReceivePacket, this));

  return sink;
}

void
AeroRPSimulation::CaseRun (uint32_t nWifis,uint32_t nSinks, uint32_t nodeSpeed ,bool securemode,bool attackmode,double totalTime,  double dataStart, bool printRoutes,std::string rate, std::string CSVfileName ,std::string format, std::string experiment,std::string strategy, std::string input, uint32_t runID)
{
  m_nWifis = nWifis;
  m_nSinks = nSinks;
  m_totalTime = totalTime;
  m_rate = rate;
  m_nodeSpeed = nodeSpeed;
  m_securityMode = securemode;
  m_attackMode = attackmode;
  m_dataStart = dataStart;
  m_printRoutes = printRoutes;
  m_CSVfileName = CSVfileName;

  std::stringstream ss;
  ss << m_nWifis;
  std::string t_nodes = ss.str ();

  std::stringstream ss3;
  ss3 << m_totalTime;
  std::string sTotalTime = ss3.str ();

  std::string tr_name = "AeroRP_" + t_nodes + "Nodes_" + sTotalTime + "SimTime";
  std::cout << "Trace file generated is " << tr_name << ".tr\n";

  CreateNodes ();
  CreateDevices (tr_name);
  SetupMobility ();
  InstallInternetStack (tr_name);
  InstallApplications ();

for (NodeContainer::Iterator i = gNodes.Begin (); i != gNodes.End (); i++)
    {
      Ptr<Node> node = (*i);
      Ptr<AeroRP::AeroRoutingProtocol> aerorp = node->GetObject<AeroRP::AeroRoutingProtocol> ();
      aerorp->SetGroundStation();
      aerorp->SetSecurityMode(m_securityMode);
      aerorp->SetAttackMode(m_attackMode);
      aerorp->Start(nWifis);

    }

for (NodeContainer::Iterator i = anNodes.Begin (); i != anNodes.End (); i++)
    {
      Ptr<Node> node = (*i);
      Ptr<AeroRP::AeroRoutingProtocol> aerorp = node->GetObject<AeroRP::AeroRoutingProtocol> ();
      aerorp->SetSecurityMode(m_securityMode);
      aerorp->Start(nWifis);
     
    }


  AeroRPHelper aerorp;
  aerorp.Install ();
  
  std::cout << "\nStarting simulation for " << m_totalTime << " s ...\n";

  CheckThroughput ();

/*
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;

  Config::SetDefault ("ns3::FlowMonitor::StartTime",TimeValue (Seconds (m_dataStart)));
  Config::SetDefault ("ns3::FlowMonitor::PacketSizeBinWidth", DoubleValue (1000));
 //Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
    {
  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
       std::cout << "Time: "<< Simulator :: Now() <<"Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress;
      std::cout << "Tx Packets = " << iter->second.txPackets;
      std::cout << "Rx Packets = " << iter->second.rxPackets;
      std::cout << "Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps";
    }

  monitor->SerializeToXmlFile("aerorp.flowmon", true, true);
  monitor = flowmon.InstallAll();

*/

  //FlowMonitorHelper flowmon;
  //Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop (Seconds (m_totalTime));

  Ptr<Node> node1 = NodeList::GetNode (1);
  AnimationInterface::SetNodeColor(node1,0,255,0);

  AnimationInterface anim ("aerorp.xml");

  anim.UpdateNodeColor(node1,0,255,0);

  PrintTime();

 Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/RxHello", MakeCallback (&AeroPacketHelloTrace));
  Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/RxGstation", MakeCallback (&AeroPacketGsTrace));
 Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/TxGstation", MakeCallback (&AeroPacketTxHelloTrace));
 Config::Connect ("/NodeList/*/$ns3::AeroRP::AeroRoutingProtocol/TxHello", MakeCallback (&AeroPacketTxGsTrace));

  Simulator::Run ();
/*
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  uint64_t totalPacketsTx = 0, totalPacketsRx = 0, numberOfFlows = 0, numberOfRoutingPackets = 0;;
  Time totalDelay = Seconds(0.0);
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

	   if (t.destinationAddress == "10.0.0.255")
	     {
	      numberOfRoutingPackets = numberOfRoutingPackets + i->second.rxPackets;
	     }

		if (t.destinationAddress != "10.0.0.255")
		{
		   totalPacketsTx = totalPacketsTx + i->second.txPackets;
            	   totalPacketsRx = totalPacketsRx + i->second.rxPackets;
		   numberOfFlows += 1;
		   totalDelay = totalDelay + i->second.delaySum;
		}
	}

  monitor->SerializeToXmlFile("aerorp.flowmon", true, true);
*/
  //Simulator::Destroy ();
}

void
AeroRPSimulation::CreateNodes ()
{
  std::cout << "Creating " << (unsigned) m_nWifis << " anNodes.\n";
  gNodes.Create (m_nSinks);
  anNodes.Create (m_nWifis);
  
  NS_ASSERT_MSG (m_nWifis > m_nSinks, "Sinks must be less or equal to the number of nodes in network");
}

void
AeroRPSimulation::SetupMobility ()
{
  MobilityHelper gmobility;
  gmobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (75000.0),
                                "MinY", DoubleValue (75000.0),
                                "DeltaX", DoubleValue (0.0),
                                "DeltaY", DoubleValue (0.0),
                                "GridWidth", UintegerValue (1),
                                "LayoutType", StringValue ("RowFirst"));
  gmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  gmobility.Install (gNodes);

  Ptr<Node> node1 = NodeList::GetNode (1);
  MobilityHelper mobility1;
  Ptr<ListPositionAllocator> positionAlloc1 = CreateObject<ListPositionAllocator> ();
  positionAlloc1->Add (Vector (85000.0, 85000.0, 0.0));
  mobility1.SetPositionAllocator (positionAlloc1);
  mobility1.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
                             "Bounds", BoxValue (Box (80000, 90000, 80000, 90000, 0, 1000)),
                             "TimeStep", TimeValue (Seconds (10.0)),
                             "Alpha", DoubleValue (0.85),
   "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=800|Max=1200]"),
   "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
   "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
   "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
   "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
   "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));

  mobility1.Install (node1);

   MobilityHelper mobility;
   mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
   "Bounds", BoxValue (Box (0, 150000, 0, 150000, 0, 1000)),
   
    /*a new set of values are calculated for each node. Since the nodes’ velocity and direction are    
    * fixed 
    *until the next timestep, setting a large timestep will result in long periods of straight 
    * movement. 
    *A short timestep such as 0.25 s, will result in a path that is almost continuously changing. The 
    *timestep value for our simulations is set to 10 s.
   */
   "TimeStep", TimeValue (Seconds (10.0)),
  /*
    *Setting alpha between zero and one allows us to tune the model with degrees of memory and 
    *variation.
    *In order to analyze the impact of alpha on the mobility, we conducted baseline simulations.We    
    *observe that as alpha increases, the node paths become less random and more predictable. For the 
    *rest of the simulations we kept alpha value to be 0.85 to have some predictability in the mobility 
    *of the nodes, while avoiding abrupt AN direction changes
    */
   "Alpha", DoubleValue (0.85),
   "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=800|Max=1200]"),
   "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
   "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
   "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
   "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
   "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));
   mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
   "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
   "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
   "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=1000]"));

  mobility.Install (anNodes);}

void
AeroRPSimulation::CreateDevices (std::string tr_name)
{

/*
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (m_phyMode), "ControlMode",StringValue (m_phyMode));

  devices = wifi.Install (wifiPhy, wifiMac, anNodes);
  gdevices = wifi.Install (wifiPhy, wifiMac, gNodes);

  AsciiTraceHelper ascii;
  wifiPhy.EnableAsciiAll (ascii.CreateFileStream (tr_name + ".tr"));
  wifiPhy.EnablePcapAll (tr_name);
 */
 
     Config::SetDefault ("ns3::SimpleWirelessChannel::MaxRange", DoubleValue (27800));
     Config::SetDefault ("ns3::SimpleWirelessChannel::GroundRange", DoubleValue (150000));
     Config::SetDefault ("ns3::SimpleWirelessChannel::GroundID", IntegerValue (0));
      // default allocation, each node gets a slot to transmit
      TdmaHelper tdma = TdmaHelper (anNodes.GetN ()+gNodes.GetN(), anNodes.GetN ()+gNodes.GetN()); // in this case selected, numSlots = nodes
     
      TdmaControllerHelper controller;
      controller.Set ("SlotTime", TimeValue (MicroSeconds (1100)));
      controller.Set ("GaurdTime", TimeValue (MicroSeconds (100)));
      controller.Set ("InterFrameTime", TimeValue (MicroSeconds (10)));
      tdma.SetTdmaControllerHelper (controller);

      devices = tdma.Install (anNodes);

      gdevices = tdma.Install (gNodes);


	//try to install a second device for each node

// default allocation, each node gets a slot to transmit
      TdmaHelper tdma2 = TdmaHelper (anNodes.GetN ()+gNodes.GetN(), anNodes.GetN ()+gNodes.GetN(),150000, 150000, 0);
      //TdmaHelper tdma2 = TdmaHelper (anNodes.GetN ()+gNodes.GetN(), anNodes.GetN ()+gNodes.GetN()); 
// in this case selected, numSlots = nodes
     
      TdmaControllerHelper controller2;
      controller2.Set ("SlotTime", TimeValue (MicroSeconds (1100)));
      controller2.Set ("GaurdTime", TimeValue (MicroSeconds (100)));
      controller2.Set ("InterFrameTime", TimeValue (MicroSeconds (10)));
      tdma2.SetTdmaControllerHelper (controller2); 

      devices2 = tdma2.Install (anNodes);

      gdevices2 = tdma2.Install (gNodes);

/*

      AsciiTraceHelper ascii;
      Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (tr_name + (".tr"));
      tdma.EnableAsciiAll (stream); 
*/ 
}

void
AeroRPSimulation::InstallInternetStack (std::string tr_name)
{
  Config::SetDefault ("ns3::AeroRP::NeighborTable::MaxRange", DoubleValue (27800));
  AeroRPHelper aerorp;

  InternetStackHelper stack;
  stack.SetRoutingHelper (aerorp); // has effect on the next Install ()
   stack.Install (gNodes);
  stack.Install (anNodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (gdevices);
  interfaces = address.Assign (devices);

  Ipv4AddressHelper address2;
  address2.SetBase ("192.168.1.0", "255.255.255.0");
  address2.Assign (gdevices2);
  authInterfaces = address2.Assign (devices2);


  if (m_printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ((tr_name + ".routes"), std::ios::out);
      //this make a problem to us in the printing way
      //aerorp.PrintRoutingTableAllEvery (Seconds ( 1 ), routingStream);
      aerorp.PrintRoutingTableAllAt (Seconds (31), routingStream);
    }
}

void
AeroRPSimulation::InstallApplications ()
{
      Ptr<Node> appNode1 = NodeList::GetNode (1);

      Ipv4Address nodeAddress = appNode1->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
      Ptr<Socket> sink = SetupPacketReceive (nodeAddress, appNode1);
 
  for (uint32_t clientNode = 1; clientNode <= m_nWifis - 1; clientNode++ )
      { 
          OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (0), port)));
          onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
          onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

              ApplicationContainer apps1 = onoff1.Install (anNodes.Get (clientNode));
              Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
              apps1.Start (Seconds (var->GetValue (m_dataStart, m_dataStart + 1)));
              apps1.Stop (Seconds (m_totalTime - 400));
      } 
 Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx", MakeCallback (&OnoffTxTrace));
 Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx", MakeCallback (&PacketCounterCalculator::PacketUpdate, m_txcalc));
}

