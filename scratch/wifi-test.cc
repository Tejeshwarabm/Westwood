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

uint32_t    nLeftLeaf = 3;
uint32_t    nRightLeaf = 3;

Ptr<PacketSink> sink1;//sink2,sink3;                        /* Pointer to the packet sink application */
uint64_t lastTotalRx1 = 0;//,lastTotalRx2 = 0,lastTotalRx3 = 0;  

void CalculateThroughput ()
{
    Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
    //double cur = ((sink1->GetTotalRx() - lastTotalRx1) + (sink2->GetTotalRx() - lastTotalRx2) + (sink3->GetTotalRx() - lastTotalRx3)) *(double) 8/1e5;     /* Convert Application RX Packets to MBits. */
    double cur = (sink1->GetTotalRx() - lastTotalRx1) * (double) 8/1e5;
    std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
    lastTotalRx1 = sink1->GetTotalRx ();
    //lastTotalRx2 = sink2->GetTotalRx ();
    //lastTotalRx3 = sink3->GetTotalRx ();
    Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}


int main(int argc, char *argv[])
{

    CommandLine cmd;
    cmd.Parse (argc, argv);


    double stopTime = 20;
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
    edgeLink.SetDeviceAttribute  ("DataRate", StringValue ("100Mbps"));
    edgeLink.SetChannelAttribute ("Delay", StringValue ("5ms"));

    /**
    * Creating dumbbell topology which uses point to point channel
    * for edge links and bottleneck using wirelessPointToPointDumbbellHelper
    */
    WirelessPointToPointDumbbellHelper wirtop (nLeftLeaf,
                             nRightLeaf, edgeLink,
                             bottleneckLink, "ns3::RandomWalk2dMobilityModel");

    /**
    * Setting the queue size for pfifo
    */
    Config::SetDefault ("ns3::PfifoFastQueueDisc::Limit", UintegerValue (queueDiscLimit));

    /** TCPW protocol  **/
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));

    /** TCPW+ protocol   **/
    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    //Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    //Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    
    
    /**
    * Installing Internetstack in all nodes
    */
    InternetStackHelper stack;
    wirtop.InstallStack (stack);

    /**
    * Installing queue disc on bottleneck queues
    */
    TrafficControlHelper tchBottleneck;
    tchBottleneck.SetRootQueueDisc (queueDiscVariant);
    tchBottleneck.Install (wirtop.GetLeft ()->GetDevice (0));
    tchBottleneck.Install (wirtop.GetRight ()->GetDevice (0));

    wirtop.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                           Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                           Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

    /**
    * port to be used for first connection from node 0 from left to node 0 on
    * right side
    */
    port = 8080;


    Address tcpReceiverLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper tcpReceiver ("ns3::TcpSocketFactory", tcpReceiverLocalAddress);

    for(uint16_t i = 0; i < 3; i++)
    {
        AddressValue remoteAddress (InetSocketAddress (wirtop.GetRightIpv4Address (i), port)); 
        BulkSendHelper tcpSender ("ns3::TcpSocketFactory", Address ());
        tcpSender.SetAttribute ("Remote", remoteAddress);
        tcpSender.SetAttribute ("SendSize", UintegerValue (1000));
        
        ApplicationContainer senderApp = tcpSender.Install (wirtop.GetLeft (i));
        senderApp.Start (Seconds (1));
        senderApp.Stop (Seconds (stopTime));

        ApplicationContainer receiverApp = tcpReceiver.Install (wirtop.GetRight (i));

        switch(i)
        {
            case 0: sink1 = StaticCast<PacketSink> (receiverApp.Get (0));
                    break;
            case 1: //sink2 = StaticCast<PacketSink> (receiverApp.Get (1));
                    break;  
            case 2: //sink3 = StaticCast<PacketSink> (receiverApp.Get (2));
                    break;
        }
        
        receiverApp.Start (Seconds (1.0));
        receiverApp.Stop (Seconds (stopTime));
    }

    Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

    /**
    *  Enabling Pcap for point to point bottleneck channel to generate Pcap file
    */
    bottleneckLink.EnablePcapAll ("WiFi Dumbell", false);

    /**
    * Initializing routing table on the nodes
    */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    Simulator::Stop (Seconds(30.0));
    Simulator::Run ();
    Simulator::Destroy ();
    
    return 0;

}
