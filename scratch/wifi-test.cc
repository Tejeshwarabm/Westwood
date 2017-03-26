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
    port = 50000;
    /**
    * TCP application that send data from Node1 to Node3
    * Configuring sender application
    * Installing  application on left side nodes
    * Starting the application on left side node at 1 second
    */
    AddressValue remoteAddress1 (InetSocketAddress (wirtop.GetRightIpv4Address (0), port));
    BulkSendHelper tcpSender1 ("ns3::TcpSocketFactory", Address ());
    tcpSender1.SetAttribute ("Remote", remoteAddress1);
    tcpSender1.SetAttribute ("SendSize", UintegerValue (1000));
    ApplicationContainer senderApp1 = tcpSender1.Install (wirtop.GetLeft (0));
    senderApp1.Start (Seconds (1.0));
    senderApp1.Stop (Seconds (stopTime));



    /**
	* Configuring receiver application
	* Installing packet sink application on right side nodes
	* Starting the application at 1 second and stoping it at stopTime
	*/
	Address tcpReceiverLocalAddress1 (InetSocketAddress (Ipv4Address::GetAny (), port));
	PacketSinkHelper tcpReceiver1 ("ns3::TcpSocketFactory", tcpReceiverLocalAddress1);
	tcpReceiver1.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
	ApplicationContainer receiverApp1 = tcpReceiver1.Install (wirtop.GetRight (0));
	receiverApp1.Start (Seconds (1.0));
	receiverApp1.Stop (Seconds (stopTime));


	port = 50001;

	/**
	* TCP application that send data from Node2 to Node3
	* Configuring sender application
	* Installing  application on left side nodes
	* Starting the application on left side node2 at 15th second
	*/
	AddressValue remoteAddress2 (InetSocketAddress (wirtop.GetRightIpv4Address (0), port));
	BulkSendHelper tcpSender2 ("ns3::TcpSocketFactory", Address ());
	tcpSender2.SetAttribute ("Remote", remoteAddress2);
	tcpSender2.SetAttribute ("SendSize", UintegerValue (1000));
	ApplicationContainer senderApp2 = tcpSender2.Install (wirtop.GetLeft (1));
	senderApp2.Start (Seconds (1.0));
	senderApp2.Stop (Seconds (stopTime - 1));

	/**
	* Configuring receiver application
	* Installing packet sink application on right side nodes
	* Starting the application at 1.0 second and stoping it at stopTime
	*/
	Address tcpReceiverLocalAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port));
	PacketSinkHelper tcpReceiver2 ("ns3::TcpSocketFactory", tcpReceiverLocalAddress2);
	tcpReceiver2.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
	ApplicationContainer receiverApp2 = tcpReceiver2.Install (wirtop.GetRight (1));
	receiverApp2.Start (Seconds (1.0));
	receiverApp2.Stop (Seconds (stopTime));

	port = 50002;

	/**
	* TCP application that send data from Node2 to Node4
	* Configuring sender application
	* Installing  application on left side nodes
	* Starting the application on left side node at 1st second
	*/
	AddressValue remoteAddress3 (InetSocketAddress (wirtop.GetRightIpv4Address (1), port));
	BulkSendHelper tcpSender3 ("ns3::TcpSocketFactory", Address ());
	tcpSender3.SetAttribute ("Remote", remoteAddress3);
	tcpSender3.SetAttribute ("SendSize", UintegerValue (1000));
	ApplicationContainer senderApp3 = tcpSender3.Install (wirtop.GetLeft (1));
	senderApp3.Start (Seconds (1.0));
	senderApp3.Stop (Seconds (stopTime - 1));

    /**
    * Configuring receiver application
    * Installing packet sink application on right side nodes
    * Starting the application at 1st second and stoping it at stopTime
    */
    Address tcpReceiverLocalAddress3 (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper tcpReceiver3 ("ns3::TcpSocketFactory", tcpReceiverLocalAddress3);
    tcpReceiver3.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    ApplicationContainer receiverApp3 = tcpReceiver3.Install (wirtop.GetRight (2));
    receiverApp3.Start (Seconds (1.0));
    receiverApp3.Stop (Seconds (stopTime));

    // Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

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

    /**
    * Goodput Calculation at the sink nodes.
    */
    uint32_t totalRxBytesCounter = 0;
    Ptr <Application> app1 = receiverApp1.Get (0);
    Ptr <PacketSink> pktSink1 = DynamicCast <PacketSink> (app1);

    Ptr <Application> app2 = receiverApp2.Get (0);
    Ptr <PacketSink> pktSink2 = DynamicCast <PacketSink> (app2);

    Ptr <Application> app3 = receiverApp3.Get (0);
    Ptr <PacketSink> pktSink3 = DynamicCast <PacketSink> (app3);
    totalRxBytesCounter = pktSink1->GetTotalRx () + pktSink2->GetTotalRx () + pktSink3->GetTotalRx ();

    NS_LOG_UNCOND ("----------------------------\n:::::::::"
                     << "\nGoodput Bytes/sec:"
                     << totalRxBytesCounter/Simulator::Now ().GetSeconds ());
    NS_LOG_UNCOND ("----------------------------");

    std::cout<<"\n Successful \n";

    Simulator::Destroy ();
    
    return 0;

}
