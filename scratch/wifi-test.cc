#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/config-store-module.h"
#include "ns3/wireless-point-to-point-dumbbell.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("Exhaustive Evaluation");

uint32_t nleft=3;
uint32_t nright=3;
uint32_t maxBytes = 1024;
uint64_t lastTotalRx = 0, countDrop = 0 ;
double aggregate=0;
double simulationTime = 20;
std::string tcpVar="westwood";
std::string raaAlgo="arf";
uint32_t totalRxBytesCounter = 0;
 

std::ofstream outfile;
//std::ofstream out('RX-output.txt');

Ptr<PacketSink> sink0;
Ptr<PacketSink> sink1;
Ptr<PacketSink> sink2;

Ptr <PacketSink> pktSink1;
Ptr <PacketSink> pktSink2;
Ptr <PacketSink> pktSink3;

std::vector<std::string> tcpVec;
uint32_t MacTxDropCount, PhyTxDropCount, PhyRxDropCount;

void
MacTxDrop(Ptr<const Packet> p)		// - MacTxDrop is called only when the node is not associated.
{
  NS_LOG_INFO("Packet Drop");
  MacTxDropCount++;
}

void
PrintDrop()
{

  std::cout << Simulator::Now().GetSeconds() << "\t" << MacTxDropCount << "\t"<< PhyTxDropCount << "\t" << PhyRxDropCount << "\n";
    //MacTxDropCount = 0;
    PhyTxDropCount = 0;
    PhyRxDropCount = 0;
  Simulator::Schedule(Seconds(1), &PrintDrop);
}


// PhyTxDrop is called only if you send a packet while the PHY is in sleep
// mode (unlikely, the Mac should wake the PHY before using send). 
void
PhyTxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  PhyTxDropCount++;
}


// Collisions should be in phyRxDropCount, as Yans wifi set collided frames snr on reception,
// but it's not possible to differentiate from propagation loss.
void
PhyRxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  PhyRxDropCount++;
}



/*
static void
RxDrop (Ptr<const Packet> p)
{
  countDrop++;
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  out << Simulator::Now().GetSeconds()<< std::endl;
}

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
}
*/

void
CalculateThroughput ()
{
    Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
    double cur = (sink0->GetTotalRx () + sink1->GetTotalRx () + sink2->GetTotalRx () - lastTotalRx) * (double) 8 /1e6;     
    std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" <<std::endl;
    lastTotalRx = sink0->GetTotalRx () + sink1->GetTotalRx () + sink2->GetTotalRx ();
    aggregate=aggregate+cur;
    outfile << now.GetSeconds () << " " << cur << std::endl;
  
    if( now.GetSeconds() == simulationTime)
        std::cout << "Aggregate Throughput:" << aggregate / simulationTime << "\n";

    Simulator::Schedule (MilliSeconds (1000), &CalculateThroughput);
}

static void Comparison()
{

    NodeContainer          m_leftLeaf;            //!< Left Leaf nodes
    NetDeviceContainer     m_leftLeafDevices;     //!< Left Leaf NetDevices
    NodeContainer          m_rightLeaf;           //!< Right Leaf nodes
    NetDeviceContainer     m_rightLeafDevices;    //!< Right Leaf NetDevices
    NodeContainer          centrallink;             //!< Router and AP
    NetDeviceContainer     m_centralDevices;       //!< Routers NetDevices
    NetDeviceContainer     m_rightRouterDevices;
    Ipv4InterfaceContainer m_leftLeafInterfaces;    //!< Left Leaf interfaces (IPv4)
    Ipv4InterfaceContainer m_leftRouterInterfaces;  //!< Left router interfaces (IPv4)
    Ipv4InterfaceContainer m_rightLeafInterfaces;   //!< Right Leaf interfaces (IPv4)
    Ipv4InterfaceContainer m_rightRouterInterfaces; //!< Right router interfaces (IPv4)
    Ipv4InterfaceContainer m_routerInterfaces;      //!< Router interfaces (IPv4)
    WifiHelper wifi;
    MobilityHelper mobility;
    NodeContainer ap;
    WifiMacHelper wifiMac;
    NodeContainer wifiApNode;
    NetDeviceContainer apDevice;
  
    
    // Create the BottleNeck Routers
    centrallink.Create(2);

    // Create the leaf nodes
    m_leftLeaf.Create (nleft);
    m_rightLeaf.Create (nright);
    
    PointToPointHelper right,central;
  
    central.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    central.SetChannelAttribute ("Delay", StringValue ("2ms"));
    
    // Add the link connecting routers
    m_centralDevices = central.Install (centrallink);
    wifiApNode = centrallink.Get (0);
    
    
    // Add the left side links
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  
    //wifiPhy.SetErrorRateModel ("ns3::DsssErrorRateModel");
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
  
    Ssid ssid = Ssid ("wifi-default");
    wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
    
    //set Rate Adaptation Algorithm
    if(raaAlgo == "aarf")
        wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
    else if(raaAlgo == "ideal")
        wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
    else if(raaAlgo == "minstrel")
        wifi.SetRemoteStationManager ("ns3::MinstrelWifiManager");
    else
        wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
  
    // setup stas.
    wifiMac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid));

    m_leftLeafDevices = wifi.Install (wifiPhy, wifiMac, m_leftLeaf);
  
    // setup ap.
    wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
    apDevice = wifi.Install (wifiPhy, wifiMac, wifiApNode);
  
    // mobility.
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  
    //Left leaf nodes Mobility
    mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel");
    mobility.Install (m_leftLeaf);
  
    //AP Mobility
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode);

  
    right.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    right.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
    for (uint32_t i = 0; i < nright; ++i)
    {
        NetDeviceContainer c = right.Install (centrallink.Get (1),
                                                  m_rightLeaf.Get (i));
        m_rightRouterDevices.Add (c.Get (0));
        m_rightLeafDevices.Add (c.Get (1));
    }  
  
 
    //Config::Set ("$ns3::NodeListPriv/NodeList/4/$ns3::TcpL4Protocol/SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    
    //set TCP variant
    if(tcpVar == "westwoodplus") 
        Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    
    InternetStackHelper stack;
    stack.Install (centrallink);
    stack.Install (m_leftLeaf);
    stack.Install (m_rightLeaf);
  
  
    //Assign IPAddress
    Ipv4AddressHelper routerIp;
    Ipv4AddressHelper rightIp;
    Ipv4AddressHelper leftIp;
    
    routerIp.SetBase ("10.1.1.0", "255.255.255.0");
    m_routerInterfaces = routerIp.Assign (m_centralDevices);
     
    // Assign to left side 
    leftIp.SetBase("10.1.2.0","255.255.255.0");
    NetDeviceContainer ndc;
    ndc.Add (m_leftLeafDevices);
    ndc.Add(apDevice);
    Ipv4InterfaceContainer ifc = leftIp.Assign (ndc);
   
    for (uint32_t i = 0; i < m_leftLeaf.GetN (); ++i)
    {
        m_leftLeafInterfaces.Add (ifc.Get (i));
    }
    m_leftRouterInterfaces.Add (ifc.Get (m_leftLeaf.GetN ()));
    leftIp.NewNetwork ();
  
    // Assign to right side 
    rightIp.SetBase("10.1.3.0","255.255.255.0");
    for (uint32_t i = 0; i < m_rightLeaf.GetN (); ++i)
    { 
        NetDeviceContainer ndc;
        ndc.Add (m_rightLeafDevices.Get (i));
        ndc.Add (m_rightRouterDevices.Get (i));
        Ipv4InterfaceContainer ifc = rightIp.Assign (ndc);
        m_rightLeafInterfaces.Add (ifc.Get (0));
        m_rightRouterInterfaces.Add (ifc.Get (1));
        rightIp.NewNetwork ();
    }
  
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    BulkSendHelper source ("ns3::TcpSocketFactory", Address ());
    //source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

    ApplicationContainer sourceApps;

    for (uint32_t i = 0; i < nleft; ++i)
    {
        AddressValue sourceaddress (InetSocketAddress (m_rightLeafInterfaces.GetAddress (i), 8080));
        source.SetAttribute ("Remote", sourceaddress);
	    source.SetAttribute ("SendSize", UintegerValue (1000));
        sourceApps.Add (source.Install (m_leftLeaf.Get (i)));
    }
  
    sourceApps.Start (Seconds (0.0));
    sourceApps.Stop (Seconds (simulationTime));

    PacketSinkHelper sinkhelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 8080));
    ApplicationContainer sinkApps;
  
  
    for (uint32_t i = 0; i < nright; ++i)
    {
        sinkApps.Add (sinkhelper.Install (m_rightLeaf.Get (i)));
    }
  
    sink0 = StaticCast<PacketSink> (sinkApps.Get (0));
    sink1 = StaticCast<PacketSink> (sinkApps.Get (1));
    sink2 = StaticCast<PacketSink> (sinkApps.Get (2));
    
    pktSink1 = DynamicCast <PacketSink> (sinkApps.Get (0));
    pktSink2 = DynamicCast <PacketSink> (sinkApps.Get (1));
    pktSink3 = DynamicCast <PacketSink> (sinkApps.Get (2));
  
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (simulationTime));
  
    /* // Add ErrorRate on AP
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue(0.005));
    wp2p.GetLeft ()->GetDevice (0) -> SetAttribute ("ReceiveErrorModel", PointerValue (em));

    */
    /**
    *  Enabling Pcap for point to point bottleneck channel to generate Pcap file
    */
    //central.EnablePcapAll ("WiFi Dumbell", false);

   // Trace Collisions
   Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
   Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
   Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));
  
   // PrintDrop function calculates number of packets drops
   //Simulator::Schedule(Seconds(1), &PrintDrop);

    std::ostringstream tss;
    
    tss << tcpVar<< "-" << raaAlgo <<".plt";

    outfile.open (tss.str().c_str(), std::ofstream::out);

    outfile<< "set terminal png" <<"\n";
    outfile<< "set output \"" << tcpVar << "-" << raaAlgo << ".png" <<"\"\n"; 
    outfile<< "set title \"" << tcpVar << "-" << raaAlgo<< "\"\n";
    outfile<< "set xlabel \"Time (seconds)\"\n";
    outfile<< "set ylabel \"Throughput(Mbps)\"\n\n";
    outfile<< "set xrange [4:20]\n";
    outfile<<"plot \"-\"  title \"Throughput vs Time\" with linespoints\n";
    
    Simulator::Schedule (Seconds (1), &CalculateThroughput);
  
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop(Seconds(simulationTime+1));
    Simulator::Run ();
    
    //calculate goodput
    totalRxBytesCounter = (pktSink1->GetTotalRx () + pktSink2->GetTotalRx () + pktSink3->GetTotalRx ()) * (double) 8/1e6;
    NS_LOG_UNCOND ("----------------------------    "
                     << "\nGoodput Mbits/sec:"
                     << totalRxBytesCounter/Simulator::Now ().GetSeconds ());
                     
    Simulator::Destroy ();
  
    outfile <<"e\n";
    outfile.close ();
    
    system(("gnuplot " + tcpVar + "-" + raaAlgo + ".plt").c_str());


}


int main (int argc, char *argv[])
{ 
    CommandLine cmd;
    cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue ("tcpVariant", "Variant of TCP to use", tcpVar); 
    cmd.AddValue("raaAlgo", "type of Rate Adaptation Algorithm to use", raaAlgo);  
    cmd.Parse (argc, argv);
      
    tcpVec.push_back("westwood");
    tcpVec.push_back("westwoodplus");
    
    //for( int i=0;i<2;i++)
    //{
        Comparison(); 
        //lastTotalRx = 0;
        //countDrop=0;
        //aggregate=0;        
    
    //}
    
    return 0;
  
}
