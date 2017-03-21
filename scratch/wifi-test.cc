/*

   Node_1----Wireless_1..                                      Node_4
                        :.                                    /
                         :...   Base   central link          /
   Node_2----Wireless_2 ....:..Station-------------- Router_2 --- Node_5
                         ...: (Router 1)                     \
                        .:                                    \
   Node_3----Wireless_3.:                                      Node_6


*/

#include <iostream>
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/wireless-point-to-point-dumbbell.h"

using namespace ns3;

int main()
{

/**
* Number of nodes on left side of bottleneck
* Number of nodes on right side of bottleneck
* TCP Maximum window size. Default = 2000 bytes
* Bandwidth of Bottleneck link
* Propagation dealy of bottleneck link
* Bandwidth dealy of edge link
* Propagation dealy of edge links
*/

uint32_t    nLeftLeaf = 3;
uint32_t    nRightLeaf = 3;

//std::string bottleneckBandwidth = "1Mbps";
//std::string bottleneckDelay = "20ms";

std::string edgeLinkBandwidth = "2Mbps";
std::string edgeLinkDelay = "5ms";
//std::string mobilityModel = "RandomWalk2d";
double stopTime = 10;
uint16_t port;
std::string queueDiscVariant = "ns3::PfifoFastQueueDisc";
uint32_t      queueDiscLimit = 200;

/**
* Creating point to point channel for bottleneck
* Setting the date rate of the bottleneck link
* Setting the propagation dealy of bottleneck link
*/
PointToPointHelper bottleneckLink;
bottleneckLink.SetDeviceAttribute  ("DataRate", StringValue ("100Mbps"));
bottleneckLink.SetChannelAttribute ("Delay", StringValue ("2ms"));

/**
* Creating point to point channel for edge links
* Setting the date rate of the edge links
* Setting the propagation dealy of edge links
*/
PointToPointHelper edgeLink;
edgeLink.SetDeviceAttribute  ("DataRate", StringValue ("2Mbps"));
edgeLink.SetChannelAttribute ("Delay", StringValue ("5ms"));

/**
* Creating dumbbell topology which uses point to point channel
* for edge links and bottleneck using PointToPointDumbbellHelper
*/
WirelessPointToPointDumbbellHelper d (nLeftLeaf,
                         nRightLeaf, edgeLink,
                         bottleneckLink, "ns3::RandomWalk2dMobilityModel");

        

/**
* Setting the queue size for pfifo
*/
Config::SetDefault ("ns3::PfifoFastQueueDisc::Limit", UintegerValue (queueDiscLimit));
//Config::SetDefault ("ns3::TcpWestwood::ProtocolType", StringValue ("TcpWestwood::WESTWOOD"));

Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
//Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));

/**
* Installing Internetstack in all nodes
*/
InternetStackHelper stack;
d.InstallStack (stack);

/**
* Installing queue disc on bottleneck queues
*/
TrafficControlHelper tchBottleneck;
tchBottleneck.SetRootQueueDisc (queueDiscVariant);
tchBottleneck.Install (d.GetLeft ()->GetDevice (0));
tchBottleneck.Install (d.GetRight ()->GetDevice (0));

d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                       Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                       Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

/**
* port to be used for first connection from node 0 from left to node 0 on
* right side
*/
port = 50000;


Address tcpReceiverLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
PacketSinkHelper tcpReceiver ("ns3::TcpSocketFactory", tcpReceiverLocalAddress);

for(uint16_t i = 0; i < 3; i++)
  {
    AddressValue remoteAddress (InetSocketAddress (d.GetRightIpv4Address (i), port)); 
    BulkSendHelper tcpSender ("ns3::TcpSocketFactory", Address ());
    tcpSender.SetAttribute ("Remote", remoteAddress);
    tcpSender.SetAttribute ("SendSize", UintegerValue (1000));
    ApplicationContainer senderApp = tcpSender.Install (d.GetLeft (i));
    senderApp.Start (Seconds (2 * i));
    senderApp.Stop (Seconds (stopTime - 1));

    ApplicationContainer receiverApp = tcpReceiver.Install (d.GetRight (i));
    receiverApp.Start (Seconds (1.0));
    receiverApp.Stop (Seconds (stopTime));
  }

/**
*  Enabling Pcap for point to point bottleneck channel to generate Pcap file
*/
bottleneckLink.EnablePcapAll ("WiFi Dumbell", false);

/**
* Initializing routing table on the nodes
*/
Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
Simulator::Stop (Seconds(50.0));
Simulator::Run ();
Simulator::Destroy ();
return 0;
}
