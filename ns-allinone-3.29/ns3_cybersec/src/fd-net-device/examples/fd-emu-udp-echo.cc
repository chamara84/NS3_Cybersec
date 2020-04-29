/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of Washington, 2012 INRIA 
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
 */

// Network topology
//
// Normally, the use case for emulated net devices is in collections of
// small simulations that connect to the outside world through specific
// interfaces.  For example, one could construct a number of virtual
// machines and connect them via a host-only network.  To use the emulated
// net device, you would need to set all of the host-only interfaces in
// promiscuous mode and provide an appropriate device name (search for "eth1"
// below).  One could also use the emulated net device in a testbed situation
// where the host on which the simulation is running has a specific interface
// of interested.  You would also need to set this specific interface into
// promiscuous mode and provide an appropriate device name.
//
// This philosophy carries over to this simple example.
//
// We don't assume any special configuration and all of the ns-3 emulated net
// devices will actually talk to the same underlying OS device.  We rely on
// the fact that the OS will deliver copies of our packets to the other ns-3
// net devices since we operate in promiscuous mode.
//
// Packets will be sent out over the device, but we use MAC spoofing.  The
// MAC addresses will be generated using the Organizationally Unique Identifier
// (OUI) 00:00:00 as a base.  This vendor code is not assigned to any
// organization and so should not conflict with any real hardware.  We'll use
// the first n of these addresses, where n is the number of nodes, in this
// simualtion.  It is up to you to determine that using these MAC addresses is
// okay on your network and won't conflict with anything else (including another
// simulation using emu devices) on your network.  Once you have made this
// determination, you need to put the interface you chose into promiscuous mode.
// We don't do it for you since you need to think about it first.
//
// This simulation uses the real-time simulator and so will consume ten seconds
// of real time.
//
// By default, we create the following topology
//
//            n0    n1  
//            |     |   
//            -------
//             "eth1"
//
// - UDP flows from n0 to n1 and back
// - DropTail queues
// - Tracing of queues and packet receptions to file "udp-echo.tr"
// - pcap tracing on all devices
//
// Another mode of operation corresponds to the wiki HOWTO
// 'HOWTO use ns-3 scripts to drive real hardware'
//
// If the --client mode is specified, only one ns-3 node is created 
// on the specified device name, assuming that a server node is
// on another virtual machine.  The client node will use 10.1.1.2
//
// If the --server mode is specified, only one ns-3 node is created 
// on the specified device name, assuming that a client node is
// on another virtual machine.  The server node will use 10.1.1.1

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/callback.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/socket.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/tcp-option-rfc793.h"
#include "ns3/header.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("EmulatedUdpEchoExample");
int changeVariables = 0;
std::list<Ptr<Socket> > m_socketList; //the accepted sockets
/*static void
 SendMsg (Ptr<Packet> packet,Ptr<Socket> socket)
 {

	   unsigned char buffer[1024] ;
	   unsigned int bufferFloat[1024];


	   if(changeVariables){
		   packet->CopyData (buffer, packet->GetSize ());
		          int integerData;
		          float floatingPointData;
		          memcpy(&bufferFloat, &buffer[0], 8);

		          bufferFloat[0] = ntohl(bufferFloat[0]);
		          bufferFloat[1] = ntohl(bufferFloat[1]);
		          integerData =  bufferFloat[0];
		          memcpy(&floatingPointData, &bufferFloat[1], 4);
		          integerData = integerData + (int)ceil(integerData*0.1);
		          floatingPointData = floatingPointData*1.1;
		          memcpy(&bufferFloat[0], &integerData, 4);
		          memcpy(&bufferFloat[1], &floatingPointData, 4);
		          bufferFloat[0] = htonl(bufferFloat[0]);
		          bufferFloat[1] = htonl(bufferFloat[1]);
		          memcpy(&buffer, & bufferFloat, 8);
		          Ptr<Packet> packetNew = Create<Packet>(&buffer,8);
		          int numbTx = socket->Send(packetNew);
	   }
 }*/

bool HandleAcceptRequest (Ptr<Socket> s, const Address& from)
{
	NS_LOG_INFO(" HANDLE ACCEPT REQUEST FROM " <<  InetSocketAddress::ConvertFrom(from));

	return true;
}

 void
 SocketPrinter (Ptr<Socket> socket)
 {


   Ptr<Packet> packet;


//   Ptr<SocketFactory> rxSocketFactory = n->Get (0)->GetObject<TcpSocketFactory> ();
//   Ptr<Socket> txSocketn0 = rxSocketFactory->CreateSocket ();
//   txSocketn0->Bind();
//   txSocketn0->Connect (InetSocketAddress (Ipv4Address ("172.24.9.244"), 7001));
   while ((socket->GetRxAvailable())>0)
     {
       std::cout << "at=" << Simulator::Now ().GetSeconds () << "s, rx bytes=" << packet->GetSize () << std::endl;

       uint32_t toRead =socket->GetRxAvailable ();
       packet = socket->Recv (toRead, 0);
       unsigned char buffer[1024] ;
       unsigned int bufferFloat[1024];
       packet->CopyData (buffer, packet->GetSize ());
       int integerData;
       float floatingPointData;
       memcpy(&bufferFloat, &buffer[0], 8);

       bufferFloat[0] = ntohl(bufferFloat[0]);
       bufferFloat[1] = ntohl(bufferFloat[1]);
       memcpy(&floatingPointData, &bufferFloat[1], 4);
       printf("point0 = %d\n", bufferFloat[0]);
       printf("point1 = %f\n", floatingPointData);
       //Ptr<Packet> copy = packet->Copy ();
       //Ipv4Header iph;
       //copy->RemoveHeader (iph);
       //Ipv4Address destAddr = iph.GetDestination();

       //int numbTx = txSocketn0->Send(packet);

       //std::printf("Float as int:  %x\n",bufferFloat[1]);
       //std::printf("Float as float: %f\n",floatingPointData);
       integerData =  bufferFloat[0];
       memcpy(&floatingPointData, &bufferFloat[1], 4);
       integerData = integerData + (int)ceil(integerData*0.1);
       floatingPointData = floatingPointData*1.1;
       memcpy(&bufferFloat[0], &integerData, 4);
       memcpy(&bufferFloat[1], &floatingPointData, 4);
       bufferFloat[0] = htonl(bufferFloat[0]);
       bufferFloat[1] = htonl(bufferFloat[1]);
       memcpy(&buffer, &bufferFloat, 8);
       Ptr<Packet> packetNew = Create<Packet>(buffer,8);
       socket->Send(packetNew);
       //txSocketn0->SetSendCallback (MakeBoundCallback (&SendMsg,packet));

     }
 }

void HandleAccept (Ptr<Socket> s, const Address& from)
		  {
	NS_LOG_FUNCTION (s << from);
	s->SetRecvCallback (MakeCallback (&SocketPrinter));
	m_socketList.push_back (s);
		  }
// static void
// PrintTraffic (NodeContainer *n,Ptr<Socket> socket)
// {
//
//   socket->SetRecvCallback (MakeBoundCallback (&SocketPrinter,n));
//   std::printf("after Print traffic method \n");
// }


void HandleClose(Ptr<Socket> s1)
{
	NS_LOG_INFO(" PEER CLOSE ");
	NS_LOG_INFO("**********************************************************");

	//Ptr<Socket> s2 = m_pair[s1];

	/*
	m_conn.erase(s1);
	m_conn.erase(s2);
	m_pair.erase(s1);
	m_pair.erase(s2);
	PrintPairs();
	NS_LOG_INFO("Closing socket S1 " << s1);
	s1->Close();
	NS_LOG_INFO("Closing socket S2 " << s2);
	s2->Close();
	NS_LOG_INFO("Done closing sockets " << s1 << " " << s2);
	*/
	if (s1) {
		s1->Close();
		s1 = NULL;
	}
}
int
main (int argc, char *argv[])
{
  ns3::PacketMetadata::Enable ();
  std::string deviceName ("eno1");
  std::string encapMode ("Dix");
  bool clientMode = false;
  bool serverMode = false;
  double stopTime = 10;
  uint32_t nNodes = 2;
  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpTahoe"));


  //
  // Allow the user to override any of the defaults at run-time, via command-line
  // arguments
  //
  CommandLine cmd;
  cmd.AddValue ("client", "client mode", clientMode);
  cmd.AddValue ("server", "server mode", serverMode);
  cmd.AddValue ("deviceName", "device name", deviceName);
  cmd.AddValue ("stopTime", "stop time (seconds)", stopTime);
  cmd.AddValue ("encapsulationMode", "encapsulation mode of emu device (\"Dix\" [default] or \"Llc\")", encapMode);
  cmd.AddValue ("nNodes", "number of nodes to create (>= 2)", nNodes);

  cmd.Parse (argc, argv);

  GlobalValue::Bind ("SimulatorImplementationType",
                     StringValue ("ns3::RealtimeSimulatorImpl"));

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  if (clientMode && serverMode)
    {
      NS_FATAL_ERROR("Error, both client and server options cannot be enabled.");
    }
  //
  // need at least two nodes
  //
  nNodes = nNodes < 2 ? 2 : nNodes;

  //
  // Explicitly create the nodes required by the topology (shown above).
  //
  NS_LOG_INFO ("Create nodes.");
  NodeContainer n;
  n.Create (1);
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1460));

  InternetStackHelper internet; //installing the networking capability

  internet.Install (n);

  //
  // Explicitly create the channels required by the topology (shown above).
  //
  NS_LOG_INFO ("Create channels.");
  EmuFdNetDeviceHelper emu;
  emu.SetDeviceName (deviceName);
  emu.SetAttribute ("EncapsulationMode", StringValue (encapMode));

  NetDeviceContainer d;
  Ipv4AddressHelper ipv4;
  Ipv4InterfaceContainer i;
  ApplicationContainer apps;

  ipv4.SetBase ("172.24.0.0", "255.255.0.0", "0.0.2.55");
  if (clientMode)
    {
      d = emu.Install (n.Get (0));
      // Note:  incorrect MAC address assignments are one of the confounding
      // aspects of network emulation experiments.  Here, we assume that there
      // will be a server mode taking the first MAC address, so we need to
      // force the MAC address to be one higher (just like IP address below)
      Ptr<FdNetDevice> dev = d.Get (0)->GetObject<FdNetDevice> ();
      dev->SetAddress (Mac48Address ("00:00:00:00:00:02"));
      NS_LOG_INFO ("Assign IP Addresses.");
      ipv4.NewAddress ();  // burn the 10.1.1.1 address so that 10.1.1.2 is next
      i = ipv4.Assign (d);
    }
  else if (serverMode)
    {

      Ipv4StaticRoutingHelper ipv4RoutingHelper;
       // Create static routes from A to C

      Ptr<Ipv4> ipv4A = n.Get(0)->GetObject<Ipv4>();

      Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting (ipv4A);
       // The ifIndex for this outbound route is 1; the first p2p link added

      staticRoutingA->SetDefaultRoute(Ipv4Address("172.24.0.1"),1,0);

      d = emu.Install (n.Get (0));
      Ptr<FdNetDevice> dev = d.Get (0)->GetObject<FdNetDevice> ();
      dev->SetAddress (Mac48Address ("64:00:6a:5c:af:58"));
      NS_LOG_INFO ("Assign IP Addresses.");
      i = ipv4.Assign (d);
    }
  else
    {
      d = emu.Install (n);
      NS_LOG_INFO ("Assign IP Addresses.");
      i = ipv4.Assign (d);
    }
    
  if (serverMode)
    {

      //
      // Create a UdpEchoServer application 
      //
      NS_LOG_INFO ("Create Applications.");
      //uint16_t sinkPort = 12345;
      // Create the UDP sockets
      Ptr<SocketFactory> rxSocketFactory = n.Get (0)->GetObject<TcpSocketFactory> ();
            Ptr<Socket> rxSocketn0 = rxSocketFactory->CreateSocket ();

      Ptr<Ipv4> ipV4Info = n.Get (0)->GetObject<Ipv4>();
      int interfaceIndex = ipV4Info->GetInterfaceForAddress(Ipv4Address("172.24.2.55"));
      						std::cout<<"NetDev:"<<interfaceIndex;
      						rxSocketn0->BindToNetDevice(n.Get (0)->GetDevice(interfaceIndex));

      //rxSocketn0->SetAttribute("SegmentSize", UintegerValue (1460));
     // rxSocketn0->SetAttribute("MaxWindowSize", UintegerValue (60000));
     //rxSocketn0->SetAttribute("WindowScaling", BooleanValue (true));
     //rxSocketn0->SetAttribute("RcvBufSize", ns3::UintegerValue(60000));
      rxSocketn0->Bind (InetSocketAddress (Ipv4Address("172.24.2.55"), 12345));
      rxSocketn0->Listen();
      rxSocketn0->SetRecvCallback (MakeCallback (&SocketPrinter));
      rxSocketn0->SetAcceptCallback (MakeCallback(&HandleAcceptRequest),MakeCallback (&HandleAccept));
      rxSocketn0->SetCloseCallbacks(MakeCallback(&HandleClose), MakeCallback(&HandleClose));
      rxSocketn0->Initialize();

        //rxSocketn0->Connect (InetSocketAddress (Ipv4Address ("172.24.9.244"), 7001));

//      Address sinkLocalAddress (InetSocketAddress (Ipv4Address ("172.24.2.55"), sinkPort));
//           PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
//             ApplicationContainer sinkApp = sinkHelper.Install (n.Get(0));
//            sinkApp.Start (Seconds (1.0));
//             sinkApp.Stop (Seconds (60.0));

//        Ptr<TcpEchoServer> app = CreateObject<TcpEchoServer> ();
//             //app->Setup (ingressNode,Ipv4Address ("172.24.2.55"),Ipv4Address ("10.1.8.6"), 7001,7001,INGRESS_NODE);
//             app->SetAttribute("Local",Ipv4AddressValue("172.24.2.55"));
//             app->SetAttribute("Port", UintegerValue (12345));
//
//
//             n.Get(0)->AddApplication (app);
//             app->SetStartTime (Seconds (1.));
//             app->SetStopTime (Seconds (stopTime));

        //rxSocketn0->SetRecvCallback (MakeCallback (&SocketPrinter));
       // rxSocket2->SetRecvCallback (MakeCallback (&SocketPrinter) );
        //PrintTraffic ( &n, rxSocketn0);

    /*  UdpEchoServerHelper server (4888);
      apps = server.Install (n.Get (0));
      apps.Start (Seconds (1.0));
      apps.Stop (Seconds (stopTime));*/
    }
  else if (clientMode)
    {
      //
      // Create a UdpEchoClient application to send UDP datagrams 
      //
      uint32_t packetSize = 1024;
      uint32_t maxPacketCount = 20;
      Time interPacketInterval = Seconds (0.1);
      UdpEchoClientHelper client (Ipv4Address ("10.1.1.2"), 9);
      client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client.SetAttribute ("Interval", TimeValue (interPacketInterval));
      client.SetAttribute ("PacketSize", UintegerValue (packetSize));
      apps = client.Install (n.Get (0));
      apps.Start (Seconds (2.0));
      apps.Stop (Seconds (stopTime));
      // Users may find it convenient to initialize echo packets with actual data;
       // the below lines suggest how to do this
       //
         client.SetFill (apps.Get (0), "Hello World");

         client.SetFill (apps.Get (0), 0xa5, 1024);

         uint8_t fill[] = { 0, 1, 2, 3, 4, 5, 6};
        client.SetFill (apps.Get (0), fill, sizeof(fill), 1024);
    }
  else
    {
      //
      // Create a UdpEchoServer application on node one.
      //
      NS_LOG_INFO ("Create Applications.");
      UdpEchoServerHelper server (9);
      apps = server.Install (n.Get (1));
      apps.Start (Seconds (1.0));
      apps.Stop (Seconds (stopTime));
    
      //
      // Create a UdpEchoClient application to send UDP datagrams from node zero to node one.
      //
      uint32_t packetSize = 1024;
      uint32_t maxPacketCount = 20;
      Time interPacketInterval = Seconds (0.1);
      UdpEchoClientHelper client (i.GetAddress (1), 9);
      client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client.SetAttribute ("Interval", TimeValue (interPacketInterval));
      client.SetAttribute ("PacketSize", UintegerValue (packetSize));
      apps = client.Install (n.Get (0));
      apps.Start (Seconds (2.0));
      apps.Stop (Seconds (stopTime));
    }

  emu.EnablePcapAll ("fd-emu-udp-echo", true);
  emu.EnableAsciiAll ("fd-emu-udp-echo.tr");

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (stopTime + 2));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}