/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

/*
En este codigo se definen 3 redes adhoc inalambricas
La red de mayor jerarquia sera netone
Las otras dos, nettwo, netone, solo se podrían comunicar através de netone
El numero minimo de nodos de cada red puede tener es de dos. En cada red habra almenos
una cabeza de cluster

*/




#include <iostream>
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/ssid.h"
#include "ns3/on-off-helper.h"
#include "ns3/qos-txop.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/csma-helper.h"
#include "ns3/animation-interface.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("taller");
void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

int main (int argc, char *argv[])
{
    //DEfinicion de sistema de logs que se usara
    NS_LOG_INFO("CREATING NODES.");
    LogComponentEnable("taller", LOG_LEVEL_INFO);

    //Definicion de valores por defecto de variables que se pueden modificar como parametros del
    //programa
    uint32_t packetSize = 1000; // bytes
    uint32_t numPackets = 4;
    uint32_t numNodesNetOne = 10;
    uint32_t numNodesNetTwo = 10;
    uint32_t numNodesNetThree = 10;
    double interval = 1.0; // seconds


    //Configuracion de parametros de programa que se podran ingresar mediante ./waf ...taller --<par> = valor
    CommandLine cmd;
    cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue ("numPackets", "number of packets generated", numPackets);
    cmd.AddValue ("numNodesNetOne", "number of nodes for main network", numNodesNetOne);
    cmd.AddValue ("numNodesNetTwo", "number of nodes for second network", numNodesNetTwo);
    cmd.AddValue ("numNodesNetThree", "number of nodes for third network", numNodesNetThree);
    cmd.Parse (argc, argv);

    //si el usuario escoge un numero de nodos menor al limite, automanticamente este valor se cambia
    //por el valor minimo
    uint32_t minValue = 2;
    if(numNodesNetOne < 2) numNodesNetOne = minValue;
    if(numNodesNetTwo < 2) numNodesNetTwo = minValue;
    if(numNodesNetThree < 2) numNodesNetThree = minValue;
    //se establecen los nodos que van a servir como conexion entre las redes de baja jerarquia
    // y la red principal
    uint32_t mainTwoAttachmentIndex = 0;
    //uint32_t mainThreeAttachmentIndex = numNodesNetOne-1;


    Time interPacketInterval = Seconds (interval);

    ///////////////////////////////////////////////////////////////////////////
  //                                                                         //
  //                            RED PRINCIPAL                               //
  //                                                                       //
  ///////////////////////////////////////////////////////////////////////////

  //Contenedor de nodos que almacena todos los nodos de la red principal
  NodeContainer mainNodeContainer;
  //Creacion de los nodos
  mainNodeContainer.Create(numNodesNetOne);
  NS_LOG_INFO("MAIN: "<< numNodesNetOne <<" nodes created");

  //Intalacion de dispositivos de red wifi en modo adhoc, capa fisica y capa mac a los nodos
  WifiHelper wifi;
  WifiMacHelper mac;
  mac.SetType ("ns3::AdhocWifiMac");
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  NetDeviceContainer mainDeviceContainer = wifi.Install (wifiPhy, mac, mainNodeContainer);


  //Intalacion de pila de protocolos a los dispositivos de red
  NS_LOG_INFO ("MAIN:Enabling OLSR routing on all nodes");
  OlsrHelper olsr;
  InternetStackHelper internet;
  internet.SetRoutingHelper (olsr);
  //se establace al protocolo olsr como protocolo de enrutamiento para la red.
  //Este funciona con tablas de enrutamiento en redes adhoc moviles
  internet.Install (mainNodeContainer);

  //Asignacion de direcciones ip
  NS_LOG_INFO ("MAIN:setting up ip adresses on all nodes");
  Ipv4AddressHelper ipAddrs;
  ipAddrs.SetBase ("192.168.0.0", "255.255.255.0");
  //cada dispositivo de red tendra direcciones 192.168.0.1, 192.168.0.2....192.168.0.254
  ipAddrs.Assign (mainDeviceContainer);

  //La red adhoc es movil, por lo tanto se definen atributos de movilidad a los nodos

  MobilityHelper mobility;
  //Primero se define la posicion de cada nodo en una grilla
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (20.0),
                                 "MinY", DoubleValue (20.0),
                                 "DeltaX", DoubleValue (20.0),
                                 "DeltaY", DoubleValue (20.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  //Luego se les indica a los nodos en que forma se pueden mover
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  mobility.Install (mainNodeContainer);


  /******Transmision de informacion de un nodo a otro*****/
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (mainNodeContainer.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (mainNodeContainer.Get (1), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", mainDeviceContainer);

  // Output what we are doing
  NS_LOG_INFO ("Testing " << numPackets  << " packets sent");

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval);
/******Transmision de informacion de un nodo a otro*****/



    ///////////////////////////////////////////////////////////////////////////
  //                                                                         //
  //                                RED 2                                   //
  //                                                                       //
  ///////////////////////////////////////////////////////////////////////////

  NS_LOG_INFO ("TWO:Creating net 2 atached by main net by node" << mainTwoAttachmentIndex);
  //Creamos un contenedor temporal donde se guardan los nuevos nodos 
  NodeContainer newNodesNetTwo;
  //Creamos numNodesNetTwo - 1 nuevos nodos, el -1 es porque habrá un nodo adicional que es el de
  //union con la red principal
  newNodesNetTwo.Create (numNodesNetTwo - 1);
  // Creacion del contenedor de nodos para la red
  NodeContainer secondNodeContainer (mainNodeContainer.Get (mainTwoAttachmentIndex), newNodesNetTwo);

  Simulator::Run ();
  Simulator::Destroy ();


  return 0;
}

